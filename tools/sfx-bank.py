#!/usr/bin/env python3
"""W2.2 Paula sound bank generator and listening previews.

Reads the kept sounds from docs/sound-inventory.md (tools/sound-inventory.py),
their sources from extracted/audio/manifest.json and the per-class choices
from tools/sfx-bank-policy.json.

  sfx-bank.py --preview   renders, for every kept sound and every candidate
                          rate x crop (long sounds: rate x cut), a preview WAV
                          of what Paula would play, the source for comparison,
                          per-class reels, and extracted/amiga-sfx/preview/
                          index.html, a local listening page that shows the
                          Chip totals of the choices and exports a policy.
  sfx-bank.py --bank      converts the policy's current choice of every sound
                          to signed 8-bit samples in extracted/amiga-sfx/bank/
                          with a manifest of rates, Paula volumes, loops and
                          per-scene Chip totals.

Conversion: ffmpeg (swr) downmixes and resamples to the Paula rate of the
nearest period (3,546,895 / period, PAL). One-shots are cropped after the
last 2 ms window whose peak reaches the crop threshold and faded over that
last window. Long sounds keep a head with a fade-out, a faded window around their
loudest part (the winds' swell), or a loop segment whose
head is crossfaded (equal power) with the audio after it. Each sample is peak
normalised to 8 bits; the asset volume and the normalisation move into the
Paula volume (0-64), so quiet sounds keep their resolution. Rounding, no
dither. Lengths are padded to whole words.

Previews: zero-order hold at the Paula rate, with (p/) and without (praw/)
the A500's fixed one-pole RC low-pass (360 ohm, 0.1 uF: 4,421 Hz), the LED
filter always off, rendered at 4 x
44,100 Hz and decimated to 44,100 Hz 16-bit. One voice at volume 64 is full
scale; the source preview uses the same scale with the asset volume, so the
two compare at equal loudness. The ~5 Hz output high-pass is left out.
"""
import argparse
import hashlib
import html
import json
import re
import subprocess
import wave
from pathlib import Path

import numpy as np

VERSION = "1.0.1"
ROOT = Path(__file__).resolve().parents[1]
INVENTORY = ROOT / "docs/sound-inventory.md"
AUDIO = ROOT / "extracted/audio/manifest.json"
POLICY = ROOT / "tools/sfx-bank-policy.json"
OUTPUT = ROOT / "extracted/amiga-sfx"
PAULA_CLOCK = 3546895
MIN_PERIOD = 124
OUT_RATE = 44100
OVERSAMPLE = 4
RC_HZ = 1 / (2 * np.pi * 360 * 0.1e-6)
SCENE_ORDER = ["front", "gameplay", "shop", "pause", "clear", "results"]


def sha(data):
    return hashlib.sha256(data).hexdigest()


def period(rate):
    return round(PAULA_CLOCK / rate)


def paula_rate(rate):
    return PAULA_CLOCK / period(rate)


def kept_sounds():
    rows = []
    for line in INVENTORY.read_text().splitlines():
        cells = [c.strip() for c in line.strip().strip("|").split("|")]
        # ID, name, bytes, s, loop, scenes, played by, [observed, peak,] verdict
        if len(cells) not in (8, 10) or not cells[0].isdigit():
            continue
        verdict = cells[-1]
        if verdict.startswith("drop"):
            continue
        rows.append({"id": int(cells[0]), "name": cells[1].strip("`"),
                     "scenes": [s.strip() for s in cells[5].split(",")], "verdict": verdict})
    return rows


def classify(sounds, policy):
    for s in sounds:
        for c in policy["classes"]:
            if any(re.search(p, s["name"]) for p in c["match"]):
                s["class"] = c["key"]
                break
        else:
            raise SystemExit(f"no class matches {s['id']} {s['name']}")
        if s["class"] == "long" and str(s["id"]) not in policy["long"]:
            raise SystemExit(f"long sound {s['id']} {s['name']} has no cuts")


def decode(path, rate):
    raw = subprocess.run(
        ["ffmpeg", "-v", "error", "-i", str(path), "-ac", "1",
         "-af", f"aresample={rate}:filter_size=64:phase_shift=12:cutoff=0.95",
         "-f", "f32le", "-"], check=True, capture_output=True).stdout
    return np.frombuffer(raw, dtype="<f4").astype(np.float64)


