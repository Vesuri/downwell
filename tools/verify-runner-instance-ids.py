#!/usr/bin/env python3
"""Pin the selected Windows runner's initial dynamic instance-ID counter."""
from hashlib import sha256
from pathlib import Path
import runpy
import struct
import json
import re

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

# Shop ordinary Step may capture registered membership without pending Shop
# candidates: there are no object changes anywhere in this exact corpus, and
# the only whole-room/region activation calls are reconciled Begin Steps.
# Other callers deactivate their newly-created box/enemy or the wall itself.
activation_roots = {
    "gml_GlobalScript_scrBuilder": "0c95048af06ff612499acd64a717766441b820d19ffc6017c0ff3a95425581f5",
    "gml_GlobalScript_spawnerStep": "793947ac8f003f7a32b0f2b66d449af53a20600b196f9a8b563852ce43f048fb",
    "gml_Object_enmBat_Create_0": "e80ce038ee2ea894a97991c294541c4cd1f1b6d74e426e6222313721ab04632a",
    "gml_Object_objControlerN_Step_1": "b5897552335417df74ce906851b290990cb47082df34b344dc20b84d30e43a4f",
    "gml_Object_objPlayer_n_Step_1": "46a8178a1b7cf78ff8f14f204047096d00717ea59da6e2cd74d908cf1932fed5",
    "gml_Object_objWall_n_Step_2": "782c095938c35cbc94a2163b39379ce1957126deec5731201faba9b9faf0e49d",
}
roots = list((export / "Assembly").glob("*.asm"))
assert len(roots) == 2501
actual_activation_roots = set()
for path in roots:
    text = path.read_text()
    assert not re.search(r"call\..*\b(?:gml_Script_)?instance_change\(", text), path.name
    assert not re.search(r"^pop\..*\bobject_index\b", text, re.M), path.name
    if re.search(r"call\..*\b(?:gml_Script_)?instance_(?:activate|deactivate)_", text):
        actual_activation_roots.add(path.stem)
assert actual_activation_roots == set(activation_roots)
for name, digest in activation_roots.items():
    assembly(name, digest)
assert not any(o["parentId"] == 277 for o in manifest["resources"]["objects"])
print("Pinned Shop ordinary-Step capture domain: no descendants, type changes or pending Shop activation callers")

# Player and enemy-target globals are distinct when a decoy is selected.
assembly("gml_Object_enmWhiteJelly_Create_0",
    "d23276a6a24d6e69159f8f8a2bde523119b594f5336d1e17a739d4e2116e312e")
assembly("gml_Object_enmWhiteJelly_Step_0",
    "5368114688f29a938da820781c3ed7852643ca1a873b26e99419ffc930c118f7")
assembly("gml_GlobalScript_scrPlMovement",
    "3ff0e28b9dcef221ed9dd6a1ceb801bef746d7538016547fbe49a60af0177644")
assembly("gml_GlobalScript_scrPlayerGlobalStat",
    "34dfd0f192ca34217da36cdfd001b792a3e7d496d3fc8c3e9967f8ce7ff2266a")
builder = assembly("gml_GlobalScript_scrBuilder",
    "0c95048af06ff612499acd64a717766441b820d19ffc6017c0ff3a95425581f5")
assert (authored[100856]["x"], authored[100856]["y"]) == (-16, 704)
assert ("pushi.e 78\nconv.i.v\npush.v self.y\npush.v self.x\n"
        "pushi.e 100\nsub.i.v\ncall.i gml_Script_instance_create(argc=3)") in builder
# The newline and extra-pillar loops each publish a row before continuing;
# updating just once at pattern end changes synchronous child observations.
for label in (187,207):
    assert f":[{label}]\npush.v self.y\npushi.e 16\nadd.i.v\npop.v.v self.y" in builder

