#!/usr/bin/env python3
"""Pin blast VM inputs and metadata; not evidence of natural encounter admission."""
from hashlib import sha256
from pathlib import Path
import json

root = Path(__file__).resolve().parents[1]
export = root / "extracted/utmt-0.9.2.0"
digest = "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"
assert sha256((root / "data/data.win").read_bytes()).hexdigest() == digest
manifest = json.loads((export / "manifest.json").read_text())
assert manifest["source"]["sha256"] == digest
pins = {
    "gml_Object_bulletBlast_Alarm_0": "e6a4f5b338660e6e9d1c4455b84c1a1caebf9935d0eef61fd9964f5e154bbd84",
    "gml_Object_bulletBlast_Collision_objPlayer_n": "b2e4dba2b576defffbdd70c0820452c5a9647d13d98af0019fb6bcd7beda626a",
    "gml_Object_bulletBlast_Collision_parentShootableThing": "d307f4cc5411e070a9dc39f500e0693bcc5d9f76ea45a4e8d4cd7a02290bf66e",
    "gml_Object_bulletBlast_Collision_parentWall": "5ab6d0b7f1e14c1121590270a63829d4b186fd10f57ebc8f51c1efcaea381f57",
    "gml_Object_bulletBlast_Collision_subparentEnemy": "a9d90e6f8c97b4f9e17f602d5ce44611a338f6dc3998f34fcc46f319195a71e3",
    "gml_Object_bulletBlast_Create_0": "9676b7826bb93ad019ab0da1da93e062f796fa7f551684211a6b01fc3f7c9518",
    "gml_Object_bulletBlast_Other_7": "7fa420002321759f60d9103f49806b50a0673341e139e47b46bb18672ccf6d78",
    "gml_Object_bulletBlast_PreCreate_0": "4c193a4f963773b2c70b60323cfcdd763b259c208f342702a253d03b3f9e966b",
    "gml_Object_bulletBlast_Step_0": "b3ed0d4756ce05f43561520ff1b95c75d277a99f67db1a62aba909ab0c631140",
    "gml_Object_parentPBullet_Create_0": "2a8a1682f62d2b8a80b389ce6bd90baa2e1165b9c1d9b54fc2a0c82f5bfd49c6",
    "gml_GlobalScript_hitIgnoreCheck": "11df5b73f7a7e849160a637a1071d70510d3c3efa36d060388386e41051f83ef",
    "gml_GlobalScript_scrTypicalExplosionDamage": "41caabfd7355bb12ca55f5b9d803d6417745b7dc2e006f90a4466e7ea706a159",
    "gml_GlobalScript_scrDamageEnemy": "912a0fada8c28104c9d4f975346dd908527546556be1fb0d908763a66e66c95d",
    "gml_GlobalScript_scrBulletImpact": "6bcd3beb826d00ca8c9fd34d360bcad709ce938cef45e9cca7757c9858f1e960",
}
for name, expected in pins.items():
    assert sha256((export / "Assembly" / (name + ".asm")).read_bytes()).hexdigest() == expected, name
resources = manifest["resources"]
blast, parent, sprite = resources["objects"][276], resources["objects"][89], resources["sprites"][471]
assert (blast["name"], blast["parentId"], blast["spriteId"], blast["textureMaskId"], blast["Depth"]) == (
    "bulletBlast", 89, 471, -1, 0)
assert blast["Visible"] and not blast["Solid"] and not blast["Persistent"]
assert (parent["name"], parent["parentId"]) == ("parentPBullet", -1)
assert {(e["eventType"], e["eventSubtype"]) for e in parent["events"]} == {(0, 0), (14, 0)}
assert {(e["eventType"], e["eventSubtype"]) for e in blast["events"]} == {
    (0, 0), (2, 0), (3, 0), (4, 90), (4, 57), (4, 83), (4, 35), (7, 7), (14, 0)}
assert (sprite["name"], len(sprite["frames"]), sprite["separateMasks"]) == ("sprBlast", 14, "AxisAlignedRect")
print("Pinned blast events, inherited bullet Create, hit memory, damage scripts and 14-frame sprite")
