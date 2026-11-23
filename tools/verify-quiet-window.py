#!/usr/bin/env python3
"""Guard the no-input fall's quiet window against its frozen baseline.

Call only for profile.sh quiet-window's deterministic workload. Frame numbers
and an ELF hash cannot independently prove equivalent inputs or game behavior.
Each summary given is checked against the baseline of its CPU clock and kind:
the A500's whole frames (its updates overran the field, so frame time was
work) and the A1200 target's work, where every quiet frame fills its field
and only work, the frame minus the publication wait, can show a regression.
A machine with no baseline is not judged.
"""
import argparse
import hashlib
import importlib.util
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('frame_compare', ROOT / 'tools/amiga-frame-compare.py')
frames = importlib.util.module_from_spec(spec)
spec.loader.exec_module(frames)


BASELINES = (ROOT / 'docs/amiga-quiet-frame-baseline.json',
             ROOT / 'docs/amiga-quiet-work-baseline-a1200-4m.json')


def matching_baseline(summary, baselines=BASELINES):
    _, frames_ = frames.load_summary(summary)
    for baseline in baselines:
        if not baseline.exists():
            continue
        _, reference = frames.load_summary(baseline)
        if (reference.clock, reference.wait) == (frames_.clock, frames_.wait):
            return baseline
    return None


def verify(baseline, summary, elf):
    _, before = frames.load_summary(baseline)
    identity, after = frames.load_summary(summary)
    if hashlib.sha256(Path(elf).read_bytes()).hexdigest() != identity:
        raise ValueError('summary ELF identity differs from the frozen executable')
    # Check complete windows, not a selected subset or the tape's maximum gap.
    quiet, generation = frames.compare(before, after, [(57, 121), (122, 122)])
    failures = []
    if quiet[3] > quiet[2] + 0.5:
        failures.append(f'quiet mean {quiet[3]:.3f} ms exceeds {quiet[2] + 0.5:.3f} ms')
    if quiet[5] > quiet[4] + 1.0:
        failures.append(f'quiet worst {quiet[5]:.3f} ms exceeds {quiet[4] + 1.0:.3f} ms')
    if generation[3] > generation[2] + 1.0:
        failures.append(f'generation 122 {generation[3]:.3f} ms exceeds {generation[2] + 1.0:.3f} ms')
    if failures:
        raise ValueError('; '.join(failures))
    return quiet[3], quiet[5], generation[3]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('summaries', type=Path, nargs='+', help='frame and/or work summaries')
    parser.add_argument('elf', type=Path)
    parser.add_argument('--baseline', type=Path, help='use this baseline for every summary')
    args = parser.parse_args()
    judged = 0
    try:
        for summary in args.summaries:
            baseline = args.baseline or matching_baseline(summary)
            if baseline is None:
                continue
            mean, worst, generation = verify(baseline, summary, args.elf)
            judged += 1
            print(f'QUIET_WINDOW_PASS baseline={baseline.name} mean_ms={mean:.3f} '
                  f'worst_ms={worst:.3f} generation122_ms={generation:.3f}')
    except (ValueError, KeyError, OSError) as error:
        parser.error(str(error))
    if not judged:
        print('QUIET_WINDOW_NOT_JUDGED: no baseline for this CPU clock')
    print('Regression guard only; replay equivalence remains a separate check.')


if __name__ == '__main__':
    main()