spawner = manifest["resources"]["objects"][78]
assert (spawner["name"], spawner["parentId"], spawner["spriteId"]) == ("Spawner", -1, 702)
spawner_create = assembly("gml_Object_Spawner_Create_0",
    "18c953fa8b664ee259c633e56ea906128bfad370792bf20234250213c9d07627")
spawner_event = assembly("gml_Object_Spawner_Step_0",
    "c4b6b170447274b021385cd01836f998a4744d4acf523112193453d644d8811a")
assert "call.i gml_Script_spawnerInit(argc=0)" in spawner_create
assert "call.i gml_Script_spawnerStep(argc=0)" in spawner_event
spawner_init = assembly("gml_GlobalScript_spawnerInit",
    "c1c81d5b355ecde8291ee261518dc5c1deec03647e47b9ffe1a24721a2257ce5")
spawner_set = assembly("gml_GlobalScript_spawnerSet1",
    "f2f87b87c3567f5840210c9f417372d1fcf55f1ac76afd0ca599b0ce1ad0d74b")
spawner_step = assembly("gml_GlobalScript_spawnerStep",
    "793947ac8f003f7a32b0f2b66d449af53a20600b196f9a8b563852ce43f048fb")
# Script active is not runner activation. The spawner remains scheduled while
# waiting for the player, and newborn enemies deactivate only AFTER Create.
assert "pushi.e 0\npop.v.i self.active" in spawner_init
assert "instance_deactivate_object" not in spawner_init
assert "pushi.e 20\npop.v.i self.enemyMaxCount" in spawner_set
assert "pushi.e 115\nconv.i.v" in spawner_set
assert spawner_step.count("call.i gml_Script_instance_create(argc=3)") == 2
assert spawner_step.count("call.i instance_deactivate_object(argc=1)") == 2
for create_label, with_label in ((33,34),(55,56)):
    birth = spawner_step.index(f":[{create_label}]")
    enter = spawner_step.index(f":[{with_label}]", birth)
    assert "gml_Script_instance_create" in spawner_step[birth:enter]
    assert "call.i @@This@@(argc=0)\ncall.i instance_deactivate_object(argc=1)" in spawner_step[enter:]
print("Pinned Spawner 78: depth-driven channels and synchronous Create-before-deactivate")

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
assert init.index("pushi.e 256") < init.index("pushi.e 0\npop.v.i global.droneNum") < init.index("pushi.e 249")
assert init.index("pushi.e 249") < init.index("pushi.e 1\npop.v.i global.droneNum") < init.rindex("pushi.e 249")
assert init.rindex("pushi.e 249") < init.index("pop.v.i self.xdir") < init.index("pop.v.i self.ydir")
drone = assembly("gml_Object_objDrone_Create_0",
    "6b529f4739e3836ae65bcdafa5049e276edebb106ca5284225884b85ca641847")
balloon = assembly("gml_Object_decoyBalloon_Create_0",
    "102112ec0a83ccabf47a904b9b7ed7272ad8b9fb184a663d55ef133a0b5e8c74")
assert objects[249]["parentId"] == objects[256]["parentId"] == -1
assert "call.i" not in drone and "call.i" not in balloon
for name,digest,field in (
    ("gml_Object_objDrone_Alarm_3","e14dac0a7386d3f986e505aa6a23ba025b66de1ae6796ef8eb1e098f17370010",None),
    ("gml_Object_objDrone_Alarm_2","d24bc9212a9e4e04569c2db8701c73a98781c5b453e2dc4a9ff0487eba11a175","shotDelay"),
    ("gml_Object_decoyBalloon_Alarm_1","b5f534ec4da2c4f3cf6f841a4fbb21b2b3846c8995fa505c362c2154c1f32047","hitStun"),
    ("gml_Object_decoyBalloon_Alarm_0","358740a702888fe7f2ef47f57b3d411c37fd270ab5ff4de9870cf7fddddfa32d","noHold")):
    body=assembly(name,digest)
    instructions=[line for line in body.splitlines() if line and not line.startswith((".",":"))]
    assert instructions==(["pushi.e 0",f"pop.v.i self.{field}"] if field else [])
