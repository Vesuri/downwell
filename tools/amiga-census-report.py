#!/usr/bin/env python3
"""Summarise a per-update work census (W3.5) against the 20 ms update budget.

Input is the ring a CENSUS=1 profile build keeps (`CENSUS=1 bash
amiga/profile.sh MODE` writes census.bin and capture.log). Each main-loop
iteration has one record, mark to mark: its span, its work (the span minus
every display wait, so it means the same on every CPU), its scene, room
speed, run stage and flags. Ticks are beam positions, 313 x 256 per PAL field.

Updates are judged in separate classes: gameplay play, the deliberate
slowdowns (room speed below 60, sleeps), scene and level handoffs (with the
first gameplay update), the clear/results/upgrade overlays, side-room camera
jumps and the front end. Handoffs are judged by total latency, not per update.

  python3 tools/amiga-census-report.py census.bin capture.log
      [--frames=N-M] [--list=over|all] [--json=FILE]
"""
import argparse
import json
import re
import struct
import sys

RECORD = struct.Struct('>IIIBBBB')
SCENES = {0: 'splash', 1: 'menu', 2: 'style select', 3: 'gameplay'}
SLOWED, DELIBERATE, HANDOFF, OVERLAY, SIDE_CAMERA, WARMUP = 1, 2, 4, 8, 16, 32
FLAG_NAMES = [(SLOWED, 'slowed'), (DELIBERATE, 'deliberate'), (HANDOFF, 'handoff'),
              (OVERLAY, 'overlay'), (SIDE_CAMERA, 'side-camera'), (WARMUP, 'warm-up')]
CLASSES = ['gameplay', 'slowdown', 'handoff', 'overlay', 'camera jump', 'front end']
BUDGET_MS = 20.0
HEADER = re.compile(r'^CENSUS count=(\d+) capacity=(\d+) ticks_per_field=(\d+)$', re.M)


def classify(record):
    if record['flags'] & (HANDOFF | WARMUP):
        return 'handoff'
    if record['scene'] != 3:
        return 'front end'
    if record['flags'] & OVERLAY:
        return 'overlay'
    if record['flags'] & (SLOWED | DELIBERATE):
        return 'slowdown'
    if record['flags'] & SIDE_CAMERA:
        return 'camera jump'
    return 'gameplay'


def load(ring_path, log_text):
    match = HEADER.search(log_text)
    if not match:
        raise ValueError('capture log has no CENSUS line')
    count, capacity, ticks = (int(v) for v in match.groups())
    with open(ring_path, 'rb') as source:
        data = source.read()
    if capacity <= 0 or len(data) != capacity * RECORD.size or ticks <= 0:
        raise ValueError(f'census ring is {len(data)} bytes, not {capacity} records')
    kept = min(count, capacity)
    first = count - kept
    records = []
    for index in range(first, count):
        work, span, frame, scene, speed, flags, stage = RECORD.unpack_from(
            data, (index % capacity) * RECORD.size)
        if work > span:
            raise ValueError(f'update {index}: work {work} exceeds span {span}')
        record = dict(index=index, frame=frame, scene=scene, room_speed=speed, flags=flags,
                      area=stage // 4, level=stage % 4,
                      work_ms=work * BUDGET_MS / ticks, span_ms=span * BUDGET_MS / ticks)
        record['class'] = classify(record)
        records.append(record)
    return dict(count=count, capacity=capacity, ticks_per_field=ticks,
                overwritten=first, records=records)


def summarise(records):
    work = [r['work_ms'] for r in records]
    over = [w for w in work if w > BUDGET_MS]
    worst = max(records, key=lambda r: r['work_ms'])
    return dict(updates=len(records), work_mean_ms=sum(work) / len(work),
                worst_ms=worst['work_ms'], worst_frame=worst['frame'], worst_index=worst['index'],
                over_budget=len(over), excess_ms=sum(w - BUDGET_MS for w in over))


def place(record):
    scene = SCENES.get(record['scene'], str(record['scene']))
    return f'{scene} {record["area"]}-{record["level"]}' if record['scene'] == 3 else scene


