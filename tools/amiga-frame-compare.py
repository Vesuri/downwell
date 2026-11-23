#!/usr/bin/env python3
"""Compare explicitly identical complete-frame windows, never capture duration.

A frame summary compares whole frames, publication wait included. A work
summary (--work-summary) compares each frame's work: its cycles minus the
named wait. Both summaries must be of the same kind, wait and CPU clock.

Inputs are --frame-summary JSON files from amiga-trace-report.py. Callers must
separately establish identical input/RNG workloads and behavior. Matching frame
numbers alone does not prove that two revisions performed equivalent work.
"""
import argparse
import json
import re


def load_summary(path):
    with open(path) as source:
        value = json.load(source)
    schema = value.get('schema')
    if schema not in ('downwell-frame-cycles/v1', 'downwell-frame-work/v1'):
        raise ValueError(f'{path}: unsupported frame summary')
    wait = value.get('wait_symbol') if schema == 'downwell-frame-work/v1' else None
    if schema == 'downwell-frame-work/v1' and not wait:
        raise ValueError(f'{path}: work summary names no wait')
    clock = value.get('pal_field_cycles')
    if type(clock) is not int or clock <= 0:
        raise ValueError(f'{path}: missing PAL field clock')
    if not re.fullmatch(r'[0-9a-f]{64}', value.get('elf_sha256', '')):
        raise ValueError(f'{path}: missing ELF identity')
    partial = set(value.get('partial_frames', []))
    frames = Frames()
    for row in value['frames']:
        frame, cycles = row['frame'], row['cycles']
        if (type(frame) is not int or frame <= 0 or frame in frames or frame in partial or
                type(cycles) is not int or cycles <= 0):
            raise ValueError(f'{path}: invalid, duplicate or partial frame {frame}')
        if wait is not None:
            waited = row.get('wait_cycles')
            if type(waited) is not int or not 0 <= waited < cycles:
                raise ValueError(f'{path}: invalid wait in frame {frame}')
            cycles -= waited
        frames[frame] = cycles
    frames.clock = clock
    frames.wait = wait
    return value['elf_sha256'], frames


class Frames(dict):
    """Complete-frame cycles (or work) with the capture's PAL field length."""
    clock = None
    wait = None


def windows(spec):
    result = []
    for item in spec.split(','):
        match = re.fullmatch(r'([1-9][0-9]*)(?:-([1-9][0-9]*))?', item)
        if not match:
            raise ValueError(f'invalid frame window: {item}')
        first = int(match[1])
        last = int(match[2] or match[1])
        if last < first:
            raise ValueError(f'reversed frame window: {item}')
        result.append((first, last))
    return result


def compare(before, after, ranges):
    if before.clock != after.clock:
        raise ValueError(f'summaries from different CPU clocks: {before.clock} and {after.clock} '
                         'cycles per field; compare captures of the same machine')
    if before.wait != after.wait:
        raise ValueError(f'summaries measure different things: wait {before.wait} and {after.wait}')
    result = []
    for first, last in ranges:
        keys = range(first, last + 1)
        for label, frames in (('before', before), ('after', after)):
            missing = [f for f in keys if f not in frames]
            if missing:
                raise ValueError(f'{label}: missing complete frames {missing}; capture a longer window')
        scale = 20 / before.clock
        a, b = [before[f] for f in keys], [after[f] for f in keys]
        result.append((first, last, sum(a) / len(a) * scale, sum(b) / len(b) * scale,
                       max(a) * scale, max(b) * scale,
                       max((after[f] - before[f]) * scale for f in keys)))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('before')
    parser.add_argument('after')
    parser.add_argument('--frames', required=True, help='explicit windows, e.g. 51-70,676-681,1047')
    args = parser.parse_args()
    try:
        old_hash, before = load_summary(args.before)
        new_hash, after = load_summary(args.after)
        rows = compare(before, after, windows(args.frames))
    except (ValueError, KeyError, OSError) as error:
        parser.error(str(error))
    print(f'Before ELF: `{old_hash}`\n\nAfter ELF: `{new_hash}`\n')
    print('Same frame numbers are compared; input/RNG and behavior equivalence require separate evidence.\n')
    print('| Frames | Before mean ms | After mean ms | Before worst ms | After worst ms | Largest matched delta ms |')
    print('| --- | ---: | ---: | ---: | ---: | ---: |')
    for first, last, *values in rows:
        print(f'| {first}-{last} | ' + ' | '.join(f'{v:.4f}' for v in values) + ' |')


if __name__ == '__main__':
    main()
