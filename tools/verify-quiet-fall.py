#!/usr/bin/env python3
"""Check the historical no-input landing witness, not a performance threshold."""
import math
from pathlib import Path
import sys

LANDING = dict(tick=164, x=0, y=262144, health=4, ammo=8, enemies=3, rng=1371)
MOTION = dict(vx=0, vy=0, camera_x=0, camera_y=1024, grounded=1, bullets=0)


def record(text, name):
    rows = [line for line in text.splitlines() if line.startswith(name + ' ')]
    if len(rows) != 1:
        raise ValueError(f'{name}: expected exactly one record')
    result = {}
    for field in rows[0].split()[1:]:
        key, value = field.split('=')
        if key in result:
            raise ValueError(f'{name}: duplicate {key}')
        result[key] = value
    return result


def verify(text):
    if text.splitlines().count('DOWNWELL_QUIET_CADENCE_READY') != 1 or 'DOWNWELL_QUIET_INVALID' in text:
        raise ValueError('missing or invalid completion witness')
    for name, expected in [('QUIET_LANDING', LANDING), ('QUIET_MOTION', MOTION)]:
        actual = {key: int(value) for key, value in record(text, name).items()}
        if actual != expected:
            raise ValueError(f'{name}: no-input landing mismatch: {actual}')
    cadence = record(text, 'QUIET_CADENCE')
    if set(cadence) != {'frames', 'fields', 'fps', 'worst_gap', 'worst_frame'}:
        raise ValueError('incomplete cadence record')
    frames, fields = int(cadence['frames']), int(cadence['fields'])
    gap, worst = int(cadence['worst_gap']), int(cadence['worst_frame'])
    fps = float(cadence['fps'])
    if frames != 164 or fields < frames or not 1 <= gap <= fields or not 1 <= worst <= frames:
        raise ValueError('invalid cadence counters')
    if not math.isfinite(fps) or abs(fps - 50 * frames / fields) > 0.00051:
        raise ValueError('FPS does not match field counters')
    return fields


if __name__ == '__main__':
    if len(sys.argv) != 2:
        raise SystemExit('usage: verify-quiet-fall.py GDB_LOG')
    try:
        fields = verify(Path(sys.argv[1]).read_text())
    except (ValueError, OSError) as error:
        raise SystemExit(f'Quiet-fall witness rejected: {error}')
    print(f'Quiet-fall landing witness verified: 164 updates / {fields} fields; performance not judged')
