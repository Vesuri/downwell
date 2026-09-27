#!/usr/bin/env python3
"""Verify the shared terrain Create property subset against the pinned VM export."""
import json
import re
from pathlib import Path

root = Path(__file__).resolve().parents[1]
export = root / "extracted/utmt-0.9.2.0"
manifest = json.loads((export / "manifest.json").read_text())
assert manifest["source"]["sha256"] == "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"
assert manifest["extractor"]["version"] == "0.9.2.0"

def instructions(name):
    return [s.strip() for s in (export / "Assembly" / (name + ".asm")).read_text().splitlines()
            if s.strip() and not s.startswith((".", ":"))]

wall = instructions("gml_Object_objWall_n_Create_0")
assert wall[:5] == ["pushglb.v global.wallTile", "pop.v.v self.sprite_index",
                    "pushglb.v global.glitchMode", "conv.v.b", "bf [2]"]
assert wall[5:17] == [op for sprite in (57, 55, 53, 52, 51)
                      for op in (f"pushi.e {sprite}", "conv.i.v")] + [
                          "call.i choose(argc=5)", "pop.v.v self.sprite_index"]
assert wall[17:21] == ['push.s "foliage"@776', "pop.v.s self.material", "pushi.e 0", "pop.v.i self.image_speed"]
assert wall[21:] == [op for name, value in (
    ("deactivate", 0), ("checked", 0), ("checkwall", 0), ("i", 0),
    ("grass", -1), ("surrounded", 0), ("lightCheck", 0), ("isPillar", 0),
    ("lightUpLeft", 0), ("lightUpRight", 0), ("lightDownLeft", 0), ("lightDownRight", 0))
    for op in (f"pushi.e {value}", f"pop.v.i self.{name}")]
for code, number, value in ((676, 1, -1), (677, 2, 1), (678, 3, -1)):
    name = f"gml_RoomCC_rmMain_{number}_Create"
    assert manifest["resources"]["code"][code]["name"] == name
    assert instructions(name) == [f"pushi.e {value}", "pop.v.i self.isPillar"]
thin = instructions("gml_Object_objThinPlatform_Create_0")
assert thin == ['push.s "thin"@775', "pop.v.s self.material",
                "pushglb.v global.wallTile", "pop.v.v self.sprite_index",
                "pushi.e 23", "pop.v.i self.image_index", "pushi.e 0", "pop.v.i self.image_speed"]
init = instructions("gml_GlobalScript_scrInitialize")
table = []
for i, op in enumerate(init):
    if op == "pop.v.v [array]self.levelTile":
        assert init[i-3:i-1] == ["conv.i.v", "pushi.e -5"]
        table.append((int(init[i-1].split()[-1]), int(init[i-4].split()[-1])))
assert table == list(enumerate((76, 51, 74, 72, 70, 55)))
header = (root / "src/core/TerrainCreate.h").read_text()
assert re.search(r"tiles\[\]=\{76,51,74,72,70,55\}", header)
assert re.search(r"choices\[\]=\{51,52,53,55,57\}", header)
print("Pinned VM terrain Create: tile table, glitch choice order, platform frame and stopped animation verified")
