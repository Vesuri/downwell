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
skeleton = (export / "Assembly/gml_Object_enmSkeleton_Step_0.asm").read_bytes()
assert sha256(skeleton).hexdigest() == "159d57d00e13a2dd00e8aefd5edc246735f47c8988f1ca69839add909865d76a"
throw = skeleton.decode().split(":[21]", 1)[1].split(":[27]", 1)[0]
ordered = ["call.i gml_Script_instance_create", "pop.v.v self.myBone",
           "call.i gml_Script_soundPlayOL", "pop.v.v self.yarc", "pop.v.v self.xarc",
           "pop.v.v [stacktop]self.ysp", "pop.v.v [stacktop]self.xsp",
           "pop.i.v [stacktop]self.imgSp", "pop.v.i self.eShotDelay"]
assert [throw.index(op) for op in ordered] == sorted(throw.index(op) for op in ordered)
assert throw.count("call.i choose(argc=3)") == 2
for name, digest in {
    "enmSkeleton": "6dfaeddd37bd746f3fcde25d350a6a25ce690df5ac3a9e962e933b1c9bd54615",
    "subparentEnemy": "575a31b3123128e4ce352c4ee169193583167b76cd0001d4e360f0f4362cd3c0",
    "parentLife": "19838825dc0e49ada1d670de755ba9942ab622914a87d8d4d2acb51eae4c3c04",
}.items():
    payload = (export / "Assembly" / f"gml_Object_{name}_Create_0.asm").read_bytes()
    assert sha256(payload).hexdigest() == digest
    assert b"random" not in payload and b"choose" not in payload
skeleton_object = manifest["resources"]["objects"][120]
assert (skeleton_object["parentId"], skeleton_object["spriteId"], skeleton_object["textureMaskId"]) == (83, 208, 208)
for sprite, frames in ((205, 1), (207, 8), (208, 6)):
    assert len(manifest["resources"]["sprites"][sprite]["frames"]) == frames
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
view = (export / "Assembly/gml_GlobalScript_scrInView.asm").read_bytes()
assert sha256(view).hexdigest() == "cf66675bc8ea809f5bd42270ceee29d3e50eb9c3aa9d916205006db32e0e3432"
assert view.count(b"cmp.v.v GT") == 2 and view.count(b"cmp.v.v LT") == 2
assert view.count(b"call.i gml_Script___view_get(argc=2)") == 6
assert b"push.v self.marginyBottom\nadd.v.v\ncmp.v.v LT" in view
obj = manifest["resources"]["objects"][269]
fx = (export / "Assembly/gml_GlobalScript_scrFxNol.asm").read_bytes()
assert sha256(fx).hexdigest() == "fe76a7898f52d717dc47f864c924facf2cea92a5d87e26b3bfd9ba27ce0c2d6c"
assert fx.index(b"instance_create") < fx.index(b"pop.v.v self.myFx") < fx.index(b"self.sprite_index") < fx.index(b"self.image_speed")
assert b"self.image_angle" not in fx and b"self.depth" not in fx
assert len(manifest["resources"]["sprites"][85]["frames"]) == 6
effect = manifest["resources"]["objects"][219]
assert effect["parentId"] == -1 and effect["spriteId"] == -1
assert not any(e["eventType"] in (0,8) for e in effect["events"])
assert obj["name"] == "bulletEnmBone" and obj["spriteId"] == 460
assert obj["parentId"] == 90 and obj["textureMaskId"] == -1
draw = source("Draw_0", "f80bec54d3f7e90d0ee349add68771ed6f26c0acd51b80c233edb68ceda536ba")
assert draw.index("gml_Script_drawShade") < draw.index("draw_self")
shade = (export / "Assembly/gml_GlobalScript_drawShade.asm").read_bytes()
assert sha256(shade).hexdigest() == "c0aa27477ef320ed8e7f9bcc38bd4d468d0b84ed9dedd2d7bacd1e1844585c82"
assert shade.index(b"global.lowSpec") < shade.index(b"scrInView") < shade.index(b"global.noControl") < shade.index(b"surface_exists")
assert shade.count(b"call.i round(argc=1)") == 2 and b"pushi.e 117" in shade
parent_object = manifest["resources"]["objects"][90]
assert parent_object["parentId"] == -1 and not any(e["eventType"] == 8 for e in parent_object["events"])
assert len(manifest["resources"]["sprites"][460]["frames"]) == 4
assert not any(e["eventType"] == 7 and e["eventSubtype"] == 7 for e in obj["events"])
print("Pinned bone Step, collision callback order and four-frame animation metadata")
