#!/usr/bin/env python3
"""Pin the object-223 dependency used by corpse consumption to exported VM."""
import json
from pathlib import Path

root = Path(__file__).resolve().parents[1] / "extracted/utmt-0.9.2.0"
manifest = json.loads((root / "manifest.json").read_text())
obj = manifest["resources"]["objects"][223]
assert (obj["name"], obj["spriteId"], obj["parentId"], obj["Depth"]) == ("fxBlood", 92, -1, 0)
assert len(manifest["resources"]["sprites"][92]["frames"]) == 8
assert {(e["eventType"], e["eventSubtype"]) for e in obj["events"]} == {(0, 0), (2, 0), (3, 0), (4, 88), (4, 57), (14, 0)}

def instructions(event):
    return [s.strip() for s in (root / "Assembly" / f"gml_Object_fxBlood_{event}.asm").read_text().splitlines()
            if s.strip() and not s.startswith((".", ":"))]

create = instructions("Create_0")
assert create == """pushi.e 1
conv.i.v
pushi.e -2
conv.i.v
call.i random_range(argc=2)
pop.v.v self.ysp
pushi.e 1
conv.i.v
call.i random(argc=1)
pushi.e -1
conv.i.v
pushi.e 1
conv.i.v
call.i choose(argc=2)
mul.v.v
pop.v.v self.xsp
push.i 68049
setowner.e
pushi.e 120
conv.i.v
pushi.e -1
pushi.e 0
pop.v.v [array]self.alarm
push.d 0.2
pop.v.d global.grav
push.d 0.3
pop.v.d self.image_speed
pushi.e 7
conv.i.v
call.i irandom(argc=1)
pop.v.v self.image_index""".splitlines()
assert instructions("Step_0") == """push.v self.x
push.v self.xsp
add.v.v
pop.v.v self.x
push.v self.y
push.v self.ysp
add.v.v
pop.v.v self.y
push.v self.xsp
push.d 1.01
div.d.v
pop.v.v self.xsp
push.v self.ysp
pushglb.v global.grav
add.v.v
pop.v.v self.ysp
push.v self.y
pushglb.v global.fallspeed
sub.v.v
pop.v.v self.y""".splitlines()
for event in ("Alarm_0", "Collision_parentWall", "Collision_parentWater"):
    assert instructions(event) == ["call.i gml_Script_action_kill_object(argc=0)", "popz.v"]
print("Pinned fxBlood 223: global gravity write, source RNG order, three position setters and three kill events")