assert "push.d 0.5\npop.v.d self.image_speed" in drone
balloon_end=assembly("gml_Object_decoyBalloon_Step_2",
    "53c564f09cf941f9abb926572094437a9ea768125a2d17f17d7b4412c5d0ef0e")
assert [line for line in balloon_end.splitlines() if line and not line.startswith((".",":"))]==[
    "pushglb.v global.pugDecoy","conv.v.b","not.b","bf [end]",
    "call.i instance_destroy(argc=0)","popz.v"]
assert "push.d 0.08\npop.v.d self.imgSp" in balloon
assert "push.v self.x\npop.v.v self.xx\npush.v self.y\npop.v.v self.yy" in balloon
for object_id, sprite_id, frame_count in ((249,131,2),(256,526,4)):
    obj=objects[object_id]
    assert obj["spriteId"]==sprite_id
    assert len(manifest["resources"]["sprites"][sprite_id]["frames"])==frame_count
    assert not any(e["eventType"]==7 and e["eventSubtype"]==7 for e in obj["events"])
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
# Authored controller 61 is a wall-destroying actor, not an inert ID holder.
above = {}
for suffix, digest in {
    "Create_0": "0bf94b6a4cb4e3f6a0f562a7daf7408f28844e72305c1f037373df827a8fca2f",
    "Step_1": "7f21aa839eb0703dd28b2af312bf6ef05521f88fdff1a72571a476e05cb5cfb7",
    "Collision_objWall_n": "63e3ce6fbd22f939746b1a4f2423f730fba703c57c8dcc6705828e95af015aa1",
    "Draw_0": "911cbed4afad8a654d071ebc7485c7a8399e1c168762b838ab9a34036071e15e",
    "PreCreate_0": "4c193a4f963773b2c70b60323cfcdd763b259c208f342702a253d03b3f9e966b",
}.items():
    above[suffix] = assembly("gml_Object_objDitherAbove_" + suffix, digest)
assert objects[61]["parentId"] == 59 and objects[61]["spriteId"] == 128
events = {(e["eventType"], e["eventSubtype"]): [a["codeId"] for a in e["actions"]]
          for e in objects[61]["events"]}
assert events[(3, 1)] == [990] and events[(4, 60)] == [991]
assert "push.v 63.x\npop.v.v self.x" in above["Step_1"]
assert "pushi.e 272\ncmp.i.v GTE" in above["Step_1"]
assert "pushi.e 208\ncmp.i.v GTE" in above["Step_1"]
assert above["Step_1"].count("pushi.e 16\nadd.i.v\npop.v.v self.y") == 2
assert above["Step_1"].index("call.i round(argc=1)") < above["Step_1"].index("pushglb.v global.ending")
assert above["Collision_objWall_n"].index("call.i @@Other@@(argc=0)") < above["Collision_objWall_n"].index("pushenv")
assert "call.i instance_destroy(argc=0)" in above["Collision_objWall_n"]
assert "call.i draw_self(argc=0)" in above["Draw_0"]
print("Pinned authored dither Begin Step and wall-destruction events; an inert controller is not equivalent")
print("Pinned rmMain's 240 walls/four controllers and player child-birth order; live allocation parity remains unverified")

# Shop creation is not just stock RNG and static drawing. Its ten children
# consume the same numeric stream as the enclosing builder and its next cell.
shop = assembly("gml_Object_Shop_Create_0",
    "670e7f398a89a720f63eae31b674023ea06e2ae1b6f45be5560349b5f0a7fb3a")
assert objects[277]["parentId"] == 91
shop_step = assembly("gml_Object_Shop_Step_0",
    "ced1552ae520650536d0a6ec2a9c96d183c3a6401c5b5c33d4a0de790950b8b4")
