#!/usr/bin/env python3
"""Pin object 379's recovered event bodies and absence of inherited alarms."""
from hashlib import sha256
import json
from pathlib import Path

root = Path(__file__).resolve().parents[1]
export = root / "extracted/utmt-0.9.2.0"
manifest = json.loads((export / "manifest.json").read_text())
source = "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"
assert sha256((root / "data/data.win").read_bytes()).hexdigest() == source
assert manifest["source"]["sha256"] == source
assert manifest["extractor"]["version"] == "0.9.2.0"
assert manifest["extractor"]["commit"] == "03c7048438347799e0717b6e57a9358d000472e0"
obj = manifest["resources"]["objects"][379]
assert (obj["name"], obj["spriteId"], obj["parentId"], obj["textureMaskId"]) == ("limboShard", 91, -1, -1)
events = [
    (0, 0, 2769, "Create_0", "045dd4d335590c43a344eb718d870a18a2d1a454be848aa560fe6126931249cc"),
    (3, 0, 2770, "Step_0", "87c075fafedd836e27253c280de7da8908d7454b6e32e7d64c804e88d93de195"),
    (4, 89, 2771, "Collision_parentPBullet", "c8cc4ca2dfec47fd8b1168990cb9accce8b054ef85caf8b61a4fb9d058dac5da"),
    (8, 0, 2772, "Draw_0", "b4f50d897f2c290b6b82cd55add69cf85a10369325591ff2d3ae141945b4ee17"),
    (14, 0, 2773, "PreCreate_0", "4c193a4f963773b2c70b60323cfcdd763b259c208f342702a253d03b3f9e966b"),
]
assert len(obj["events"]) == len(events)
for kind, subtype, code, suffix, digest in events:
    name = "gml_Object_limboShard_" + suffix
    selected = [e for e in obj["events"] if (e["eventType"], e["eventSubtype"]) == (kind, subtype)]
    assert len(selected) == 1 and selected[0]["actions"] == [{"codeId": code, "codeName": name}]
    assert sha256((export / "Assembly" / (name + ".asm")).read_bytes()).hexdigest() == digest
print("Pinned limboShard Create, Step, bullet Collision, Draw and PreCreate; no parent or alarm handlers")