def handoffs(records):
    """Consecutive handoff updates as one transition, judged by its latency."""
    result, run = [], []
    for record in records + [None]:
        if record is not None and record['class'] == 'handoff' and (
                not run or record['index'] == run[-1]['index'] + 1):
            run.append(record)
            continue
        if run:
            following = record if record is not None and record['index'] == run[-1]['index'] + 1 else run[-1]
            result.append(dict(first_index=run[0]['index'], updates=len(run),
                               latency_ms=sum(r['span_ms'] for r in run),
                               work_ms=sum(r['work_ms'] for r in run),
                               worst_ms=max(r['work_ms'] for r in run),
                               **{'from': place(run[0]), 'to': place(following)}))
            run = []
        if record is not None and record['class'] == 'handoff':
            run.append(record)
    return result


def flag_text(flags):
    return ','.join(name for bit, name in FLAG_NAMES if flags & bit) or '-'


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('ring')
    parser.add_argument('log')
    parser.add_argument('--frames', help='only gameplay frames N or N-M (FPS/trace numbering)')
    parser.add_argument('--list', choices=('over', 'all'), help='also list updates')
    parser.add_argument('--json', help='write the summary and every record')
    args = parser.parse_args()
    try:
        with open(args.log) as source:
            census = load(args.ring, source.read())
    except (ValueError, OSError) as error:
        sys.exit(f'census rejected: {error}')
    records = census['records']
    if args.frames:
        match = re.fullmatch(r'([1-9][0-9]*)(?:-([1-9][0-9]*))?', args.frames)
        if not match:
            sys.exit('--frames must be N or N-M')
        first, last = int(match[1]), int(match[2] or match[1])
        records = [r for r in records if r['scene'] == 3 and not r['flags'] & WARMUP
                   and first <= r['frame'] <= last]
        missing = sorted(set(range(first, last + 1)) - {r['frame'] for r in records})
        if missing:
            sys.exit(f'census lacks gameplay frames {missing[:10]}')
    if not records:
        sys.exit('census holds no updates')
    print(f'# Work census: {census["count"]} updates recorded, ring of {census["capacity"]}'
          + (f'; the first {census["overwritten"]} were overwritten' if census['overwritten'] else ''))
    print(f'Work = mark-to-mark time minus display waits; 20 ms = {census["ticks_per_field"]} beam ticks.\n')
    print('| class | updates | work mean ms | worst ms (frame) | over 20 ms | excess ms |')
    print('| --- | ---: | ---: | ---: | ---: | ---: |')
    summary = {}
    for name in CLASSES:
        group = [r for r in records if r['class'] == name]
        if not group:
            continue
        s = summary[name] = summarise(group)
        print(f'| {name} | {s["updates"]} | {s["work_mean_ms"]:.1f} | {s["worst_ms"]:.1f} '
              f'({s["worst_frame"]}) | {s["over_budget"]} | {s["excess_ms"]:.0f} |')
    transitions = handoffs(records)
    if transitions:
        print('\n| handoff | updates | latency ms | work ms | worst update ms |')
        print('| --- | ---: | ---: | ---: | ---: |')
        for t in transitions:
            print(f'| {t["from"]} to {t["to"]} (index {t["first_index"]}) | {t["updates"]} | '
                  f'{t["latency_ms"]:.1f} | {t["work_ms"]:.1f} | {t["worst_ms"]:.1f} |')
    if args.list:
        print('\n| index | scene | frame | stage | speed | flags | work ms | span ms |')
        print('| ---: | --- | ---: | --- | ---: | --- | ---: | ---: |')
        for r in records:
            if args.list == 'over' and r['work_ms'] <= BUDGET_MS:
                continue
            print(f'| {r["index"]} | {SCENES.get(r["scene"], r["scene"])} | {r["frame"]} | '
                  f'{r["area"]}-{r["level"]} | {r["room_speed"]} | {flag_text(r["flags"])} | '
                  f'{r["work_ms"]:.2f} | {r["span_ms"]:.2f} |')
    if args.json:
        with open(args.json, 'w') as target:
            json.dump(dict(schema='downwell-work-census/v1', count=census['count'],
                           capacity=census['capacity'], ticks_per_field=census['ticks_per_field'],
                           overwritten=census['overwritten'], classes=summary,
                           handoffs=transitions, records=records),
                      target, indent=1)
            target.write('\n')


if __name__ == '__main__':
    main()
