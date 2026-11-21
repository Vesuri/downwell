#!/usr/bin/env python3
"""W2.4: compare FS-UAE's recording of the Paula mixer probe with the host render.

  compare-paula-recording.py RECORDING.raw EXPECTED.wav [--wav OUT.wav]

RECORDING is FS-UAE's SDL disk-audio output (raw signed 16-bit stereo at
44.1 kHz) of amiga/verify_paula_mixer.sh; EXPECTED is the host render of the
same PaulaProbeSchedule (build/paula-mixer-test PACK WAV: the same mixer plan
and sequencer driving a Paula model). Both use zero-order hold with no output
filter, but the emulator's level, DC and sample timing differ, so the check
compares envelopes, not samples:

1. 10 ms RMS envelopes per side; the recording is aligned to the render by
   cross-correlating the summed envelopes, then corrected for drift.
2. Events: the stretches where the render is above -40 dB of its peak. Each
   event must sound in the recording (no missing or truncated sound), at the
   same level relative to the median event (within 3 dB), on the same side
   (left share within 0.15), lasting as long (to -40 dB of its peak, within
   30 ms), and nothing may sound where the render is silent.

The emulator writes slightly fewer frames than 882 per field (about 0.06%),
so the alignment fits a line through eight separately aligned segments and
refines each event by up to 40 ms. The whole-run envelope correlation per
side is only a coarse guard (at least 0.9; residual alignment noise keeps it
near 0.95); the per-event checks above are the criteria.

--wav also writes the aligned recording as a WAV to listen to.
"""
import argparse
import sys
import wave

import numpy as np

RATE = 44100
WINDOW = 441  # 10 ms
SLACK = 15    # local alignment, in windows


def envelopes(stereo):
    n = stereo.shape[0] // WINDOW
    x = stereo[: n * WINDOW].astype(np.float64).reshape(n, WINDOW, 2)
    x = x - x.mean(axis=(0, 1), keepdims=True)
    return np.sqrt((x ** 2).mean(axis=1))  # (n, 2)


def read_wav(path):
    with wave.open(path) as w:
        if w.getnchannels() != 2 or w.getsampwidth() != 2 or w.getframerate() != RATE:
            sys.exit(f"{path}: expected 16-bit stereo at {RATE} Hz")
        return np.frombuffer(w.readframes(w.getnframes()), dtype="<i2").reshape(-1, 2)


