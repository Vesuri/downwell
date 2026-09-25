#!/usr/bin/env python3
"""Check hand-authored rectangle adapters against the untouched-build export."""
import json
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
manifest = json.loads((root / "extracted/utmt-0.9.2.0/manifest.json").read_text())
assert manifest["source"]["sha256"] == "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"
assert manifest["extractor"]["version"] == "0.9.2.0"
header = (root / "src/core/EnemyContact.h").read_text()
records = re.findall(r"case (\d+): return \{(-?\d+), (-?\d+), (-?\d+), (-?\d+)\};", header)
assert {int(row[0]) for row in records} == {31, 133, 134, 153, 154, 157, 162, 163, 164, 166, 177, 186, 278, 460}
for row in records:
    sprite = manifest["resources"]["sprites"][int(row[0])]
    assert sprite["id"] == int(row[0])
    assert sprite["separateMasks"] == "AxisAlignedRect"
    expected = (sprite["marginLeft"] - sprite["originX"],
                sprite["marginRight"] - sprite["originX"] + 1,
                sprite["marginTop"] - sprite["originY"],
                sprite["marginBottom"] - sprite["originY"] + 1)
    assert tuple(map(int, row[1:])) == expected, (sprite["name"], row, expected)
for object_id, mask in ((35, 31), (113, 166), (117, 177), (119, 278)):
    assert manifest["resources"]["objects"][object_id]["textureMaskId"] == mask
assert manifest["resources"]["objects"][112]["spriteId"] == 186
assert manifest["resources"]["objects"][248]["spriteId"] == 133
assert manifest["resources"]["objects"][248]["textureMaskId"] == -1
assert manifest["resources"]["objects"][116]["spriteId"] == 153
assert manifest["resources"]["objects"][269]["spriteId"] == 460
assert manifest["resources"]["objects"][269]["textureMaskId"] == -1
assert manifest["resources"]["rooms"][11]["name"] == "rmMain"
assert manifest["resources"]["rooms"][11]["width"] == 480
print("14 enemy/player/currency/bone contact rectangles match pinned source metadata")
