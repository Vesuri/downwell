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

marker = manifest["resources"]["objects"][99]
assert (marker["id"], marker["parentId"], marker["spriteId"], marker["textureMaskId"], marker["Visible"]) == (99, 60, 46, -1, False)
for suffix in ("Create_0", "Step_2"):
    assert instructions("gml_Object_Wall_" + suffix) == []
ancestor = 99
while ancestor >= 0:
    definition = manifest["resources"]["objects"][ancestor]
    for event in definition["events"]:
        if event["eventType"] == 14:
            for action in event["actions"]:
                assert instructions(action["codeName"]) in ([], ["call.i event_inherited(argc=0)", "popz.v"])
    ancestor = definition["parentId"]

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
platform = manifest["resources"]["objects"][93]
assert platform["parentId"] == 87 and platform["textureMaskId"] == 49
def family(parent):
    result = []
    for obj in manifest["resources"]["objects"]:
        ancestor = obj["id"]
        while ancestor >= 0 and ancestor != parent:
            ancestor = manifest["resources"]["objects"][ancestor]["parentId"]
        if ancestor == parent:
            result.append(obj["id"])
    return result
assert family(87) == [87, 93, 100, 318, 320, 324]
assert family(88) == [88, 94, 95, 96, 103]
main = next(room for room in manifest["resources"]["rooms"] if room["name"] == "rmMain")
assert not any(instance["objectId"] in family(87) + family(88) for instance in main["instances"])
mask = manifest["resources"]["sprites"][49]
assert mask["separateMasks"] == "AxisAlignedRect"
assert tuple(mask[k] for k in ("originX", "originY", "marginLeft", "marginTop", "marginRight", "marginBottom")) == (12, 12, 4, 4, 19, 5)
assert thin == ['push.s "thin"@775', "pop.v.s self.material",
                "pushglb.v global.wallTile", "pop.v.v self.sprite_index",
                "pushi.e 23", "pop.v.i self.image_index", "pushi.e 0", "pop.v.i self.image_speed"]

# The enemy probability draw precedes enemy-symbol dispatch for EVERY cell.
# Plain terrain is not a draw-free span: buffering those draws before Create
# changes the stream seen by glitch walls and any synchronous child callback.
cavern_text = (export / "Assembly/gml_GlobalScript_builderCavern.asm").read_text()
def cavern_block(number):
    body = cavern_text.split(f":[{number}]\n", 1)[1].split("\n:[", 1)[0]
    return [line.strip() for line in body.splitlines() if line.strip()]
assert cavern_block(12) == ["popz.v", "pushglb.v global.level", "pushi.e 2",
    "add.i.v", "call.i random(argc=1)", "pushglb.v global.level", "cmp.v.v LTE", "bt [17]"]
assert cavern_block(13) == ["pushglb.v global.hardMode", "conv.v.b", "bf [15]"]
assert cavern_block(14) == ["pushi.e 5", "conv.i.v", "call.i random(argc=1)",
    "pushi.e 1", "cmp.i.v GT", "b [16]"]
assert cavern_block(15) == ["push.e 0"]
assert cavern_block(16) == ["b [18]"]
assert cavern_block(17) == ["push.e 1"]
assert cavern_block(18) == ["bf [38]"]
assert cavern_block(19)[:3] == ["push.v self.i", "push.v self.levelStr", "call.i string_char_at(argc=2)"]
init = instructions("gml_GlobalScript_scrInitialize")
table = []
for i, op in enumerate(init):
    if op == "pop.v.v [array]self.levelTile":
        assert init[i-3:i-1] == ["conv.i.v", "pushi.e -5"]
        table.append((int(init[i-1].split()[-1]), int(init[i-4].split()[-1])))
assert table == list(enumerate((76, 51, 74, 72, 70, 55)))
for sprite in {76, 51, 74, 72, 70, 55, 52, 53, 57}:
    art = manifest["resources"]["sprites"][sprite]
    expected = (32, 32, 16, 16) if sprite == 70 else (16, 16, 8, 8)
    assert (art["width"], art["height"], art["originX"], art["originY"]) == expected

# Pin the local event surface before treating terrain as stationary. This is
# deliberately not a whole-program proof: external with/setter paths and the
# runner's implicit movement/animation are outside this check.
for object_id, parent, event_keys in (
    (56, -1, {(14, 0)}), (57, 56, {(14, 0)}),
    (59, 57, {(14, 0)}),
    (60, 59, {(0, 0), (3, 2), (8, 0), (14, 0)}),
    (87, 56, {(0, 0), (14, 0)}),
    (93, 87, {(0, 0), (3, 2), (14, 0)}),
    (99, 60, {(0, 0), (3, 2), (14, 0)}),
):
    definition = manifest["resources"]["objects"][object_id]
    assert definition["parentId"] == parent
    assert {(e["eventType"], e["eventSubtype"]) for e in definition["events"]} == event_keys
    for event in definition["events"]:
        assert len(event["actions"]) == 1
        if event["eventType"] == 14:
            assert instructions(event["actions"][0]["codeName"]) in (
                [], ["call.i event_inherited(argc=0)", "popz.v"])

wall_end = instructions("gml_Object_objWall_n_Step_2")
# Whitelist every explicit store, not just a search for absent x/y writes.
assert {op.split()[-1] for op in wall_end if op.startswith("pop.")} == {
    "self.checked", "self.checkThing", "self.i", "self.lightUpLeft",
    "self.lightUpRight", "self.lightDownLeft", "self.lightDownRight",
    "self.chkdist", "self.surrounded", "self.image_index",
}
assert {op for op in wall_end if op.startswith("call.")} == {
    "call.i gml_Script_scrOutofview(argc=0)",
    "call.i gml_Script_tile_add(argc=8)", "call.i instance_destroy(argc=0)",
    "call.i @@This@@(argc=0)", "call.i instance_deactivate_object(argc=1)",
    "call.i place_meeting(argc=3)", "call.i position_meeting(argc=3)",
    "call.i choose(argc=3)", "call.i gml_Script_instance_create(argc=3)",
}
assert instructions("gml_Object_objThinPlatform_Step_2") == [
    "call.i gml_Script_scrOutofview(argc=0)", "popz.v"]
wall_draw = instructions("gml_Object_objWall_n_Draw_0")
assert not any(op.startswith("pop.") for op in wall_draw)
assert [op for op in wall_draw if op.startswith("call.")] == [
    "call.i draw_self(argc=0)"] + ["call.i draw_sprite(argc=4)"] * 4
marker_art = manifest["resources"]["sprites"][46]
assert len(marker_art["frames"]) == 1
header = (root / "src/core/TerrainCreate.h").read_text()
assert re.search(r"tiles\[\]=\{76,51,74,72,70,55\}", header)
assert re.search(r"choices\[\]=\{51,52,53,55,57\}", header)
print("Pinned VM terrain Create: tile table, glitch choice order, platform frame and stopped animation verified")
print("Terrain local-event audit: ancestry, explicit End Step stores/calls and Draw calls verified; external mutations not proven")
print("Terrain batching boundary: per-cell probability RNG precedes symbol dispatch; hard mode has an additional conditional draw")
