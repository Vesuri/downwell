#!/usr/bin/env python3
"""Summarise the W3.15 blit census: blits started per update, by kind.

Input is a census directory from `CENSUS=1 BLIT_CENSUS=1 bash amiga/profile.sh
MODE` (census.bin, blit-census.bin, capture.log). Each update's blit record
sits at the same ring index as its census record. A twin is a blit that
repeats the previous blit's kind and size one plane further on: the second
plane's blit of a non-interleaved bitmap, which one interleaved blit covers.
It also reports the blitter's busy time, the time it sat finished with blits
still queued (the queue is polled), and the CPU's drain waits.

  python3 tools/amiga-blit-census-report.py tmp/census/DIR [--class=gameplay]
"""
import argparse
import importlib.util
import os
import struct

KINDS = ['clear', 'copy', 'masked copy', 'masked copy + background', 'line', 'fill',
         'solid rectangle', 'combine', 'pattern']
TWINS = ['gameplay ring stride', 'display/HUD stride', 'other stride']
RECORD = struct.Struct('>9I3I3I9H3HHH')


def census_module():
    path = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'amiga-census-report.py')
    spec = importlib.util.spec_from_file_location('census_report', path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('directory')
    parser.add_argument('--class', dest='classes', action='append', default=[],
                        help='update classes to report (default: every class present)')
    args = parser.parse_args()
    census = census_module()
    with open(os.path.join(args.directory, 'capture.log')) as log:
        loaded = census.load(os.path.join(args.directory, 'census.bin'), log.read())
    with open(os.path.join(args.directory, 'blit-census.bin'), 'rb') as source:
        ring = source.read()
    capacity = loaded['capacity']
    if len(ring) != capacity * RECORD.size:
        raise SystemExit(f'blit ring is {len(ring)} bytes, not {capacity} records')
    groups = {}
    for record in loaded['records']:
        fields = RECORD.unpack_from(ring, (record['index'] % capacity) * RECORD.size)
        blit = dict(words=fields[0:9], twin_words=fields[9:12], busy=fields[12], idle=fields[13],
                    drain=fields[14], blits=fields[15:24], twins=fields[24:27], queued=fields[27])
        groups.setdefault(record['class'], []).append((record, blit))
    for name in args.classes or census.CLASSES:
        rows = groups.get(name)
        if not rows:
            continue
        count = len(rows)
        total = sum(sum(b['blits']) for _, b in rows)
        twins = [sum(b['twins'][i] for _, b in rows) for i in range(3)]
        worst = max(rows, key=lambda row: row[0]['work_ms'])
        busiest = max(rows, key=lambda row: sum(row[1]['blits']))
        print(f'## {name}: {count} updates')
        print(f'blits/update {total / count:.1f}, queued {sum(b["queued"] for _, b in rows) / count:.1f}, '
              f'twins/update {sum(twins) / count:.1f} '
              f'({", ".join(f"{TWINS[i]} {twins[i] / count:.1f}" for i in range(3))})')
        print(f'most blits: {sum(busiest[1]["blits"])} ({sum(busiest[1]["twins"])} twins) at frame '
              f'{busiest[0]["frame"]}, {busiest[0]["work_ms"]:.1f} ms; worst update {worst[0]["work_ms"]:.1f} ms '
              f'at frame {worst[0]["frame"]}: {sum(worst[1]["blits"])} blits, {sum(worst[1]["twins"])} twins')
        ms = 20.0 / loaded['ticks_per_field']
        timed = lambda key: sum(b[key] for _, b in rows) * ms / count
        print(f'blitter busy {timed("busy"):.2f} ms/update, idle with blits queued '
              f'{timed("idle"):.2f} ms, CPU drain waits {timed("drain"):.2f} ms; worst update: busy '
              f'{worst[1]["busy"] * ms:.2f}, idle {worst[1]["idle"] * ms:.2f}, drain {worst[1]["drain"] * ms:.2f}')
        idlest = max(rows, key=lambda row: row[1]['idle'])
        print(f'most idle: {idlest[1]["idle"] * ms:.2f} ms at frame {idlest[0]["frame"]} '
              f'({idlest[0]["work_ms"]:.1f} ms update, drain {idlest[1]["drain"] * ms:.2f})')
        print('| kind | blits/update | words/update | words/blit |')
        print('| --- | ---: | ---: | ---: |')
        for k, kind in enumerate(KINDS):
            blits = sum(b['blits'][k] for _, b in rows)
            words = sum(b['words'][k] for _, b in rows)
            if blits:
                print(f'| {kind} | {blits / count:.1f} | {words / count:,.0f} | {words / blits:.0f} |')
        print()


if __name__ == '__main__':
    main()
