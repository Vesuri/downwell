#!/usr/bin/env python3
"""Guard the recovered no-input fall, not the unfinished historical target.

Call only for profile.sh quiet-window's deterministic workload. Frame numbers
and an ELF hash cannot independently prove equivalent inputs or game behavior.
"""
import argparse
import hashlib
import importlib.util
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('frame_compare', ROOT / 'tools/amiga-frame-compare.py')
frames = importlib.util.module_from_spec(spec)
spec.loader.exec_module(frames)


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
    parser.add_argument('summary', type=Path)
    parser.add_argument('elf', type=Path)
    parser.add_argument('--baseline', type=Path, default=ROOT / 'docs/amiga-quiet-frame-baseline.json')
    args = parser.parse_args()
    try:
        mean, worst, generation = verify(args.baseline, args.summary, args.elf)
    except (ValueError, KeyError, OSError) as error:
        parser.error(str(error))
    print(f'QUIET_WINDOW_PASS mean_ms={mean:.3f} worst_ms={worst:.3f} generation122_ms={generation:.3f}')
    print('Regression guard only; historical fall restoration and replay equivalence remain separate checks.')


if __name__ == '__main__':
    main()
