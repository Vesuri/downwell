#!/usr/bin/env python3
"""Pin the resource identity and complete object-370 Create used by DebrisBirth."""
from hashlib import sha256
from pathlib import Path
import json

root = Path(__file__).resolve().parents[1]
export = root / "extracted/utmt-0.9.2.0"
manifest = json.loads((export / "manifest.json").read_text())
source_hash = "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"
assert sha256((root / "data/data.win").read_bytes()).hexdigest() == source_hash
assert manifest["source"]["sha256"] == source_hash
assert manifest["extractor"]["version"] == "0.9.2.0"
assert manifest["extractor"]["commit"] == "03c7048438347799e0717b6e57a9358d000472e0"
obj = manifest["resources"]["objects"][370]
assert (obj["id"], obj["name"], obj["parentId"], obj["spriteId"]) == (370, "breakablesDebris", -1, 91)
assert obj["textureMaskId"] == 598
assert obj["Persistent"] is False
event = next(e for e in obj["events"] if e["eventType"] == 0 and e["eventSubtype"] == 0)
assert event["actions"] == [{"codeId": 2722, "codeName": "gml_Object_breakablesDebris_Create_0"}]
assert not any(e["eventType"] == 7 and e["eventSubtype"] == 7 for e in obj["events"])
assembly = (export / "Assembly/gml_Object_breakablesDebris_Create_0.asm").read_bytes()
assert sha256(assembly).hexdigest() == "7eeeed80e7b0eeb7c293fd5dbf872ab9644985906dbe9addc7bbefaba1a91417"
for name, digest in {
    "Step_0": "f10781ed5f8231e1ead276eba2b2de864c2f4f70e5f3498e7ec0550e28603052",
    "Alarm_0": "d7288b2999ecc62b3f8db25a0313f8144cf903161c68062d4cb8181c8fbcd7dd",
    "Alarm_1": "3b7a34c872f9265143d15f3052836d739c30e20f3668d9446ebe86ab8e13a1dc",
    "Alarm_2": "790f7e88f5b3bca443bafdaca3a2f2fcc946cd55e05cc25a17875a5828e99738",
    "Collision_objPlayer_n": "e14dac0a7386d3f986e505aa6a23ba025b66de1ae6796ef8eb1e098f17370010",
    "Draw_0": "ac27f4219f2706c07122b7b06c1549709c5be5f5697777c4560daa12f4d68854",
}.items():
    assert sha256((export / f"Assembly/gml_Object_breakablesDebris_{name}.asm").read_bytes()).hexdigest() == digest
print("Pinned object-370 Create: no parent or relocation, four RNG draws and area artwork")
assert sha256((export / "Assembly/gml_GlobalScript_drawFx.asm").read_bytes()).hexdigest() == \
    "6e3dceea4db4e2baaf76c5d4103bd4ed930042ae769d209369d98246162b5d92"
