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
assembly = (export / "Assembly/gml_GlobalScript_scrDeadBody.asm").read_bytes()
assert sha256(assembly).hexdigest() == "2245353309bd2ac8cb2006f0e162803fb1df0079b8ab2f42dcd20ac469425e08"
print("Pinned scrDeadBody: synchronous object-263 Create, ordered overrides and caller xsp mutation")
for name,digest in {
    "objDeadBody": "c16d01975d0ec7438dc9f4d01099ef9e9a85a11ece015940ec655ef91831d793",
    "parentShootableThing": "f5c0c5d89f5db5d6ecc01a01c961592fb0a0e413eda14df7c402ed936f487a40",
}.items():
    assert sha256((export / f"Assembly/gml_Object_{name}_Create_0.asm").read_bytes()).hexdigest() == digest
for event,digest in {
    "Alarm_0": "c66e3d24e7a8fd8cbcaf0e06dc3457cae0aea183604afe5d578cd7b6f0ab535a",
    "Alarm_1": "1d019f4c251b3e928cd9e7e1e43ef6fdee1c1ac2b44926914947097b10d7763b",
    "Alarm_2": "aabeb387894e077ae2089bfcf7a6f485401b32a0599400fb78fdffe079108e9f",
    "Alarm_3": "7ef1cbd642cf4091103bb6b5276b8992d8dacf3c2dcc5d51a50b1f136de46402",
    "Collision_objPlayer_n": "a50d56a52a49e2e7b35c5980a268db839e321359a5a8e75b5e275e7f7f0b5ddc",
}.items():
    assert sha256((export / f"Assembly/gml_Object_objDeadBody_{event}.asm").read_bytes()).hexdigest() == digest