step_binding = [e for e in objects[277]["events"] if (e["eventType"],e["eventSubtype"]) == (3,0)]
assert len(step_binding) == 1 and step_binding[0]["actions"][0]["codeName"] == "gml_Object_Shop_Step_0"
assert shop_step.startswith(".localvar 2 arguments\n\n:[0]\npushi.e 277\npushenv [126]\n")
assert shop_step.endswith(":[126]\npopenv [1]\n\n:[end]")
assert ":[125]\ncall.i gml_Script_scrCheckThisSignStep(argc=0)\npopz.v" in shop_step
assert shop_step.count("pop.v.i global.dUp") == 2
assert shop_step.count("pop.v.i global.padCancel") == 1
for block in (43,48,61,66):
    body = shop_step.split(f":[{block}]\n",1)[1].split(":[",1)[0]
    assert body.index("call.i gml_Script_soundPlayOL") < body.index("pop.v.i self.wfxx")
purchase = shop_step.split(":[76]\n",1)[1].split(":[81]\n",1)[0]
ordered = ["pop.v.i self.thank", "popaf.e", "call.i gml_Script_scrGainUg", "pop.v.v global.currency",
           "pop.v.i [stacktop]self.storedItem", "call.i instance_destroy", "pop.v.i [stacktop]self.thankful",
           "pop.v.i [stacktop]self.image_index", "call.i gml_Script_scrRisingTextShop", "call.i gml_Script_soundPlayOL"]
assert [purchase.index(s) for s in ordered] == sorted(purchase.index(s) for s in ordered)
print("Pinned Shop Step: per-event with(277) traversal, shared input writes and synchronous purchase ordering")
view = shop_step.split(":[96]\n",1)[1].split(":[114]\n",1)[0]
assert view.count("call.i gml_Script___view_get(argc=2)") == 6
assert "scrInView" not in view
for block,coordinate,comparison in ((96,"x","GT"),(97,"x","LT"),(98,"y","GT"),(99,"y","LT")):
    body = shop_step.split(f":[{block}]\n",1)[1].split(":[",1)[0]
    assert body.startswith(f"push.v self.{coordinate}\n") and f"cmp.v.v {comparison}" in body
for block,latch,sound in ((103,"shopInView",103),(110,"keeperAngry",94)):
    body = shop_step.split(f":[{block}]\n",1)[1].split(":[",1)[0]
    assert body.index(f"pop.v.i self.{latch}") < body.index("call.i gml_Script_soundPlayOL")
    assert f"pushi.e {sound}\nconv.i.v\ncall.i gml_Script_soundPlayOL" in body
assert "push.v self.keeperx\ncmp.v.v GT" in view
assert "pushi.e 0\npop.v.i self.keeperAngry\npushi.e 0\npop.v.i self.shopInView" in shop
print("Pinned shop greeting/anger latches, strict view edges and six ordered view reads")
tail = shop_step.split(":[114]\n",1)[1].split(":[126]\n",1)[0]
assert tail.startswith("pushglb.v global.criminal\nconv.v.b\nbt [116]")
assert tail.count("pop.v.i global.noControl") == 1
assert tail.index("pop.v.i self.mask_index") < tail.index("pop.v.i global.noControl") < tail.index("call.i gml_Script_scrCheckThisSignStep")
prompt = assembly("gml_GlobalScript_scrCheckThisSignStep",
    "ef914eeede814890ecbc7c2ce347b75f127c74f2b17d01b028addfed172d9891")
assert prompt.index("call.i place_meeting") < prompt.index("pushglb.v global.interactable")
assert "pushi.e 2\ncmp.i.v LT" in prompt and "pushi.e 2\ncmp.i.v GT" in prompt
assert "push.d 5.1\ncmp.d.v LTE" in prompt
print("Pinned shop crime/exit/sign tail and prompt VM comparison gates")
inherited = assembly("gml_Object_parentInteractable_Create_0",
    "c7501948045f88afa9ab289ee4b1b5a380bb4de07aee3857eeba1e937a427647")
