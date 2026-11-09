#!/usr/bin/env python3
"""Convert the extracted sound effects to 8-bit mono 8363 Hz for Paula.

Reads extracted/audio/manifest.json (tools/extract-audio.py) and writes, per
sound, a listening WAV (8-bit unsigned PCM) and a raw signed 8-bit sample to
extracted/amiga-sfx/. ffmpeg (swr) downmixes and resamples to float; the
tail is cut after the last 2 ms window whose peak is at or above the
threshold (default -30 dBFS), so quiet gaps inside a sound survive. Levels are
not normalised, except that a sound whose resampled peak overshoots full
scale is attenuated to 0 dBFS instead of clipping (gainDb records it). The
sound's GameMaker volume is not applied (playback gain). Quantisation rounds
to nearest, without dither.
"""
import argparse
import hashlib
import json
import subprocess
import wave
from pathlib import Path

import numpy as np

VERSION = "1.0.0"
ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "extracted/audio/manifest.json"
OUTPUT = ROOT / "extracted/amiga-sfx"
RATE = 8363
WINDOW = max(1, RATE // 500)  # 2 ms


def decode(path):
    raw = subprocess.run(
        ["ffmpeg", "-v", "error", "-i", str(path), "-ac", "1",
         "-af", f"aresample={RATE}:filter_size=64:phase_shift=12:cutoff=0.95",
         "-f", "f32le", "-"],
        check=True, capture_output=True).stdout
    return np.frombuffer(raw, dtype="<f4").astype(np.float64)


def crop(samples, threshold_db):
    limit = 10 ** (threshold_db / 20)
    loud = np.flatnonzero(np.abs(samples) >= limit)
    if not loud.size:
        return samples[:0]
    end = (loud[-1] // WINDOW + 1) * WINDOW
    return samples[:min(end, samples.size)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--threshold-db", type=float, default=-30.0)
    parser.add_argument("--output", type=Path, default=OUTPUT)
    args = parser.parse_args()
    manifest_bytes = MANIFEST.read_bytes()
    manifest = json.loads(manifest_bytes)
    wav_dir, raw_dir = args.output / "wav", args.output / "raw"
    wav_dir.mkdir(parents=True, exist_ok=True)
    raw_dir.mkdir(parents=True, exist_ok=True)
    rows = []
    for sound in manifest["sounds"]:
        if not sound["output"].startswith("sound-effects/"):
            continue
        source = MANIFEST.parent / sound["output"]
        samples = decode(source)
        kept = crop(samples, args.threshold_db)
        peak = float(np.max(np.abs(samples))) if samples.size else 0.0
        gain = min(1.0, 1.0 / peak) if peak else 1.0
        signed = np.clip(np.rint(kept * gain * 127), -128, 127).astype(np.int8)
        stem = Path(sound["output"]).stem
        raw = signed.tobytes()
        (raw_dir / f"{stem}.raw").write_bytes(raw)
        with wave.open(str(wav_dir / f"{stem}.wav"), "wb") as out:
            out.setnchannels(1)
            out.setsampwidth(1)
            out.setframerate(RATE)
            out.writeframes((signed.astype(np.int16) + 128).astype(np.uint8).tobytes())
        rows.append({
            "id": sound["id"], "name": sound["name"], "volume": sound["volume"],
            "source": sound["output"], "sourceSha256": sound["sha256"],
            "sourceFrames": int(samples.size), "frames": int(signed.size),
            "bytes": len(raw), "peakDb": round(20 * np.log10(peak), 2) if peak else None,
            "gainDb": round(20 * np.log10(gain), 2),
            "sha256": hashlib.sha256(raw).hexdigest(),
        })
    ffmpeg = subprocess.run(["ffmpeg", "-version"], capture_output=True, text=True).stdout.split("\n")[0]
    (args.output / "manifest.json").write_text(json.dumps({
        "schemaVersion": 1, "tool": {"name": "convert-amiga-sfx.py", "version": VERSION},
        "ffmpeg": ffmpeg, "numpy": np.__version__,
        "input": {"path": str(MANIFEST.relative_to(ROOT)),
                  "sha256": hashlib.sha256(manifest_bytes).hexdigest()},
        "format": {"rate": RATE, "channels": 1, "bits": 8, "thresholdDb": args.threshold_db,
                   "windowFrames": WINDOW, "resampler": "swr filter_size=64 cutoff=0.95", "dither": None},
        "totalBytes": sum(r["bytes"] for r in rows), "sounds": rows,
    }, indent=1) + "\n")
    print(f"{len(rows)} sounds, {sum(r['bytes'] for r in rows)} bytes -> {args.output}")


if __name__ == "__main__":
    main()
