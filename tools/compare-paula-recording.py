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

Render events closer than 30 ms are one sound (a decaying tail that dips
under -40 dB and rises again is not a faint event of its own).

The emulator's real-time capture drifts and drops whole 60 ms buffers, at
times one every 2-3 s (25 in a 94 s run on 2026-11-22, 2 in the next). So
the alignment follows the recording event by event (each event searched
around the previous one's offset) and reports the drops (steps of 40 ms or
more) and drift steps it crossed:
- A missing, truncated or quiet event next to a drop is reported as capture
  loss, not failed. A sound lost whole in a drop shows its step at the
  following event, so missing sounds look one event further.
- Sound in the render's silence fails unless it is silent under the offset of
  the event before or the one after (a drop between them may sit anywhere);
  a stuck voice sounds under both.
- More than 3% of the render lost in drops fails.
Capture loss can hide a sound, so when a run excuses some, record again: each
excused sound must match in another run. The whole-run envelope correlation
per side is only a coarse guard (at least 0.9); the per-event checks are the
criteria.

--wav also writes the aligned recording as a WAV to listen to.
"""
import argparse
import sys
import wave

import numpy as np

RATE = 44100
WINDOW = 441  # 10 ms
TRACK = 8     # an event's offset search around the previous one (a 60 ms gap is 6), in windows
DROP = 4      # an offset step this large between events is a dropped capture buffer
MERGE = 3     # render events closer than this (in windows) are one sound
MAX_LOSS = 0.03  # more lost capture than this and the recording proves nothing


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
    if sr.size < se.size + 1000:
        # Dropped buffers can leave less than boot plus render: pad with
        # silence, so a lost tail fails as missing sound.
        print(f"recording is {sr.size / 100:.1f} s against the render's {se.size / 100:.1f} s; padding")
        er = np.vstack([er, np.zeros((se.size + 1000 - sr.size, 2))])
        sr = er.sum(axis=1)
    size = 1 << int(np.ceil(np.log2(sr.size + se.size)))
    corr = np.fft.irfft(np.fft.rfft(sr, size) * np.conj(np.fft.rfft(se, size)), size)[: sr.size - se.size + 1]
    lag = int(np.argmax(corr))
    # Capture gaps smear the whole-run peak: anchor on the opening 10 s
    # alone (normalised, so loud later material cannot win), near the end of
    # boot; event tracking takes over from there.
    head = se[:1000]
    best = -2.0
    for d in range(0, sr.size - head.size):
        r = sr[d:d + head.size]
        if r.std() == 0:
            continue
        c = float(np.corrcoef(r, head)[0, 1])
        if c > best:
            best, lag = c, d
    # Dropped buffers shorten the recording: pad its envelope with silence
    # so a lost tail fails as missing sound instead of misaligning.
    need = lag + ee.shape[0] + TRACK + 60
    if er.shape[0] < need:
        er = np.vstack([er, np.zeros((need - er.shape[0], 2))])
    er_all = er
    er = er[lag: lag + ee.shape[0]]
    # Levels: scale the recording to the render by the median ratio over loud windows.
    loud = ee.sum(axis=1) > ee.sum(axis=1).max() * 0.05
    scale = np.median(ee.sum(axis=1)[loud] / np.maximum(er.sum(axis=1)[loud], 1e-9))
    er = er * scale
    peak = ee.sum(axis=1).max()
    floor = peak * 10 ** (-40 / 20)
    active = ee.sum(axis=1) > floor
    # A decaying tail can dip under -40 dB for a window or two and rise
    # again: one sound, not a faint second event whose level is noise.
    merged = []
    for s, e in events(active):
        if merged and s - merged[-1][1] < MERGE:
            merged[-1] = (merged[-1][0], e)
        else:
            merged.append((s, e))
    evs = [(s, e) for s, e in merged if e - s >= 2]
    full = er_all * scale
    # Follow the recording event by event: FS-UAE's real-time capture drifts
    # slightly and, on a busy host, drops whole buffers (fixed 60 ms steps)
    # between sounds. Each event is aligned by normalised correlation over
    # the event with 100 ms of context, searched around the previous event's
    # offset, and the windows up to the next event use its offset.
    def corr(d, p0, p1, ctx):
        # Both sides in one vector, so a neighbour on the other side loses.
        idx = np.clip(lag + np.arange(p0, p1) + d, 0, full.shape[0] - 1)
        r = full[idx].T.ravel()
        return float(np.corrcoef(r, ctx)[0, 1]) if r.std() > 0 and ctx.std() > 0 else -1.0
    offsets, prev = [], 0
    for s, e in evs:
        p0, p1 = max(0, s - 10), min(ee.shape[0], e + 10)
        ctx = ee[p0:p1].T.ravel()
        best, off = -2.0, prev
        for d in range(prev - TRACK, prev + TRACK + 1):
            c = corr(d, p0, p1, ctx)
            if c > best:
                best, off = c, d
        offsets.append(off)
        prev = off
    shift_at = np.zeros(ee.shape[0], dtype=int)
    for (s, _), off in zip(evs, offsets):
        shift_at[s:] = off
    if evs:
        shift_at[:evs[0][0]] = offsets[0]
    gaps = [b - a for a, b in zip(offsets, offsets[1:]) if b != a]
    index = np.clip(lag + np.arange(ee.shape[0]) + shift_at, 0, full.shape[0] - 1)

    def recorded(t0, t1, d=0):
        return full[np.clip(index[t0:t1] + d, 0, full.shape[0] - 1)]

    er = recorded(0, ee.shape[0])
    problems = []
    levels = []
    shifts = []
    steps = [b - a for a, b in zip(offsets, offsets[1:])]

    def dropped(i, ahead=1):
        # A buffer dropped between the previous event's start and the next
        # one's. A sound lost whole in a dropped buffer leaves nothing to
        # track, so that step shows at the following event: look one further.
        return any(0 <= j < len(steps) and abs(steps[j]) >= DROP for j in range(i - 1, i + ahead))

    capture = []  # problems next to a dropped buffer: capture loss, not the mixer
    for i, (s, e) in enumerate(evs):
        problems_before = len(problems)
        ex = ee[s:e]
        # Refine by +-4 windows on the event with 100 ms of context, by
        # normalised correlation (so a louder neighbour cannot win).
        p0, p1 = max(0, s - 10), min(ee.shape[0], e + 10)
        ctx = ee[p0:p1].sum(axis=1)
        best, shift = -2.0, 0
        for d in range(-2, 3):
            r = recorded(p0, p1, d).sum(axis=1)
            c = float(np.corrcoef(r, ctx)[0, 1]) if r.std() > 0 and ctx.std() > 0 else -1.0
            if c > best:
                best, shift = c, d
        shifts.append(shift)
        rc = recorded(s, e, shift)
        ex_e, rc_e = (ex ** 2).sum(), (rc ** 2).sum()
        if rc_e < ex_e * 10 ** (-12 / 10):
            problems.append(f"{s / 100:7.2f} s: missing or far too quiet ({10 * np.log10(max(rc_e, 1e-12) / ex_e):.1f} dB)")
            if dropped(i, 2):
                capture.append(problems.pop())
            continue
        levels.append((i, 10 * np.log10(rc_e / ex_e)))
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
        if dropped(i):
            capture.extend(problems[problems_before:])
            del problems[problems_before:]
    med = float(np.median([lv for _, lv in levels])) if levels else 0.0
    for i, lv in levels:
        if abs(lv - med) > 3:
            line = f"{evs[i][0] / 100:7.2f} s: level {lv - med:+.1f} dB against the median event"
            (capture if dropped(i) else problems).append(line)
    quiet = ~np.convolve(active, np.ones(7), "same").astype(bool)
    # A buffer dropped somewhere between two events: the silence before the
    # next one may follow either offset. Sound under both is stray, whether or
    # not a buffer dropped there (a stuck voice sounds under any offset).
    starts_at = np.array([s for s, _ in evs])
    after = np.searchsorted(starts_at, np.arange(ee.shape[0]), side="right")  # next event's index
    nxt = np.array(offsets + [offsets[-1]])[np.minimum(after, len(offsets) - 1)] if evs else np.zeros(ee.shape[0], int)
    alt = full[np.clip(lag + np.arange(ee.shape[0]) + nxt, 0, full.shape[0] - 1)]
    loud = (er.sum(axis=1) > floor * 4) & (alt.sum(axis=1) > floor * 4) & quiet
    if loud.any():
        problems.append(f"{int(loud.sum())} windows sound where the render is silent (first at {np.nonzero(loud)[0][0] / 100:.2f} s)")
    lost = -sum(x for x in steps if x <= -DROP) / max(1, ee.shape[0])
    if lost > MAX_LOSS:
        problems.append(f"capture lost {100 * lost:.1f}% in dropped buffers (more than {100 * MAX_LOSS:.0f}%)")
    corr_side = [np.corrcoef(ee[:, i], er[:, i])[0, 1] for i in range(2)]
    drops = sum(1 for x in steps if abs(x) >= DROP)
    print(f"aligned at {lag / 100:.2f} s; {drops} dropped capture buffers and "
          f"{len(gaps) - drops} drift steps between events "
          f"(net {offsets[-1] - offsets[0]:+d} windows); local shifts {min(shifts):+d}..{max(shifts):+d}; {len(evs)} events; "
          f"envelope correlation L {corr_side[0]:.3f} R {corr_side[1]:.3f}; "
          f"event levels {min(lv for _, lv in levels) - med:+.1f}..{max(lv for _, lv in levels) - med:+.1f} dB around the median")
    if a.wav:
        seg = rec[lag * WINDOW: (lag + ee.shape[0]) * WINDOW]
        with wave.open(a.wav, "wb") as w:
            w.setnchannels(2); w.setsampwidth(2); w.setframerate(RATE); w.writeframes(seg.astype("<i2").tobytes())
    def show(lines, prefix):
        for line in lines[:40]:
            t = float(line.split(" s:")[0]) if starts and " s:" in line else None
            if t is not None:
                named = [a for f, a in starts if abs(f / 50 - t) < 0.1]
                line += f"  (sound {', '.join(map(str, named)) or '?'})"
            print(prefix + line)
    if capture:
        print(f"{len(capture)} differences next to a dropped capture buffer (capture loss, not the mixer):")
        show(capture, "  capture: ")
    if problems or min(corr_side) < 0.9:
        show(problems, "  ")
        sys.exit(f"{len(problems)} problems; correlation {min(corr_side):.3f}")
    print("recording matches the render" + (" apart from capture loss" if capture else ""))


if __name__ == "__main__":
    main()