assert "call.i" not in inherited and objects[91]["parentId"] == -1
radio = shop[:shop.index("pushi.e 278\nconv.i.v")]
assert "pushi.e 0\npop.v.i self.shopBgm" in radio
assert "pushglb.v global.area\npushi.e 5\ncmp.i.v NEQ\nbf [5]" in radio
assert "pushglb.v global.area\npushi.e 3\ncmp.i.v NEQ\nbf [3]" in radio
for sound in (192,201):
    assert f"pushi.e 1\nconv.i.v\npushi.e 100\nconv.i.v\npushi.e {sound}\nconv.i.v\ncall.i audio_play_sound(argc=3)" in radio
assert "pushi.e 0\nconv.i.v\npushi.e 0\nconv.i.v\npush.v self.radio\ncall.i audio_sound_gain(argc=3)" in radio
prefix, stock = shop.split(":[8]\n", 1)
assert prefix.count("call.i gml_Script_instance_create(argc=3)") == 7
assert prefix.count("pushi.e 279\nconv.i.v") == 3
cursor = 0
for object_id in (278, 279, 279, 279, 98, 280, 212):
    cursor = prefix.index(f"pushi.e {object_id}\nconv.i.v", cursor) + 1
assert "call.i irandom_range" not in prefix
assert "push.v self.keeper\npushi.e -9\npush.v [stacktop]self.x\npushi.e 2\nadd.i.v\npop.v.v self.msgx" in prefix
assert prefix.rindex("call.i gml_Script_instance_create(argc=3)") < prefix.index("pop.v.v self.msgx")
assert stock.index("call.i irandom_range") < stock.index("pushi.e 283\nconv.i.v")
item_loop = stock[stock.index(":[20]\n"):stock.index(":[28]\n")]
assert "push.v self.i\npushi.e 3\ncmp.i.v LTE" in item_loop
assert item_loop.count("call.i gml_Script_instance_create(argc=3)") == 1
assert item_loop.index("call.i gml_Script_instance_create") < item_loop.index("self.whichUg") < item_loop.index("self.storedItem")
assert "self.glassCase" in item_loop and "[stacktop]self.y\npushi.e 1\nadd.i.v" in item_loop
assert stock.index(":[28]\n") < stock.index("call.i gml_Script_scrShopAssignText")
for name, digest in (
    ("shopTable", "fcbb4a5f57c8976eebfc816c58e1fc80f622ab0681089f7d7d532f6d57e09c8c"),
    ("shopCase", "216b2c6928f6dd9ad53a373d12af0b320a912ee033f067aad19c6976f116d083"),
    ("shopKeeper", "7abd29cc17b2cbdcae4fc7cbef96de917c4afc912bc34ef3aded0d174c0e3210"),
    ("objObtainableUg", "b488cae24ca6095c5e158ec67c6424b19d83b8642d60289d4ae05b6b2c6ec29f"),
    ("fxRisingText", "5da331894603ae60423df75c122cb4d96eeec816d392626870409eb999a2fb66")):
    child = assembly("gml_Object_" + name + "_Create_0", digest)
    assert "call.i" not in child  # These Create bodies add no nested births/RNG.
text = assembly("gml_GlobalScript_scrShopAssignText",
    "732a1cab1f2ef3db8feaa5929cc36979ec04c62f3da5806f2365418e3d957a3f")
assert text.count("call.i choose(argc=3)") == 1
assert "gml_Script_instance_create" not in text
print("Pinned seven pre-stock shop children and three post-stock item births before farewell RNG")
# Child Create admission must include the inherited chain, not only the leaf.
for object_id, parent_id in ((277,91),(91,-1),(278,59),(279,57),(98,59),(280,-1),(212,-1),(283,-1),(59,57),(57,56),(56,-1)):
    definition = objects[object_id]
    assert definition["parentId"] == parent_id
    assembly("gml_Object_" + definition["name"] + "_PreCreate_0",
             "4c193a4f963773b2c70b60323cfcdd763b259c208f342702a253d03b3f9e966b")
    if object_id in (59,57,56):
        assert not any(e["eventType"] == 0 for e in definition["events"])
