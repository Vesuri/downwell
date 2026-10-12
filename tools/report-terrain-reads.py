#!/usr/bin/env python3
"""Read the big-endian diagnostic API-read census, not frame timings.

Counts are call/visit counts at instrumented API boundaries; nested queries
overlap. Column 1 is use outside construction, column 2 inside construction.
Zero means unobserved in this no-input route, not unneeded in the game.
"""
import argparse
import struct
from pathlib import Path

NAMES = ('identity query', 'run publication query', 'room presence query',
         'room active query', 'room animation skipped run', 'room reconcile run',
         'room cleanup visited run', 'layer state query', 'inactive cutoff query',
         'flags query', 'sprite query', 'wall-state query', 'thin retirement proof',
         'external registration query', 'generic table slot query',
         'prepared segments', 'prepared cells')


def read(path):
    data = Path(path).read_bytes()
    count = 165 * 2 * len(NAMES)
    if len(data) != count * 4:
        raise ValueError('incomplete terrain census')
    words = struct.unpack('>' + 'I' * count, data)
    width = 2 * len(NAMES)
    samples = [words[i:i+width] for i in range(0, count, width)]
    if any(samples[0]):
        raise ValueError('startup leaked into update census')
    if any(b < a for old, new in zip(samples, samples[1:]) for a, b in zip(old, new)):
        raise ValueError('census counters decreased')
    # Identity reads may genuinely be absent outside construction. Use the
    # per-update retirement proof as the consumer-path landmark, not a query
    # whose absence the census is intended to discover.
    if (not samples[-1][len(NAMES)+15] or
            not samples[-1][len(NAMES)] or samples[-1][12] != 164):
        raise ValueError('missing construction/read positive controls')
    return samples


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture', type=Path)
    args = parser.parse_args()
    samples = read(args.capture)
    for name, first, last in [('whole fall', 1, 164), ('entrance', 1, 55),
                             ('quiet generation', 56, 121), ('first burst', 122, 135),
                             ('enemies', 136, 162), ('landing', 163, 164)]:
        print(f'\n{name}: updates {first}–{last}\n')
        print('| API boundary | consumer | construction |\n| --- | ---: | ---: |')
        for i, label in enumerate(NAMES):
            delta = [samples[last][j]-samples[first-1][j] for j in (i, i+len(NAMES))]
            print(f'| {label} | {delta[0]} | {delta[1]} |')


if __name__ == '__main__':
    main()
