#!/usr/bin/env python3
"""Verify shared area-one enemy birth constants against pinned VM assembly."""
from decimal import Decimal
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
assert len(profiles) == 6
assert {int(row[0]) for row in profiles} == {111, 112, 113, 116, 117, 119}
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
print("6 shared enemy birth profiles match pinned Create-event VM assembly")