invisible = assembly("gml_Object_objInvisWall_Create_0",
    "ecdc8901c3fa1d986b23cc44ef9d6c7c133222fd45681d8d96639a7c2c391f40")
assert invisible.count("call.i") == 1 and "call.i event_inherited(argc=0)" in invisible
assert 'push.s "rock"@777\npop.v.s self.material' in invisible
print("Pinned shop child PreCreate ancestry and invisible-wall inherited Create")
speech = assembly("gml_GlobalScript_scrRisingTextShop",
    "a623aca85d6834a1fb1377125bca379173891dbe85f6ab9f2324a5a7ca7db20a")
sequence = ["pop.v.v self.basex", "pop.v.v self.basey", "pop.v.v self.drawText",
            "push.v self.myRisingText", "call.i instance_destroy(argc=0)",
            "call.i gml_Script_instance_create(argc=3)", "pop.v.v self.myRisingText",
            "pop.v.v [stacktop]self.text"]
positions = [speech.index(token) for token in sequence]
assert positions == sorted(positions)
assert objects[212]["parentId"] == -1 and not any(e["eventType"] == 1 for e in objects[212]["events"])
assembly("gml_GlobalScript_instance_create",
    "a653a868d31b5cbdfdfc40aca69f7046df1b2d0939518f4561400e5c241912f8")
print("Pinned shop speech: caller fields, previous-text destroy, fresh fractional birth, then text assignment")
rising_step = assembly("gml_Object_fxRisingText_Step_0",
    "c1f478f143645d40945f8424c5051635b9f711099afb2dac73b2d75bbc18a96d")
assert rising_step.index("pop.v.v self.y\n") < rising_step.index("pop.v.v self.ysp\n") < rising_step.index("pop.v.v self.drawing")
assert "push.v self.drawing\npushi.e -1\nmul.i.v" in rising_step
assert "call.i" not in rising_step
assembly("gml_Object_fxRisingText_Alarm_0",
    "7fa420002321759f60d9103f49806b50a0673341e139e47b46bb18672ccf6d78")
assembly("gml_Object_fxRisingText_Alarm_1",
    "a26b8b83ab354c399e30bee6a3ccd72da89e8fec98436de8b7ac320b3d76ecc7")
assembly("gml_GlobalScript_action_kill_object",
    "caac5c9d9f9d959a5aa6337f729bfc989927d8882d4ce7f9a29b9d45dd0eab33")
print("Pinned rising-text Step and alarm-driven destruction")
keeper_alarm0 = assembly("gml_Object_shopKeeper_Alarm_0",
    "e7d4df117c713ab12670714a4d172050901ec0c3b479e06fa0e0032f8da0f65e")
keeper_alarm1 = assembly("gml_Object_shopKeeper_Alarm_1",
    "82f46673e07811bcbfb63dd3e160c83084749cd16dec8d8e4d093aaee5eaa579")
assembly("gml_Object_shopKeeper_Alarm_2",
    "7b4985a324acc8f226adbd70211e4118cb31df7f5a32819ff5c16ba46bb16e47")
assert "pushi.e 5\nconv.i.v\npushi.e -1\npushi.e 1\npop.v.v [array]self.alarm" in keeper_alarm0
assert "call.i irandom_range(argc=2)\npushi.e -1\npushi.e 0\npop.v.v [array]self.alarm" in keeper_alarm1
print("Pinned keeper blink alarms: later-slot rearm precedes same-phase countdown")
keeper_step = assembly("gml_Object_shopKeeper_Step_0",
    "e2764c44a5e9db85beac82c38bdb12104c5087850fd49f668edb25e08c2319eb")
