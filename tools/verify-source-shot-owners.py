#!/usr/bin/env python3
"""Compare controlled source-Step traces from both host-built world owners.

This is ownership parity, not an original-game or native-emulator oracle.
Each executable also checks real lifetime, effect and collision integration.
"""
import subprocess
from pathlib import Path

root = Path(__file__).resolve().parents[1]


def traces(executable):
    result = subprocess.run([str(root / "build" / executable)], cwd=root,
                            text=True, capture_output=True, timeout=120)
    if result.returncode:
        raise RuntimeError(f"{executable} failed:\n{result.stdout}\n{result.stderr}")
    rows = {}
    for line in result.stdout.splitlines():
        if not line.startswith("SOURCE_SHOT "):
            continue
        fields = line.split()
        assert len(fields) == 12, line
        key = tuple(map(int, fields[1:3]))
        assert key not in rows, f"duplicate scenario: {key}"
        rows[key] = fields[3:]
    assert set(rows) == {(0, 0), (0, 1), (1, 0), (1, 1)}, rows
    return rows


native = traces("gameplay-collision-owner-test")
stored = traces("live-collision-lifecycle-test")
for key in sorted(native):
    assert native[key] == stored[key], (key, native[key], stored[key])
print("Source shot owners: four profile/spread scenarios match bit-exact motion, angle, damage and query/write counts")
