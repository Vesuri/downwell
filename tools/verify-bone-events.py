#!/usr/bin/env python3
"""Pin source event order consumed by BoneProjectile's registered entry points."""
from hashlib import sha256
from pathlib import Path
import json

root = Path(__file__).resolve().parents[1]
export = root / "extracted/utmt-0.9.2.0"
manifest = json.loads((export / "manifest.json").read_text())
assert manifest["source"]["sha256"] == sha256((root / "data/data.win").read_bytes()).hexdigest() == \
    "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"
assert manifest["extractor"]["version"] == "0.9.2.0"
assert manifest["extractor"]["commit"] == "03c7048438347799e0717b6e57a9358d000472e0"

def source(suffix, digest):
    payload = (export / "Assembly" / ("gml_Object_bulletEnmBone_" + suffix + ".asm")).read_bytes()
    assert sha256(payload).hexdigest() == digest
    return payload.decode()

step = source("Step_0", "dfee354bb77968c1ed2706113b04f04b0916496d31520706028021874d763fc3")
create = source("Create_0", "76595cba4a9d7f33e279224ef7963a43e9b56ac6ce5d2f59b64698968907c64e")
parent = (export / "Assembly/gml_Object_parentShootableThing_Create_0.asm").read_bytes()
assert sha256(parent).hexdigest() == "f5c0c5d89f5db5d6ecc01a01c961592fb0a0e413eda14df7c402ed936f487a40"
assert b"call.i" not in parent
assert create.count("call.i random_range(argc=2)") == 3
assert create.index("event_inherited") < create.index("random_range")
wall = source("Collision_parentWall", "b90f31393aabda2f26251e4bd4eea283740696ffd91807231b05899e281dc622")
player = source("Collision_objPlayer_n", "918f6068a1942b503dd74d92a6dff774bf0fa2fa1986b6fb38e0145e0a761df4")
ordered = ["cmp.v.v NEQ", "pop.v.v self.image_speed", "pop.v.v self.ysp", "pop.v.v self.xx",
           "pop.v.v self.yy", "pop.v.v self.x", "pop.v.v self.y", "pop.v.i self.image_speed",
           "pop.v.i self.mask_index", "pop.v.i self.xsp", "pop.v.i self.ysp", "pop.v.i self.active",
           "call.i instance_destroy", "call.i gml_Script_scrFxNol", "call.i gml_Script_scrInView"]
offset = 0
for instruction in ordered:
    offset = step.index(instruction + ("\n" if instruction.startswith("pop.") else ""), offset) + len(instruction)
assert wall.index("instance_destroy") < wall.index("scrFxNol")
assert player.index("scrTypicalDamage") < player.index("scrFxNol") < player.index("instance_destroy")
obj = manifest["resources"]["objects"][269]
assert obj["name"] == "bulletEnmBone" and obj["spriteId"] == 460
assert obj["parentId"] == 90 and obj["textureMaskId"] == -1
assert len(manifest["resources"]["sprites"][460]["frames"]) == 4
assert not any(e["eventType"] == 7 and e["eventSubtype"] == 7 for e in obj["events"])
print("Pinned bone Step, collision callback order and four-frame animation metadata")