def events(active):
    out, start = [], None
    for i, a in enumerate(active):
        if a and start is None:
            start = i
        elif not a and start is not None:
            out.append((start, i))
            start = None
    if start is not None:
        out.append((start, len(active)))
    return out


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("recording")
    p.add_argument("expected")
    p.add_argument("--wav")
    p.add_argument("--events", help="the render's EXPECTED.events (field asset per play) to name problems")
    a = p.parse_args()
    starts = []
    if a.events:
        with open(a.events) as f:
            starts = [tuple(map(int, line.split())) for line in f if line.strip()]
    rec = np.fromfile(a.recording, dtype="<i2")
    rec = rec[: rec.size // 2 * 2].reshape(-1, 2)
    exp = read_wav(a.expected)
    er, ee = envelopes(rec), envelopes(exp)
    # Align: the recording starts at boot, so the render sits somewhere in it.
    sr, se = er.sum(axis=1), ee.sum(axis=1)
    if sr.size < se.size:
        sys.exit(f"recording is {sr.size / 100:.1f} s, shorter than the render's {se.size / 100:.1f} s")
    size = 1 << int(np.ceil(np.log2(sr.size + se.size)))
    corr = np.fft.irfft(np.fft.rfft(sr, size) * np.conj(np.fft.rfft(se, size)), size)[: sr.size - se.size + 1]
    lag = int(np.argmax(corr))
    er_all = er
    er = er[lag: lag + ee.shape[0]]
    # Levels: scale the recording to the render by the median ratio over loud windows.
    loud = ee.sum(axis=1) > ee.sum(axis=1).max() * 0.05
    scale = np.median(ee.sum(axis=1)[loud] / np.maximum(er.sum(axis=1)[loud], 1e-9))
    er = er * scale
    peak = ee.sum(axis=1).max()
    floor = peak * 10 ** (-40 / 20)
    active = ee.sum(axis=1) > floor
    evs = [(s, e) for s, e in events(active) if e - s >= 2]
    full = er_all * scale
    # Drift: align eight segments separately around the global lag and fit
    # a line through their offsets.
    pieces = 8
    span = ee.shape[0] // pieces
    xs, ys = [], []
    for k in range(pieces):
        seg = ee[k * span:(k + 1) * span].sum(axis=1)
        if seg.max() <= 0:
            continue
        best, off = -1.0, 0
        for d in range(-60, 61):
            a0 = lag + k * span + d
            if a0 < 0 or a0 + span > full.shape[0]:
                continue
            r = full[a0:a0 + span].sum(axis=1)
            c = float((r * seg).sum() / max(np.sqrt((r ** 2).sum() * (seg ** 2).sum()), 1e-9))
            if c > best:
                best, off = c, d
        xs.append(k * span + span / 2)
        ys.append(off)
    slope, intercept = np.polyfit(xs, ys, 1) if len(xs) >= 2 else (0.0, 0.0)

    def drift(t):
        return slope * t + intercept

    # The recording's envelope mapped onto the render's windows.
    index = lag + np.arange(ee.shape[0]) + np.rint(drift(np.arange(ee.shape[0]))).astype(int)
    index = np.clip(index, 0, full.shape[0] - 1)

    def recorded(t0, t1, d=0):
        return full[np.clip(index[t0:t1] + d, 0, full.shape[0] - 1)]

    er = recorded(0, ee.shape[0])
    problems = []
    levels = []
    shifts = []
    for s, e in evs:
        ex = ee[s:e]
        # Refine by +-4 windows on the event with 100 ms of context, by
        # normalised correlation (so a louder neighbour cannot win).
        p0, p1 = max(0, s - 10), min(ee.shape[0], e + 10)
        ctx = ee[p0:p1].sum(axis=1)
        best, shift = -2.0, 0
        for d in range(-4, 5):
            r = recorded(p0, p1, d).sum(axis=1)
            c = float(np.corrcoef(r, ctx)[0, 1]) if r.std() > 0 and ctx.std() > 0 else -1.0
            if c > best:
                best, shift = c, d
        shifts.append(shift)
        rc = recorded(s, e, shift)
        ex_e, rc_e = (ex ** 2).sum(), (rc ** 2).sum()
        if rc_e < ex_e * 10 ** (-12 / 10):
            problems.append(f"{s / 100:7.2f} s: missing or far too quiet ({10 * np.log10(max(rc_e, 1e-12) / ex_e):.1f} dB)")
            continue
        levels.append(10 * np.log10(rc_e / ex_e))
        ex_l = (ex[:, 0] ** 2).sum() / max(ex_e, 1e-12)
        rc_l = (rc[:, 0] ** 2).sum() / max(rc_e, 1e-12)
        if abs(ex_l - rc_l) > 0.15:
            problems.append(f"{s / 100:7.2f} s: left share {rc_l:.2f}, render {ex_l:.2f}")
        # Truncation: the sound lasts as long as the render's (within 30 ms),
        # measured to where it falls below -40 dB of its peak.
        thr = ex.sum(axis=1).max() * 0.01
        ex_loud = np.nonzero(ex.sum(axis=1) > thr)[0]
        rc_loud = np.nonzero(rc.sum(axis=1) > thr)[0]
        ex_len = int(ex_loud[-1] - ex_loud[0])
        rc_len = int(rc_loud[-1] - rc_loud[0]) if rc_loud.size else -1
        if abs(rc_len - ex_len) > 3:
            problems.append(f"{s / 100:7.2f} s: lasts {rc_len} windows, render {ex_len}")
    med = float(np.median(levels)) if levels else 0.0
    for (s, _), lv in zip([ev for ev in evs], levels):
        if abs(lv - med) > 3:
            problems.append(f"{s / 100:7.2f} s: level {lv - med:+.1f} dB against the median event")
    quiet = ~np.convolve(active, np.ones(7), "same").astype(bool)
    stray = er.sum(axis=1)[quiet] > floor * 4
    if stray.any():
        first = np.nonzero(quiet)[0][np.nonzero(stray)[0][0]]
        problems.append(f"{int(stray.sum())} windows sound where the render is silent (first at {first / 100:.2f} s)")
    corr_side = [np.corrcoef(ee[:, i], er[:, i])[0, 1] for i in range(2)]
    print(f"drift {slope * 1e4:+.1f} windows per 10,000 ({intercept:+.1f} at the start); "
          f"aligned at {lag / 100:.2f} s, local shifts {min(shifts):+d}..{max(shifts):+d} windows; {len(evs)} events; "
          f"envelope correlation L {corr_side[0]:.3f} R {corr_side[1]:.3f}; "
          f"event levels {min(levels) - med:+.1f}..{max(levels) - med:+.1f} dB around the median")
    if a.wav:
        seg = rec[lag * WINDOW: (lag + ee.shape[0]) * WINDOW]
        with wave.open(a.wav, "wb") as w:
            w.setnchannels(2); w.setsampwidth(2); w.setframerate(RATE); w.writeframes(seg.astype("<i2").tobytes())
    if problems or min(corr_side) < 0.9:
        for line in problems[:40]:
            t = float(line.split(" s:")[0]) if starts and " s:" in line else None
            if t is not None:
                near = [a for f, a in starts if abs(f / 50 - t) < 0.1]
                line += f"  (sound {', '.join(map(str, near)) or '?'})"
            print("  " + line)
        sys.exit(f"{len(problems)} problems; correlation {min(corr_side):.3f}")
    print("recording matches the render")


if __name__ == "__main__":
    main()