def crop(x, rate, threshold_db):
    window = max(1, int(rate) // 500)
    loud = np.flatnonzero(np.abs(x) >= 10 ** (threshold_db / 20))
    if not loud.size:
        return x[:0]
    end = min((loud[-1] // window + 1) * window, x.size)
    y = x[:end].copy()
    n = min(window, y.size)
    y[-n:] *= np.linspace(1, 0, n + 1)[1:]
    return y


def cut(x, rate, spec):
    """Long sounds: a faded head, or a seamless loop. Returns (samples, loop)."""
    if spec.get("drop"):
        return x[:0], False
    if "swellSeconds" in spec:
        # A window around the loudest 50 ms, faded in over its first 40 % and
        # out over its last 50 %: the winds swell for seconds before their body.
        n = min(x.size, int(round(spec["swellSeconds"] * rate)))
        hop = max(1, int(rate) // 20)
        env = np.sqrt(np.convolve(x * x, np.ones(hop) / hop, "same"))
        a = int(np.clip(np.argmax(env) - 0.45 * n, 0, x.size - n))
        y = x[a:a + n].copy()
        fi, fo = int(0.4 * n), int(0.5 * n)
        y[:fi] *= np.sin(np.linspace(0, np.pi / 2, fi)) ** 2
        y[-fo:] *= np.cos(np.linspace(0, np.pi / 2, fo)) ** 2
        return y, False
    if "seconds" in spec:
        n = min(x.size, int(round(spec["seconds"] * rate)))
        y = x[:n].copy()
        f = min(n, int(round(spec.get("fadeSeconds", min(0.5, 0.25 * spec["seconds"])) * rate)))
        if n < x.size and f:
            y[-f:] *= np.cos(np.linspace(0, np.pi / 2, f)) ** 2
        return y, False
    a = int(round(spec["loopStart"] * rate))
    n = int(round(spec["loopSeconds"] * rate))
    f = int(round(spec.get("crossfadeSeconds", min(0.25, spec["loopSeconds"] / 4)) * rate))
    if a + n + f > x.size:
        raise SystemExit(f"loop {spec} runs past the source")
    y = x[a:a + n].copy()
    w = np.linspace(0, np.pi / 2, f)
    # The head fades in from the audio after the segment, so the wrap from
    # y[-1] (= x[a+n-1]) to y[0] continues as x[a+n] did.
    y[:f] = x[a:a + f] * np.sin(w) + x[a + n:a + n + f] * np.cos(w)
    return y, True


def quantise(x, volume):
    peak = float(np.max(np.abs(x))) if x.size else 0.0
    if not peak:
        return np.zeros(0, np.int8), 0
    q = np.clip(np.rint(x / peak * 127), -128, 127).astype(np.int8)
    if q.size & 1:
        q = np.append(q, np.int8(0))
    return q, int(min(64, max(1, round(64 * volume * peak))))


def kernel(a500_filter):
    fs = OUT_RATE * OVERSAMPLE
    a = np.exp(-2 * np.pi * RC_HZ / fs)
    rc = (1 - a) * a ** np.arange(96) if a500_filter else np.ones(1)
    taps = 127
    n = np.arange(taps) - (taps - 1) / 2
    fc = 20000 / fs
    aa = 2 * fc * np.sinc(2 * fc * n) * np.blackman(taps)
    k = np.convolve(rc / rc.sum(), aa / aa.sum())
    return k


# Preview folders: "p" with the A500's fixed filter, "praw" without it (the
# bare zero-order hold, as an A1200 or an unfiltered emulator plays it).
KERNELS = {"p": kernel(True), "praw": kernel(False)}


def paula_preview(q, rate, volume, loop, k, loop_seconds=6.0):
    if not q.size:
        return np.zeros(int(0.1 * OUT_RATE))
    s = q.astype(np.float64) * (volume / 64 / 128)
    if loop:
        s = np.tile(s, int(np.ceil(loop_seconds * rate / s.size)))
    fs = OUT_RATE * OVERSAMPLE
    n = int(np.ceil(s.size * fs / rate))
    held = s[np.minimum((np.arange(n) * rate / fs).astype(np.int64), s.size - 1)]
    held = np.concatenate([held, np.zeros(k.size)])
    return np.convolve(held, k)[63::OVERSAMPLE]  # the anti-alias FIR's centre


def write_wav(path, y):
    path.parent.mkdir(parents=True, exist_ok=True)
    pcm = np.clip(np.rint(y * 32767), -32768, 32767).astype("<i2")
    with wave.open(str(path), "wb") as out:
        out.setnchannels(1)
        out.setsampwidth(2)
        out.setframerate(OUT_RATE)
        out.writeframes(pcm.tobytes())


def variant_key(rate, choice):
    return f"{rate}_{choice}"


def crop_key(db):
    return f"c{int(-db)}"


def build(sound, source, policy, rate, choice):
    """choice is a crop in dB for one-shots or a cut name for long sounds."""
    x = decode(source, round(paula_rate(rate)))
    if sound["class"] == "long":
        y, loop = cut(x, paula_rate(rate), policy["long"][str(sound["id"])]["cuts"][choice])
    else:
        y, loop = crop(x, paula_rate(rate), choice), False
    q, vol = quantise(y, sound["volume"])
    return q, vol, loop


def choices(sound, policy):
    if sound["class"] == "long":
        return list(policy["long"][str(sound["id"])]["cuts"])
    return policy["crops"]


def current(sound, policy):
    c = next(c for c in policy["classes"] if c["key"] == sound["class"])
    if sound["class"] == "long":
        return c["rate"], policy["long"][str(sound["id"])]["cut"]
    return c["rate"], c["crop"]


def max_pitch(sound, policy):
    c = next(c for c in policy["classes"] if c["key"] == sound["class"])
    return c.get("pitchSounds", {}).get(str(sound["id"]), c.get("pitchPeak", 1.0))


def load():
    policy = json.loads(POLICY.read_text())
    audio_bytes = AUDIO.read_bytes()
    audio = {s["id"]: s for s in json.loads(audio_bytes)["sounds"]}
    sounds = kept_sounds()
    classify(sounds, policy)
    for s in sounds:
        a = audio[s["id"]]
        s["volume"] = a["volume"]
        s["source"] = AUDIO.parent / a["output"]
        s["sourceSha256"] = a["sha256"]
    inputs = {"inventory": {"path": str(INVENTORY.relative_to(ROOT)), "sha256": sha(INVENTORY.read_bytes())},
              "audio": {"path": str(AUDIO.relative_to(ROOT)), "sha256": sha(audio_bytes)},
              "policy": {"path": str(POLICY.relative_to(ROOT)), "sha256": sha(POLICY.read_bytes())}}
    return policy, sounds, inputs


def tool_info():
    ffmpeg = subprocess.run(["ffmpeg", "-version"], capture_output=True, text=True).stdout.split("\n")[0]
    return {"name": "sfx-bank.py", "version": VERSION, "ffmpeg": ffmpeg, "numpy": np.__version__}


def scene_totals(sounds, size):
    out = {}
    for scene in SCENE_ORDER:
        out[scene] = sum(size(s) for s in sounds if scene in s["scenes"])
    run = [s for s in sounds if {"gameplay", "shop", "pause"} & set(s["scenes"])]
    out["run (gameplay, shop, pause)"] = sum(size(s) for s in run)
    out["all"] = sum(size(s) for s in sounds)
    return out


def bank(policy, sounds, inputs):
    out = OUTPUT / "bank"
    out.mkdir(parents=True, exist_ok=True)
    rows = []
    for s in sounds:
        rate, choice = current(s, policy)
        q, vol, loop = build(s, s["source"], policy, rate, choice)
        raw = q.tobytes()
        (out / f"{s['id']:03d}_{s['name']}.raw").write_bytes(raw)
        rows.append({"id": s["id"], "name": s["name"], "class": s["class"], "scenes": s["scenes"],
                     "rate": rate, "period": period(rate), "choice": choice, "loop": loop,
                     "paulaVolume": vol, "bytes": len(raw), "maxPitch": max_pitch(s, policy),
                     "minPeriodOk": period(rate) / max_pitch(s, policy) >= MIN_PERIOD,
                     "sourceSha256": s["sourceSha256"], "sha256": sha(raw)})
    by_id = {r["id"]: r for r in rows}
    totals = scene_totals(sounds, lambda s: by_id[s["id"]]["bytes"])
    (out / "manifest.json").write_text(json.dumps({
        "schemaVersion": 1, "tool": tool_info(), "inputs": inputs,
        "format": {"bits": 8, "channels": 1, "paulaClock": PAULA_CLOCK, "dither": None,
                   "resampler": "swr filter_size=64 cutoff=0.95"},
        "sceneBytes": totals, "sounds": rows}, indent=1) + "\n")
    print(f"{len(rows)} sounds -> {out}")
    for k, v in totals.items():
        print(f"  {k}: {v / 1024:.1f} KiB")
    bad = [r for r in rows if not r["minPeriodOk"]]
    for r in bad:
        print(f"  warning: {r['id']} {r['name']} at {r['rate']} Hz x pitch {r['maxPitch']} needs period < {MIN_PERIOD}")


def preview(policy, sounds, inputs):
    out = OUTPUT / "preview"
    data = []
    reels = {}
    gap = np.zeros(int(0.15 * OUT_RATE))
    for i, s in enumerate(sounds):
        print(f"\r{i + 1}/{len(sounds)} {s['name']:<40}", end="", flush=True)
        src = decode(s["source"], OUT_RATE) * s["volume"]
        if s["class"] == "long":
            src = src[:12 * OUT_RATE]
        write_wav(out / "src" / f"{s['id']}.wav", src)
        reels.setdefault((s["class"], "src"), []).extend([src, gap])
        variants = {}
        for rate in policy["rates"]:
            for choice in choices(s, policy):
                q, vol, loop = build(s, s["source"], policy, rate, choice)
                key = variant_key(rate, choice if s["class"] == "long" else crop_key(choice))
                for folder, k in KERNELS.items():
                    y = paula_preview(q, paula_rate(rate), vol, loop, k)
                    write_wav(out / folder / f"{s['id']}_{key}.wav", y)
                    if s["class"] != "long":
                        reels.setdefault((s["class"], folder + "_" + key), []).extend([y, gap])
                variants[key] = {"bytes": int(q.size), "vol": vol, "loop": loop}
        rate, choice = current(s, policy)
        data.append({"id": s["id"], "name": s["name"], "class": s["class"], "scenes": s["scenes"],
                     "verdict": s["verdict"], "volume": s["volume"], "maxPitch": max_pitch(s, policy),
                     "seconds": round(src.size / OUT_RATE, 2), "variants": variants,
                     "choices": [c if s["class"] == "long" else crop_key(c) for c in choices(s, policy)],
                     "cut": choice if s["class"] == "long" else None})
    print()
    for (cls, key), parts in reels.items():
        write_wav(out / "reel" / f"{cls}_{key}.wav", np.concatenate(parts))
    page = {"tool": tool_info(), "inputs": inputs, "policy": policy, "sounds": data,
            "periods": {str(r): period(r) for r in policy["rates"]},
            "paulaRates": {str(r): round(paula_rate(r), 1) for r in policy["rates"]},
            "minPeriod": MIN_PERIOD, "scenes": SCENE_ORDER}
    (out / "preview.json").write_text(json.dumps(page, indent=1) + "\n")
    template = (ROOT / "tools/sfx-bank-preview.html").read_text()
    (out / "index.html").write_text(template.replace("/*DATA*/null", json.dumps(page).replace("</", "<\\/")))
    print(f"{len(data)} sounds -> {out / 'index.html'}")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--preview", action="store_true")
    parser.add_argument("--bank", action="store_true")
    args = parser.parse_args()
    if not (args.preview or args.bank):
        parser.error("choose --preview and/or --bank")
    policy, sounds, inputs = load()
    if args.preview:
        preview(policy, sounds, inputs)
    if args.bank:
        bank(policy, sounds, inputs)


if __name__ == "__main__":
    main()
