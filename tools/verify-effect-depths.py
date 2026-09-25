#!/usr/bin/env python3
"""Check effect ordering against compatibility VM tables, not OBJT defaults."""
from hashlib import sha256
from pathlib import Path
import json
import re

root = Path(__file__).resolve().parents[1]
export = root / "extracted/utmt-0.9.2.0"
manifest = json.loads((export / "manifest.json").read_text())
assert manifest["source"]["sha256"] == "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"

def assembly(name, digest):
    payload = (export / "Assembly" / (name + ".asm")).read_bytes()
    assert sha256(payload).hexdigest() == digest, name
    return payload.decode()

table = assembly("gml_GlobalScript___global_object_depths",
    "b0e0b7b103e755a99737f1d4d1ffb2143311939dc9a82dd7ab80231a90bea300")
assembly("gml_GlobalScript_instance_create",
    "a653a868d31b5cbdfdfc40aca69f7046df1b2d0939518f4561400e5c241912f8")
assembly("gml_GlobalScript_object_get_depth",
    "9a8877605cae032b31b4fd9b39e396660fa68da8d06c235e1d9fd0a5c3f8909b")
assembly("gml_Object_fxFlashBall_Create_0",
    "2e2f59f59ae59d5b18f04ae661fa0d1eab540e253280c09ce56671f768ea7d44")
currency = assembly("gml_Object_objCurrency_Draw_0",
    "db549eaf1588afac76046e7d4c13374cf40e8975d5e0cca7384e57d7e968f9bd")
assert "call.i gml_Script_drawFx(argc=0)" in currency and "draw_self" not in currency
# The reviewed table tail maps names via asset_get_index. These indices are
# NOT resource IDs; pairing by array index before resolving names is essential.
depths = {int(index): int(value) for value, index in re.findall(
    r"push(?:i\.e|\.i) (-?\d+)\nconv\.i\.v\npushi\.e -5\n"
    r"pushi\.e (\d+)\npop\.v\.v \[array\]self\.__objectDepths", table)}
names = {int(index): name for name, index in re.findall(
    r'push\.s "([^"]+)"@\d+\nconv\.s\.v\npushi\.e -5\n'
    r"pushi\.e (\d+)\npop\.v\.v \[array\]self\.__objectNames", table)}
assert len(names) == len(depths) == 342 and names.keys() == depths.keys()
by_name = {name: depths[index] for index, name in names.items()}
expected = {"objControlerN": -300000, "fxFlashBall": -200000,
            "objPlayer_n": -50000, "bulletStandard": 0,
            "bulletEnmBone": 0, "fxSmoke": 1000, "objCurrency": 10000,
            "enmSkeleton": 14000}
assert all(by_name[name] == depth for name, depth in expected.items())
objects = {obj["name"]: obj for obj in manifest["resources"]["objects"]}
flash = objects["fxFlashBall"]
assert flash["id"] == 224 and flash["parentId"] == -1 and flash["Visible"]
assert all(event["eventType"] != 8 for event in flash["events"]) # default Draw
room = next(room for room in manifest["resources"]["rooms"] if room["name"] == "rmInit")
controller = next(instance for instance in room["instances"] if instance["objectId"] == objects["objControlerN"]["id"])
layer = next(layer for layer in room["layers"] if controller["instanceId"] in layer["instanceIds"])
assert layer["depth"] == by_name["objControlerN"]
assert objects["objControlerN"]["Persistent"]
assert by_name["objControlerN"] < by_name["fxFlashBall"] < by_name["objPlayer_n"]
print("Source effect depths: controller -300000, flash -200000, player -50000; 342 VM mappings checked")
