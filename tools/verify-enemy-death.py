#!/usr/bin/env python3
"""Pin shared enemy-death bookkeeping and gunshot audio order to the VM export."""
from pathlib import Path
import json

root = Path(__file__).resolve().parents[1] / "extracted/utmt-0.9.2.0/Assembly"

def body(name, end):
    text = (root / f"gml_GlobalScript_{name}.asm").read_text()
    text = text.split(":[1]", 1)[1].split(f":[{end}]", 1)[0]
    return [line.strip() for line in text.splitlines() if line.strip() and not line.startswith(":")]

assert body("scrEnemyDeath", 2) == """push.v global.comboCount
pushi.e 1
add.i.v
pop.v.v global.comboCount
call.i gml_Script_gunshotStop(argc=0)
popz.v
push.v global.killCount
pushi.e 1
add.i.v
pop.v.v global.killCount
pushi.e 0
pop.v.i global.achNoKill
call.i gml_Script_momentDelay(argc=0)
popz.v
call.i gml_Script_levelBeginCue(argc=0)
popz.v
exit.i""".splitlines()
assert body("gunshotStop", 5) == """pushglb.v global.toggleGuncut
conv.v.b
bf [4]
pushi.e -5
pushi.e 1
push.v [array]self.OLchannel
call.i audio_is_playing(argc=1)
conv.v.b
bf [4]
pushi.e -5
pushi.e 1
push.v [array]self.OLchannel
call.i audio_stop_sound(argc=1)
popz.v
exit.i""".splitlines()
print("Pinned enemy-death global mutation order and gunshot channel reread")
assert body("momentDelay", 6) == """pushbltn.v builtin.room_speed
pushi.e 60
cmp.i.v EQ
bf [5]
pushi.e 37
pushenv [4]
pushi.e 30
pop.v.i builtin.room_speed
pushi.e 30
pop.v.i self.slown
push.i 12323281
setowner.e
pushi.e 1
conv.i.v
pushi.e -1
pushi.e 0
pop.v.v [array]self.alarm
popenv [3]
exit.i""".splitlines()
alarm = (root / "gml_Object_objControlerN_Alarm_0.asm").read_text()
assert [line.strip() for line in alarm.splitlines() if line.strip() and not line.startswith((".", ":"))] == [
    "pushi.e 60", "pop.v.i builtin.room_speed", "pushi.e 0", "pop.v.i self.slown"]
print("Pinned moment-delay outer speed guard, controller writes and alarm restoration")

# A pending restoration belongs to the persistent instance, not to rmMain.
# These guards establish the destination population; they do not execute the
# original runner or prove that either port schedules that population yet.
manifest = json.loads((root.parent / "manifest.json").read_text())["resources"]
controller = manifest["objects"][37]
assert controller["name"] == "objControlerN" and controller["Persistent"]
result_room = manifest["rooms"][16]
assert result_room["name"] == "rmResult" and not result_room["persistent"]
assert [(i["instanceId"], i["objectId"]) for i in result_room["instanceCreationOrder"]] == [(102522, 325)]
assert manifest["objects"][325]["name"] == "Result"
assert not manifest["objects"][325]["Persistent"]
assert not manifest["objects"][68]["Persistent"]

def event_instructions(name):
    return [s.strip() for s in (root / f"gml_{name}.asm").read_text().splitlines()
            if s.strip() and not s.startswith((".", ":"))]

assert event_instructions("Object_objLineToResult_Alarm_1") == [
    "pushi.e 16", "conv.i.v", "call.i room_goto(argc=1)", "popz.v"]
assert event_instructions("Room_rmResult_Create") == [
    "pushglb.v global.g_cameraHeight", "pushi.e 160", "conv.i.v",
    "pushi.e -6", "pushi.e 0", "push.v [array]self.view_camera",
    "call.i camera_set_view_size(argc=3)", "popz.v"]
result_create = event_instructions("Object_Result_Create_0")
assert not any("room_speed" in s for s in result_create)
assert "pop.v.i global.pTimeStop" in result_create
assert event_instructions("Object_Result_Alarm_0")[:14] == [
    "pushi.e 68", "conv.i.v", "pushi.e 0", "conv.i.v", "pushi.e 0", "conv.i.v",
    "call.i gml_Script_instance_create(argc=3)", "popz.v", "push.i 68049", "setowner.e",
    "pushi.e 90", "conv.i.v", "pushi.e -1", "pushi.e 1"]
print("Pinned Result-room handoff, authored birth and persistent-controller survival contract")