assert keeper_step.index("call.i instance_destroy(argc=0)") < keeper_step.index("pushi.e 284")
assert keeper_step.index("pushi.e 284") < keeper_step.rindex("pushglb.v global.area")
assert "push.v self.image_speed\npushi.e 2\ndiv.i.v\npop.v.v self.image_speed" in keeper_step
rising_draw = assembly("gml_Object_fxRisingText_Draw_0",
    "b3f4de1e393b0608999219ff82fba8a18679e5f526e3295f4b440e283cf6c7cc")
assert rising_draw.index("self.overXview\npushi.e -10\ncmp.i.v GT") < rising_draw.index("self.underXview\npushi.e 10\ncmp.i.v LT")
assert "push.v self.drawing\nconv.v.b\nbf [6]" in rising_draw
assert rising_draw.count("call.i gml_Script___view_get(argc=2)") == 3
assembly("gml_GlobalScript_scrDrawBorderTextBlack",
    "fa8d7e4e10625d3474e6ca2e2d1ed674d5c25c3ccc2b23a264215ca22bf93d6b")
item_step = assembly("gml_Object_objObtainableUg_Step_0",
    "2251be9cb63fac1d63d62b287e5431acda66055646f21ddb3a7dc749b4bc85b8")
assert item_step.count("push.v self.whichUg") == 2
assert "pop.v.v self.sprite_index" in item_step and "image_index" not in item_step
assert objects[283]["parentId"] == -1
case_step=assembly("gml_Object_shopCase_Step_0",
    "6c3d60b9040966633bd75afcf70b8c84da6a087e02a177e8b8bf3f017c3fc707")
assembly("gml_Object_Shop_Step_2",
    "e124d7e7d595c81e9e222a01e8570eb9f95ad58bdc37b111cc7e5909a861ea6b")
assembly("gml_Object_Shop_Other_5",
    "dfdb7033f2234097ab181e2a4359da5e5ca76af51efc6036bdbc67e1bf68637d")
ordered=["pop.v.i self.opened","call.i gml_Script_soundPlay", "pop.v.i self.image_index",
         "pop.v.i self.mask_index","call.i gml_Script_scrFlashballfx","pop.v.i global.criminal"]
assert [case_step.index(s) for s in ordered] == sorted(case_step.index(s) for s in ordered)
assembly("gml_Object_shopCase_Alarm_0",
    "bdbc9eee0ec4a87d04baadaf3cb277335425411aadf62a0210deccde5d8d637b")
assembly("gml_Object_shopCase_Collision_enemyBulletAni",
    "37dd91e99b0977b57db529c1ba01a3dc0f0baa4292b06f41ffe586223b9b0fa1")
assert not any(e["eventType"] == 8 for e in objects[283]["events"])
assert objects[289]["name"] == "Scattered16" and objects[289]["parentId"] == 90
assert objects[289]["spriteId"] == 550 and objects[289]["textureMaskId"] == -1
assembly("gml_Object_Scattered16_PreCreate_0",
    "4c193a4f963773b2c70b60323cfcdd763b259c208f342702a253d03b3f9e966b")
assembly("gml_Object_parentShootableThing_Create_0",
    "f5c0c5d89f5db5d6ecc01a01c961592fb0a0e413eda14df7c402ed936f487a40")
assembly("gml_Object_Scattered16_Create_0",
    "52b39e49f629e574734e1c9a841513b2194db09abf5e0090d4f582f775d378cd")
assembly("gml_Object_Scattered16_Step_0",
    "28f7516d6d107d7dadbf1ccae5a02792436e188cbb5c97f0295754906c4fec8c")
assembly("gml_Object_Scattered16_Collision_objPlayer_n",
    "70c90ba626fea6e8b178ce91ab34fffa53dd48be9448d0eca6030f6a866d2bd2")
assembly("gml_Object_Scattered16_Alarm_0",
    "b5f534ec4da2c4f3cf6f841a4fbb21b2b3846c8995fa505c362c2154c1f32047")
