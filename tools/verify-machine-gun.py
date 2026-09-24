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

# Rendering must follow the firing direction even before the remaining spread
# and fractional-motion work is connected. Do not mistake frame-zero artwork
# (authored pointing right) for an already downward-facing projectile.
shoot = (export / "Assembly/gml_GlobalScript_scrPlayerShootN.asm").read_text()
step = (export / "Assembly/gml_Object_bulletStandard_Step_0.asm").read_text()
assert "pushi.e 270\npush.v self.aimAngle\nadd.v.i\npop.v.v self.shotAngle" in shoot
assert "push.v self.bDir\npop.v.v self.image_angle" in step
assert step.index("call.i random_range(argc=2)") < step.index("pop.v.v self.image_angle")
print("VM projectile image angle follows firing direction plus first-Step spread")
create = (export / "Assembly/gml_Object_bulletStandard_Create_0.asm").read_text()
assert "push.d 0.3\npop.v.d self.image_speed" in create
assert "pushi.e 0\npop.v.i self.image_index" in create
assert all(len(manifest["resources"]["sprites"][i]["frames"]) == 6 for i in (458, 459))
print("VM projectile animation starts at zero, advances by 0.3, and uses six-frame sprites")
assert "push.v self.allSet\nconv.v.b\nnot.b\nbf [2]" in step
assert "push.v self.bDir\npush.v self.bSpeed\ncall.i lengthdir_x(argc=2)\npop.v.v self.xsp" in step
assert "push.v self.bDir\npush.v self.bSpeed\ncall.i lengthdir_y(argc=2)\npop.v.v self.ysp" in step
assert step.index("pop.v.i self.allSet") < step.index("call.i gml_Script_scrBulCheckSolid(argc=0)")
assert step.index("call.i gml_Script_scrBulCheckSolid(argc=0)") < step.index("pop.v.v self.x\n")
assert "push.v self.x\npush.v self.xsp\nadd.v.v\npop.v.v self.x" in step
assert "push.v self.y\npush.v self.ysp\nadd.v.v\npop.v.v self.y" in step
print("VM spread/velocity initialization precedes solid checks and retained vector movement")
