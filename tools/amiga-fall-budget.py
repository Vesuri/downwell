#!/usr/bin/env python3
"""Summarise the no-input initial fall against the 20 ms update budget.

Input is one or more reports written by tools/amiga-trace-report.py for
overlapping captures of the same build (see docs/open-work.md section 0).
Prefer JSON from --work-summary with an explicitly verified --work-wait:
it retains unrounded cycles and waits below the displayed top-six cutoff.
The named wait scope includes all its children, including idle RNG service.
Do not mix exact summaries and historical text estimates for an A/B claim.
Each report's per-frame table is read; the first report that covers a frame
wins. An update's work is its marker-to-marker time minus the publication
wait (`runDownwell()::{lambda#9}`, AmigaPlatform::waitPublished) when the
table lists it among the frame's largest parts; an unlisted wait is under
the smallest listed part and is counted as work.

  python3 tools/amiga-fall-budget.py report-1.txt report-60.txt report-120.txt
      [--frames] [--window=NAME:FIRST-LAST ...]
"""
import argparse
import json
import re
import sys

WAIT = 'runDownwell()::{lambda#9}'
ROW = re.compile(r'\| (\d+) \| ([\d.]+) \| [\d.]+ \| (.*) \|$')
WINDOWS = [
    ('entrance 2-55', 2, 55),
    ('quiet generation 56-121', 56, 121),
    ('first burst 122-135', 122, 135),
    ('enemies 136-162', 136, 162),
    ('landing 163-164', 163, 164),
    ('whole fall 2-164', 2, 164),
]


def parse(paths):
    frames = {}
    for path in paths:
        with open(path) as report:
            content = report.read()
            if content.lstrip().startswith('{'):
                data = json.loads(content)
                if (data.get('schema') != 'downwell-frame-work/v1' or
                        data.get('pal_field_cycles') != 142102 or data.get('partial_frames') or
                        not data.get('wait_symbol')):
                    raise ValueError('invalid complete-frame work summary: ' + str(path))
                seen = set()
                for row in data['frames']:
                    f, total, wait = row['frame'], row['cycles'], row['wait_cycles']
                    if (any(type(v) is not int for v in (f, total, wait)) or
                            f < 1 or total <= 0 or not 0 <= wait <= total or f in seen):
                        raise ValueError('invalid or duplicate frame in ' + str(path))
                    seen.add(f)
                    parts = row['parts_cycles']
                    if (any(type(v) is not int or v < 0 for v in parts.values()) or
                            sum(parts.values()) != total):
                        raise ValueError('frame parts do not account for total in ' + str(path))
                    scale = 20 / 142102
                    frames.setdefault(f, (total * scale, (total - wait) * scale,
                                          {n: v * scale for n, v in parts.items()}))
                continue
            for line in content.splitlines():
                match = ROW.match(line)
                if not match or int(match[1]) in frames:
                    continue
                parts = {}
                for part in match[3].split(', '):
                    name, _, value = part.rpartition(' ')
                    parts[name] = float(value)
                total = float(match[2])
                frames[int(match[1])] = (total, total - parts.get(WAIT, 0.0), parts)
    return frames


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('reports', nargs='+')
    parser.add_argument('--frames', action='store_true', help='also list every update')
    parser.add_argument('--window', action='append', default=[], help='NAME:FIRST-LAST, replaces the defaults')
    args = parser.parse_args()
    frames = parse(args.reports)
    if not frames:
        sys.exit('no per-frame tables found')
    windows = WINDOWS
    if args.window:
        windows = []
        for spec in args.window:
            name, _, span = spec.rpartition(':')
            first, _, last = span.partition('-')
            windows.append((name or span, int(first), int(last or first)))
    print('| window | updates | missing | work mean ms | worst ms (update) | over 20 ms | excess ms |')
    print('| --- | ---: | ---: | ---: | ---: | ---: | ---: |')
    for name, first, last in windows:
        present = [f for f in range(first, last + 1) if f in frames]
        if not present:
            continue
        work = [frames[f][1] for f in present]
        worst = max(present, key=lambda f: frames[f][1])
        over = sum(1 for w in work if w > 20.0)
        excess = sum(w - 20.0 for w in work if w > 20.0)
        print(f'| {name} | {len(present)} | {last - first + 1 - len(present)} | {sum(work) / len(work):.1f} | '
              f'{frames[worst][1]:.1f} ({worst}) | {over} | {excess:.0f} |')
    if args.frames:
        print('\n| update | total ms | work ms | Step ms | new-row paint ms |')
        print('| ---: | ---: | ---: | ---: | ---: |')
        for f in sorted(frames):
            total, work, parts = frames[f]
            print(f'| {f} | {total:.1f} | {work:.1f} | {parts.get("GameplayWorld::step", 0.0):.1f} | '
                  f'{parts.get("drawMainWorldSpan", 0.0):.1f} |')


if __name__ == '__main__':
    main()
