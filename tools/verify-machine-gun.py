#!/usr/bin/env python3
"""Check the shared selected profiles against the pinned VM table writes."""
import json
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
export = root / "extracted/utmt-0.9.2.0"
manifest = json.loads((export / "manifest.json").read_text())
assert manifest["source"]["sha256"] == "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"
assert manifest["extractor"]["version"] == "0.9.2.0"
asm = (export / "Assembly/gml_GlobalScript_bStatInitLevel1.asm").read_text()
assert "pushi.e 0\npop.v.i self.gunnum" in asm
first = asm.split("push.v self.gunnum\npushi.e 1\nadd.i.v\npop.v.v self.gunnum", 1)[0]
assert 'push.s "MACHINEGUN"' in first
writes = re.findall(
    r"pushi.e (-?\d+)\nconv.i.v\npushi.e -5\npush.v self.gunnum\nconv.v.i\n"
    r"push.v \[arraypopaf\]self.(bStat\w+)\npushi.e ([01])\npopaf.e", first)
table = {}
for value, field, level in writes:
    key = (field, int(level))
    assert key not in table, ("duplicate write", key)
    table[key] = int(value)

fields = ["Object", "BulletSprite", "Sound", "Muzzle", "Damage", "Speed",
          "Accuracy", "ConRate", "Rof", "Recoil", "ScreenShake",
          "ScreenShakeDur", "RangeRandom", "RangeTimer", "Pierce", "Wave", "Explode"]
header = (root / "src/core/MachineGunProfile.h").read_text()
rows = re.findall(r"MachineGunProfile\{([^{}]+)\}", header)
assert len(rows) == 2, "expected exactly two literal source profiles"
levels = set()
for row in rows:
    tokens = [v.strip() for v in row.split(",")]
    values = [int(v.replace("false", "0").replace("true", "1")) for v in tokens]
    level, actual = values[0], values[1:]
    assert level in (0, 1) and level not in levels
    levels.add(level)
    expected = [table[("bStat" + field, level)] for field in fields]
    assert actual == expected, (level, dict(zip(fields, actual)), dict(zip(fields, expected)))
print("Both machine-gun profiles match 34 selected VM table assignments")
