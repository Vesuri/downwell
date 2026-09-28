#!/usr/bin/env python3
"""Pin the selected Windows runner's initial dynamic instance-ID counter."""
from hashlib import sha256
from pathlib import Path
import runpy
import struct
import json

root = Path(__file__).resolve().parents[1]
# Reuse the hash-checked PE reader; this also validates the selected executable.
region = runpy.run_path(str(root / "tools/verify-runner-choose.py"))["region"]
data = (root / "data/data.win").read_bytes()
assert sha256(data).hexdigest() == "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"
assert data[8:12] == b"GEN8"
assert struct.unpack_from("<I", data, 16 + 12)[0] == 105186

# GEN8 loader passes payload+12 to the counter setter, not a derived room max.
assert region(0x497a95, 8) == bytes.fromhex("ff770c e883f5ffff")
assert region(0x497020, 10) == bytes.fromhex("8b442404 a3285b9400 c3")
# Dynamic depth allocation loads, increments, stores and passes the new ID
# before constructing the instance. Counter address is shared with the setter.
assert region(0x487928, 6) == bytes.fromhex("8b0d285b9400")
assert region(0x487933, 13) == bytes.fromhex("41 6a01 ff7514 890d285b9400 51")
assert region(0x487955, 5) == bytes.fromhex("e8969dfdff")
# The constructor writes the ID argument into the instance's numeric-ID field.
assert region(0x4618f8, 3) == bytes.fromhex("8b4510")
assert region(0x46190b, 3) == bytes.fromhex("89467c")
print("Pinned GEN8.LastObj counter and pre-construction increment: first runtime ID 105187")

# The counter cannot be reconstructed by numbering just visible actors. Pin
# the authored main-room population and the reviewed player child-birth path.
# This is source evidence, NOT a test that either live adapter implements it.
export = root / "extracted/utmt-0.9.2.0"
manifest = json.loads((export / "manifest.json").read_text())
assert manifest["source"]["sha256"] == sha256(data).hexdigest()
assert manifest["extractor"]["version"] == "0.9.2.0"
assert manifest["extractor"]["commit"] == "03c7048438347799e0717b6e57a9358d000472e0"
room = manifest["resources"]["rooms"][11]
assert room["name"] == "rmMain"
assert len(room["instances"]) == len(room["instanceCreationOrder"]) == 244
authored = {i["instanceId"]: i for i in room["instances"]}
assert len(authored) == 244
assert [(i["index"], i["instanceId"], i["objectId"])
        for i in room["instanceCreationOrder"] if i["objectId"] != 60] == [
    (0, 100856, 65), (1, 100855, 35), (2, 100854, 61), (147, 101097, 371)]
for i in room["instanceCreationOrder"]:
    assert authored[i["instanceId"]]["objectId"] == i["objectId"]

def assembly(name, digest):
    payload = (export / "Assembly" / (name + ".asm")).read_bytes()
    assert sha256(payload).hexdigest() == digest, name
    return payload.decode()

player = assembly("gml_Object_objPlayer_n_Create_0",
    "8298b2249fec2b4226f43d257335176ea0f6282bf89dfc5aab5be2c700e88e3b")
init = assembly("gml_GlobalScript_scrPlayerInit",
    "f17aa6e41f1a28c05adc914c0da94699745d8407783ca0ec0418d2b5dc14b7b8")
spawn_camera = assembly("gml_GlobalScript_scrSpawnCamera",
    "e95f81ae81762898b5db31847cdb37d0109817dbf966e2aefb1df8abeb9e05b4")
camera = assembly("gml_Object_camMain_Create_0",
    "af5974437a0ca17493d660e850e3bc585816642034457b23ea7ae91037cfd78e")
ground = assembly("gml_GlobalScript_groundRoom",
    "9708c07856cb976b1606c27023f8aa1b3fa9370fe38114df9b8361dbce39196d")
for room_id in range(20,25):
    assert f"pushi.e {room_id}\ncmp.i.v EQ" in ground
assert "pushi.e 0\npop.v.i self.freeCam" in camera
assert "pushi.e 0\npop.v.i self.autoScroll" in camera
parent = assembly("gml_Object_parentLife_Create_0",
    "19838825dc0e49ada1d670de755ba9942ab622914a87d8d4d2acb51eae4c3c04")
objects = manifest["resources"]["objects"]
assert objects[35]["parentId"] == 34 and objects[34]["parentId"] == -1
assert "instance_create" not in parent
assert player.index("scrPlayerInit") < player.index("event_inherited") < player.index("pushi.e 66")
assert player.index("pushi.e 66") < player.index("scrSpawnCamera") < player.index("pushi.e 73")
assert init.count("call.i gml_Script_instance_create(argc=3)") == 3
assert "pushglb.v global.pugDecoy" in init and "pushglb.v global.pugDrone" in init
assert "pushi.e 64" in spawn_camera and "pop.v.v self.myCam" in spawn_camera
# Camera Create does not publish the view; the entry mask reads the separate
# view getter, not camera.y minus an assumed centering offset.
assert "___view_set" not in camera
for suffix, digest in {
    "Create_0": "ba2ef8da3dc52e7136cb782be9aefae0a6c357434d1db7f65dfe86956d6d08b3",
    "Step_0": "d4118438897b8938def8773e4a745f47aa337a911f8a298e9371549ba4e024a1",
    "Alarm_0": "7fa420002321759f60d9103f49806b50a0673341e139e47b46bb18672ccf6d78",
    "Draw_0": "5412ac56b4c0f354833463a24582d31311d81cf8d2eca2d3e97f2ed9eb2c81d6",
}.items():
    assembly("gml_Object_objEnterDither_" + suffix, digest)
assert objects[73]["name"] == "objEnterDither" and objects[73]["spriteId"] == 129
surface_draw = assembly("gml_GlobalScript_drawOnSurface",
    "8b11cc827e9e620ebe4cd822370493a0211d3abd43ad8eb257304e3c1e0d5cde")
assert "call.i surface_exists(argc=1)" in surface_draw
assert "call.i draw_sprite_ext(argc=9)" in surface_draw
assert "scrInView" not in surface_draw and "global.death" not in surface_draw and "global.lowSpec" not in surface_draw
assert surface_draw.index("push.v self.y") < surface_draw.index("push.v self.x")
print("Pinned rmMain's 240 walls/four controllers and player child-birth order; live allocation parity remains unverified")
