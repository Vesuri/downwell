#!/usr/bin/env python3
"""Check hand-authored rectangle adapters against the untouched-build export."""
import json
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
manifest = json.loads((root / "extracted/utmt-0.9.2.0/manifest.json").read_text())
assert manifest["source"]["sha256"] == "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"
assert manifest["extractor"]["version"] == "0.9.2.0"
header = (root / "src/core/EnemyContact.h").read_text()
records = re.findall(r"case (\d+): return \{(-?\d+), (-?\d+), (-?\d+), (-?\d+)\};", header)
assert {int(row[0]) for row in records} == {31, 84, 133, 134, 153, 154, 157, 162, 163, 164, 166, 177, 186, 208, 278, 460, 569, 585}
for row in records:
    sprite = manifest["resources"]["sprites"][int(row[0])]
    assert sprite["id"] == int(row[0])
    assert sprite["separateMasks"] == "AxisAlignedRect"
    expected = (sprite["marginLeft"] - sprite["originX"],
                sprite["marginRight"] - sprite["originX"] + 1,
                sprite["marginTop"] - sprite["originY"],
                sprite["marginBottom"] - sprite["originY"] + 1)
    assert tuple(map(int, row[1:])) == expected, (sprite["name"], row, expected)
for object_id, mask in ((35, 31), (113, 166), (117, 177), (119, 278), (120, 208)):
    assert manifest["resources"]["objects"][object_id]["textureMaskId"] == mask
assert manifest["resources"]["objects"][112]["spriteId"] == 186
assert manifest["resources"]["objects"][248]["spriteId"] == 133
assert manifest["resources"]["objects"][248]["textureMaskId"] == -1
assert manifest["resources"]["objects"][116]["spriteId"] == 153
assert manifest["resources"]["objects"][269]["spriteId"] == 460
assert manifest["resources"]["objects"][269]["textureMaskId"] == -1
assert manifest["resources"]["objects"][225]["spriteId"] == 84
assert manifest["resources"]["objects"][225]["textureMaskId"] == -1
for object_id in (60,92):
    assert manifest["resources"]["objects"][object_id]["textureMaskId"] == 585
assert manifest["resources"]["objects"][293]["spriteId"] == 569
assert manifest["resources"]["objects"][293]["textureMaskId"] == -1
for object_id in (98, 278, 279):
    parent = object_id
    while parent != 57:
        assert parent >= 0, (object_id, "shop child is not parentWall")
        parent = manifest["resources"]["objects"][parent]["parentId"]
# A one-way platform belongs to parentThinwall, not parentWall. Player landing
# queries therefore cannot stand in for smoke's collision-event target family.
parent = 93
while parent >= 0:
    assert parent != 57
    parent = manifest["resources"]["objects"][parent]["parentId"]
assert {event["eventSubtype"] for event in manifest["resources"]["objects"][225]["events"]
        if event["eventType"] == 4} == {57, 88}
water_collision = (root / "extracted/utmt-0.9.2.0/Assembly/gml_Object_fxSmoke_Collision_parentWater.asm").read_text()
assert not [line for line in water_collision.splitlines()
            if line.strip() and not line.startswith((".", ":"))]
smoke_collision = (root / "extracted/utmt-0.9.2.0/Assembly/gml_Object_fxSmoke_Collision_parentWall.asm").read_text()
assert [line.strip() for line in smoke_collision.splitlines()
        if line.strip() and not line.startswith((".", ":"))] == [
    "push.v self.nocol", "conv.v.b", "not.b", "bf [end]",
    "push.v self.smokesp", "push.d 0.2", "mul.d.v", "pop.v.v self.smokesp"]
assert manifest["resources"]["rooms"][11]["name"] == "rmMain"
assert manifest["resources"]["rooms"][11]["width"] == 480
# Skeleton has its own Collision event, not typStompable's yy > ply+2 test.
# Retain the two independent branches and the post-stomp grounded re-read.
assembly = (root / "extracted/utmt-0.9.2.0/Assembly/gml_Object_enmSkeleton_Collision_objPlayer_n.asm").read_text()
instructions = [line.strip() for line in assembly.splitlines()
                if line.strip() and not line.startswith((".", ":"))]
assert instructions == [
    "push.v self.y", "pushglb.v global.ply", "cmp.v.v GT", "bf [4]",
    "push.v 35.grounded", "conv.v.b", "not.b", "bf [4]",
    "push.v 35.ysp", "pushi.e 0", "cmp.i.v GT", "bf [4]",
    "call.i gml_Script_scrEnemyStomped(argc=0)", "popz.v",
    "push.v 35.grounded", "conv.v.b", "bf [6]",
    "pushi.e 2", "conv.i.v", "pushi.e 3", "conv.i.v", "pushi.e 1", "conv.i.v",
    "call.i gml_Script_scrTypicalDamage(argc=3)", "popz.v", "b [end]",
    "pushglb.v global.ply", "push.v self.y", "cmp.v.v GT", "bf [end]",
    "pushi.e 2", "conv.i.v", "pushi.e 3", "conv.i.v", "pushi.e 1", "conv.i.v",
    "call.i gml_Script_scrTypicalDamage(argc=3)", "popz.v",
]
step = (root / "extracted/utmt-0.9.2.0/Assembly/gml_Object_enmSkeleton_Step_0.asm").read_text()
sound_calls = re.findall(
    r'push\.s "enemymove"@\d+\nconv\.s\.v\npushi\.e 1\nconv\.i\.v\n'
    r'pushi\.e 0\nconv\.i\.v\npushi\.e 50\nconv\.i\.v\npushi\.e (\d+)\n'
    r'conv\.i\.v\ncall\.i gml_Script_soundPlayOL\(argc=5\)', step)
assert sound_calls == ["133", "134", "132"]
assert step.count("call.i gml_Script_soundPlayOL") == 3
print("18 actor/terrain contact rectangles, smoke wall response and skeleton Step sound calls match pinned source")
