#!/usr/bin/env python3
"""Verify shared area-one enemy birth constants against pinned VM assembly."""
from decimal import Decimal
from hashlib import sha256
import json
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
export = root / "extracted/utmt-0.9.2.0"
manifest = json.loads((export / "manifest.json").read_text())
assert manifest["source"]["sha256"] == "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"
assert manifest["extractor"]["version"] == "0.9.2.0"
header = (root / "src/core/EnemyCreateProfile.h").read_text()
profiles = re.findall(r"case (\d+): return \{(\d+), (\d+), (\d+)\};", header)
assert len(profiles) == 7
assert {int(row[0]) for row in profiles} == {111, 112, 113, 114, 116, 117, 119}
for object_id, health, reward, rate in profiles:
    obj = manifest["resources"]["objects"][int(object_id)]
    assert obj["id"] == int(object_id)
    assembly = (export / "Assembly" / f"gml_Object_{obj['name']}_Create_0.asm").read_text()
    def assigned(name):
        matches = re.findall(r"push(?:i\.e|\.d) (-?[\d.]+)\npop\.v\.[id] self\." + name + r"\n", assembly)
        assert len(matches) == 1, (obj["name"], name, matches)
        return Decimal(matches[0])
    assert assigned("ehp") == Decimal(health), obj["name"]
    assert assigned("money") == Decimal(reward), obj["name"]
    assert assigned("imgSp") * 20 == Decimal(rate), obj["name"]
    assert "push.v self.imgSp\npop.v.v self.image_speed" in assembly, obj["name"]
print("7 shared enemy birth profiles match pinned Create-event VM assembly")

# White jelly is a separately owned source actor, not another compact profile.
# Its inherited setters and RNG precede the concrete direction publication.
for name, digest in (
    ("parentLife", "19838825dc0e49ada1d670de755ba9942ab622914a87d8d4d2acb51eae4c3c04"),
    ("subparentEnemy", "575a31b3123128e4ce352c4ee169193583167b76cd0001d4e360f0f4362cd3c0"),
    ("enmWhiteJelly", "d23276a6a24d6e69159f8f8a2bde523119b594f5336d1e17a739d4e2116e312e"),
):
    payload = (export / "Assembly" / f"gml_Object_{name}_Create_0.asm").read_bytes()
    assert sha256(payload).hexdigest() == digest
jelly = manifest["resources"]["objects"][115]
assert (jelly["name"], jelly["parentId"], jelly["textureMaskId"]) == ("enmWhiteJelly", 83, 174)
assert manifest["resources"]["objects"][83]["parentId"] == 34
assert manifest["resources"]["objects"][34]["name"] == "parentLife"
assembly = payload.decode()
markers = ["call.i event_inherited(argc=0)", "call.i random_range(argc=2)",
           "pop.v.v self.xx", "pushglb.v global.ply", "pushglb.v global.plx",
           "call.i point_direction(argc=4)", "pop.v.v self.direction",
           "push.v self.direction\npop.v.v self.playerDir", "pop.v.i self.active",
           "pop.v.v self.image_speed"]
positions = [assembly.index(marker) for marker in markers]
assert positions == sorted(positions)
print("Pinned white-jelly inheritance, RNG and direction/image setter sequence")

red = manifest["resources"]["objects"][118]
assert (red["name"], red["parentId"], red["spriteId"], red["textureMaskId"]) == ("enmRedBall", 83, 702, 153)
assert manifest["resources"]["objects"][34]["parentId"] == -1
assert next(e for e in red["events"] if e["eventType"] == 0)["actions"][0]["codeId"] == 1243
for name in ("enmRedBall", "subparentEnemy", "parentLife"):
    payload = (export / "Assembly" / f"gml_Object_{name}_PreCreate_0.asm").read_bytes()
    assert sha256(payload).hexdigest() == "4c193a4f963773b2c70b60323cfcdd763b259c208f342702a253d03b3f9e966b"
payload = (export / "Assembly/gml_Object_enmRedBall_Create_0.asm").read_bytes()
assert sha256(payload).hexdigest() == "730c853b056f240eda01cb48dfb434c2221de456c686ac1b4499f9b2e7224847"
assembly = payload.decode()
markers = ["call.i event_inherited(argc=0)", "call.i random_range(argc=2)",
           "pop.v.v self.xx", "pushglb.v global.ply", "pushglb.v global.plx",
           "call.i point_direction(argc=4)", "pop.v.v self.direction", "pop.v.i self.active",
           "pop.v.i self.emitTimer", "pushi.e 0\npop.v.v [array]self.alarm",
           "pushi.e 2\npop.v.v [array]self.alarm", "pop.v.v self.image_speed"]
positions = [assembly.index(marker) for marker in markers]
assert positions == sorted(positions)
print("Pinned red-ball PreCreate chain and synchronous Create RNG/heading/alarm/image sequence")
