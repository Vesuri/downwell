#!/usr/bin/env python3
"""Pin scrDeadBody's synchronous Create and caller/corpse assignment order."""
from hashlib import sha256
from pathlib import Path
import json

root = Path(__file__).resolve().parents[1]
export = root / "extracted/utmt-0.9.2.0"
manifest = json.loads((export / "manifest.json").read_text())
source = "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"
assert sha256((root / "data/data.win").read_bytes()).hexdigest() == source
assert manifest["source"]["sha256"] == source
assert manifest["extractor"]["version"] == "0.9.2.0"
assert manifest["extractor"]["commit"] == "03c7048438347799e0717b6e57a9358d000472e0"
obj = manifest["resources"]["objects"][263]
assert (obj["id"], obj["name"], obj["parentId"]) == (263, "objDeadBody", 90)
assert obj["spriteId"] == 172 and obj["textureMaskId"] == 599
assert not any(e["eventType"] == 7 and e["eventSubtype"] == 7 for e in obj["events"])
assembly = (export / "Assembly/gml_GlobalScript_scrDeadBody.asm").read_bytes()
assert sha256(assembly).hexdigest() == "2245353309bd2ac8cb2006f0e162803fb1df0079b8ab2f42dcd20ac469425e08"
print("Pinned scrDeadBody: synchronous object-263 Create, ordered overrides and caller xsp mutation")
for name,digest in {
    "objDeadBody": "c16d01975d0ec7438dc9f4d01099ef9e9a85a11ece015940ec655ef91831d793",
    "parentShootableThing": "f5c0c5d89f5db5d6ecc01a01c961592fb0a0e413eda14df7c402ed936f487a40",
}.items():
    assert sha256((export / f"Assembly/gml_Object_{name}_Create_0.asm").read_bytes()).hexdigest() == digest
for event,digest in {
    "Draw_0": "ac27f4219f2706c07122b7b06c1549709c5be5f5697777c4560daa12f4d68854",
    "Step_2": "71aaac10eef02f61e15327032a995fe459f49451cd38b8434e071ec96268b315",
    "Collision_parentPBullet": "e14dac0a7386d3f986e505aa6a23ba025b66de1ae6796ef8eb1e098f17370010",
    "Collision_subparentEnemy": "e14dac0a7386d3f986e505aa6a23ba025b66de1ae6796ef8eb1e098f17370010",
    "Step_0": "ece87bc038a5d6ee43fb6cf5e9d238ac12f30182d4765c748602f5e2215eebd5",
    "Alarm_0": "c66e3d24e7a8fd8cbcaf0e06dc3457cae0aea183604afe5d578cd7b6f0ab535a",
    "Alarm_1": "1d019f4c251b3e928cd9e7e1e43ef6fdee1c1ac2b44926914947097b10d7763b",
    "Alarm_2": "aabeb387894e077ae2089bfcf7a6f485401b32a0599400fb78fdffe079108e9f",
    "Alarm_3": "7ef1cbd642cf4091103bb6b5276b8992d8dacf3c2dcc5d51a50b1f136de46402",
    "Collision_objPlayer_n": "a50d56a52a49e2e7b35c5980a268db839e321359a5a8e75b5e275e7f7f0b5ddc",
}.items():
    assert sha256((export / f"Assembly/gml_Object_objDeadBody_{event}.asm").read_bytes()).hexdigest() == digest
assert sha256((export / "Assembly/gml_GlobalScript_scrOutofview.asm").read_bytes()).hexdigest() == \
    "e06b55bcc8952fd6eb918461ffce8196a41cff92c7f7dfe97060160468a2fcff"
step = (export / "Assembly/gml_Object_objDeadBody_Step_0.asm").read_text()
assert step.index("gml_Script_TimeStopBound") < step.index("call.i place_meeting")
assert step.count("pushi.e 56\nconv.i.v") == 4
assert step.count("call.i place_meeting(argc=3)") == 4
assert "push.v self.yy\ncall.i round(argc=1)\npop.v.v self.y\npush.v self.xx" in step
print("Pinned corpse Step: TimeStopBound before solid-56 queries, Y then X publication")
