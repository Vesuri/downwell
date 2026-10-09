#!/usr/bin/env python3
"""Pin shared enemy-death bookkeeping and gunshot audio order to the VM export."""
from pathlib import Path
import json
import runpy

root = Path(__file__).resolve().parents[1] / "extracted/utmt-0.9.2.0/Assembly"
player_init = (root / "gml_GlobalScript_scrPlayerInit.asm").read_text()
assert "pushi.e 1\npop.v.i global.achNoKill" in player_init
assert player_init.index("pop.v.i global.achNoKill") < player_init.index("pushglb.v global.pugDecoy")
assert "pushi.e 0\npop.v.i global.firstLand" in player_init
initialize = (root / "gml_GlobalScript_scrInitialize.asm").read_text()
assert "pushi.e 193\npop.v.i global.bgm" in initialize
for area in range(1,6):
    assert f"pushi.e {193+area}\nconv.i.v\npushi.e -5\npushi.e {area}\npop.v.v [array]self.areaBgm" in initialize
global_stuff = (root / "gml_GlobalScript_scrGlobalStuff.asm").read_text()
assert "global.bgm" not in global_stuff and "global.firstLand" not in global_stuff

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
cue = body("levelBeginCue",20)
assert cue[:19] == [
    "pushglb.v global.firstLand", "conv.v.b", "not.b", "bf [3]",
    "pushglb.v global.pTimeStop", "conv.v.b", "not.b", "b [4]", "push.e 0", "bf [19]",
    "pushglb.v global.area", "pushi.e 5", "cmp.i.v NEQ", "bf [13]",
    "pushi.e 372", "conv.i.v", "push.v self.y", "push.v self.x", "call.i gml_Script_instance_create(argc=3)"]
assert cue[19:32] == ["popz.v", "pushi.e 1", "pop.v.i global.firstLand", "pushi.e 186", "conv.i.v",
    "call.i audio_is_playing(argc=1)", "conv.v.b", "bf [8]", "pushi.e 186", "conv.i.v",
    "call.i audio_stop_sound(argc=1)", "popz.v", "pushglb.v global.bgm"]
gain = cue.index("call.i audio_sound_gain(argc=3)")
assert cue[gain-5:gain+14] == ["pushi.e 0", "conv.i.v", "pushi.e 1", "conv.i.v", "pushglb.v global.bgm",
    "call.i audio_sound_gain(argc=3)", "popz.v", 'push.s "music"@192', "conv.s.v", "pushi.e 1", "conv.i.v",
    "pushi.e 1", "conv.i.v", "pushi.e 100", "conv.i.v", "pushglb.v global.bgm",
    "call.i gml_Script_soundPlayOL(argc=5)", "popz.v", "b [19]"]
ending = cue[gain+14:]
assert ending[:19] == ["pushi.e 1", "pop.v.i global.firstLand", "pushi.e 186", "conv.i.v",
    "call.i audio_is_playing(argc=1)", "conv.v.b", "bf [15]", "pushi.e 186", "conv.i.v",
    "call.i audio_stop_sound(argc=1)", "popz.v", "pushglb.v global.bgm", "call.i audio_is_playing(argc=1)",
    "conv.v.b", "bf [17]", "pushglb.v global.bgm", "call.i audio_stop_sound(argc=1)", "popz.v", "pushglb.v global.bgm"]
assert "call.i gml_Script_instance_create(argc=3)" not in ending
assert "call.i gml_Script_soundPlayOL(argc=5)" not in ending
print("Pinned levelBeginCue guards, synchronous child, firstLand and normal/ending audio order")
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
assert manifest["objects"][325]["parentId"] == -1
result_events = {(e["eventType"], e["eventSubtype"]): e["actions"][0]["codeId"]
                 for e in manifest["objects"][325]["events"]}
assert result_events[(0, 0)] == 2513 and result_events[(14, 0)] == 2521
assert result_room["creationCodeId"] == 695
assert not manifest["objects"][68]["Persistent"]

def event_instructions(name):
    return [s.strip() for s in (root / f"gml_{name}.asm").read_text().splitlines()
            if s.strip() and not s.startswith((".", ":"))]

assert event_instructions("Object_objLineToResult_Alarm_1") == [
    "pushi.e 16", "conv.i.v", "call.i room_goto(argc=1)", "popz.v"]
assert event_instructions("Object_objLineToResult_Alarm_0") == [
    "pushi.e 68", "conv.i.v", "pushi.e 0", "conv.i.v", "pushi.e 0", "conv.i.v",
    "call.i gml_Script_instance_create(argc=3)", "popz.v", "push.i 68049", "setowner.e",
    "pushi.e 90", "conv.i.v", "pushi.e -1", "pushi.e 1", "pop.v.v [array]self.alarm"]
assert event_instructions("Room_rmResult_Create") == [
    "pushglb.v global.g_cameraHeight", "pushi.e 160", "conv.i.v",
    "pushi.e -6", "pushi.e 0", "push.v [array]self.view_camera",
    "call.i camera_set_view_size(argc=3)", "popz.v"]
result_create = event_instructions("Object_Result_Create_0")
assert event_instructions("Object_Result_PreCreate_0") == ["call.i event_inherited(argc=0)", "popz.v"]
position = result_create.index("push.v self.yPlace")
assert result_create[position:position+5] == ["push.v self.yPlace", "pop.v.v self.yy",
    "push.v self.yy", "call.i round(argc=1)", "pop.v.v self.y"]
assert position < result_create.index("call.i random_range(argc=2)")
assert not any("room_speed" in s for s in result_create)
assert "pop.v.i global.pTimeStop" in result_create
assert event_instructions("Object_Result_Alarm_0")[:14] == [
    "pushi.e 68", "conv.i.v", "pushi.e 0", "conv.i.v", "pushi.e 0", "conv.i.v",
    "call.i gml_Script_instance_create(argc=3)", "popz.v", "push.i 68049", "setowner.e",
    "pushi.e 90", "conv.i.v", "pushi.e -1", "pushi.e 1"]
print("Pinned Result-room handoff, authored birth and persistent-controller survival contract")
result_step = event_instructions("Object_Result_Step_0")
assert result_step[:2] == ["call.i gml_Script_scrResult(argc=0)", "popz.v"]
assert body("scrResult", 56)[-13:] == [
    "pop.v.v global.ply", "push.v self.yy", "push.v self.yPlace", "push.v self.yy",
    "sub.v.v", "push.v self.yyScrollSp", "div.v.v", "add.v.v", "pop.v.v self.yy",
    "push.v self.yy", "call.i round(argc=1)", "pop.v.v self.y", "exit.i"]
print("Pinned Result Step's yy rounding and instance-y publication before wind audio")
assert manifest["objects"][68]["name"] == "objTransDither" and manifest["objects"][68]["parentId"] == -1
assert event_instructions("Object_objTransDither_PreCreate_0") == ["call.i event_inherited(argc=0)", "popz.v"]
assert event_instructions("Object_objTransDither_Create_0") == [
    "pushi.e 0", "conv.i.v", "push.l 0", "conv.l.v", "call.i gml_Script___view_get(argc=2)",
    "pushi.e 80", "add.i.v", "pop.v.v self.x",
    "pushi.e 0", "conv.i.v", "push.l 1", "conv.l.v", "call.i gml_Script___view_get(argc=2)",
    "pushi.e 100", "sub.i.v", "pop.v.v self.y",
    "push.i 68049", "setowner.e", "pushi.e 15", "conv.i.v", "pushi.e -1", "pushi.e 0", "pop.v.v [array]self.alarm"]
print("Pinned exit-wipe parentless PreCreate, ordered view-relative setters and alarm initialization")
assert event_instructions("Object_objTransDither_Step_0") == [
    "push.v self.y", "pushi.e 0", "conv.i.v", "push.l 1", "conv.l.v",
    "call.i gml_Script___view_get(argc=2)", "pushi.e 0", "conv.i.v", "push.l 3", "conv.l.v",
    "call.i gml_Script___view_get(argc=2)", "add.v.v", "cmp.v.v LT", "bf [end]",
    "push.v self.y", "pushi.e 8", "add.i.v", "pop.v.v self.y"]
number=manifest["objects"][372]
assert number["name"]=="levelNumber" and number["parentId"]==-1 and not number["Persistent"]
assert number["Depth"]==0 and number["Visible"] and number["spriteId"]==-1
# Dynamic instance_create uses the compatibility table, not OBJT's depth.
depth_tools=runpy.run_path(str(Path(__file__).with_name("generate-object-pack.py")))
assert depth_tools["dynamic_depths"](root,manifest["objects"])[372]==-100000
assert {(e["eventType"],e["eventSubtype"]) for e in number["events"]}=={(0,0),(2,0),(2,1),(2,2),(8,0),(14,0)}
assert event_instructions("Object_levelNumber_PreCreate_0")==["call.i event_inherited(argc=0)","popz.v"]
number_create=event_instructions("Object_levelNumber_Create_0")
assert number_create[:4]==["pushglb.v global.hardMode","conv.v.b","not.b","bf [2]"]
for suffix in ['push.s "-"@913', 'push.s "-H"@3215']:
    i=number_create.index(suffix)
    assert number_create[i-5:i]==['push.s "area"@269',"pushglb.v global.area","call.i string(argc=1)",
        "add.v.s","call.i gml_Script_langString(argc=1)"]
    assert number_create[i+1:i+6]==["add.s.v","pushglb.v global.level","call.i string(argc=1)","add.v.v","pop.v.v self.levelTxt"]
assert body("langString",3)==[
    "push.v arg.argument0","pop.v.v self.getString",'push.s "language.ini"@2354',"conv.s.v",
    "call.i ini_open(argc=1)","popz.v",'push.s " "@304',"conv.s.v","push.v self.getString",
    "pushglb.v global.globalLanguage","call.i ini_read_string(argc=3)","pop.v.v self.returnString",
    "call.i ini_close(argc=0)","popz.v",'push.s "#"@2356',"conv.s.v",'push.s "*"@2357',"conv.s.v",
    "push.v self.returnString","call.i string_replace_all(argc=3)","pop.v.v self.returnString",
    "push.v self.returnString","ret.v","exit.i"]
assert number_create.count("call.i string_char_at(argc=2)")==1
assert number_create.index("call.i string_hash_to_newline(argc=1)")<number_create.index("call.i string_width(argc=1)")
assert number_create[-14:]==["pushi.e 4","pop.v.i self.txtApInterval","push.i 68049","setowner.e",
    "push.v self.txtApInterval","pushi.e -1","pushi.e 0","pop.v.v [array]self.alarm",
    "pushi.e 4","pop.v.i self.txtExInterval","pushi.e 120","pop.v.i self.txtRemainTime",
    "pushi.e 0","pop.v.i self.txtExit"]
assert event_instructions("Object_levelNumber_Alarm_1")==[
    "pushi.e 1","pop.v.i self.txtExit","push.i 68049","setowner.e","push.v self.txtExInterval",
    "pushi.e -1","pushi.e 2","pop.v.v [array]self.alarm"]
for index,counter,interval,next_alarm in [(0,"txtShown","txtApInterval",0),(2,"txtExit","txtExInterval",2)]:
    instructions=event_instructions(f"Object_levelNumber_Alarm_{index}")
    assert instructions[:4]==[f"push.v self.{counter}","push.v self.txtLength","cmp.v.v LT","bf [2]"]
    assert instructions[4:15]==["push.i 167295","setowner.e","pushi.e 1","conv.i.v","pushi.e -1",
        "push.v self.txtShown","conv.v.i","push.v [arraypopaf]self.txtAt","pushi.e 1","popaf.e",f"push.v self.{counter}"]
    assert instructions[15:24]==["pushi.e 1","add.i.v",f"pop.v.v self.{counter}","push.i 68049","setowner.e",
        f"push.v self.{interval}","pushi.e -1",f"pushi.e {next_alarm}","pop.v.v [array]self.alarm"]
assert event_instructions("Object_levelNumber_Alarm_2")[-2:]==["call.i instance_destroy(argc=0)","popz.v"]
number_draw=event_instructions("Object_levelNumber_Draw_0")
assert number_draw.count("call.i gml_Script___view_get(argc=2)")==4
assert number_draw.count("call.i gml_Script_scrDrawBorderTextBlack(argc=3)")==2
assert number_draw[:4]==["pushglb.v global.surfaceFx","call.i surface_exists(argc=1)","conv.v.b","bf [end]"]
assert number_draw[-2:]==["call.i surface_reset_target(argc=0)","popz.v"]
print("Pinned level-number parentless lifetime, alarms and Draw-owned text animation")
border = ["push.v arg.argument0", "pop.v.v self.sx", "push.v arg.argument1", "pop.v.v self.sy",
          "push.v arg.argument2", "call.i string(argc=1)", "pop.v.v self.txt",
          "pushi.e 0", "conv.i.v", "call.i draw_set_color(argc=1)", "popz.v"]
for axis, operation in [("x","add"),("x","sub"),("y","add"),("y","sub")]:
    border += ["push.v self.txt", "call.i string_hash_to_newline(argc=1)", "push.v self.sy"]
    if axis == "x":
        border += ["push.v self.sx", "pushi.e 1", f"{operation}.i.v"]
    else:
        border += ["pushi.e 1", f"{operation}.i.v", "push.v self.sx"]
    border += ["call.i draw_text(argc=3)", "popz.v"]
border += ["push.i 16777215", "conv.i.v", "call.i draw_set_color(argc=1)", "popz.v",
           "push.v self.txt", "call.i string_hash_to_newline(argc=1)", "push.v self.sy", "push.v self.sx",
           "call.i draw_text(argc=3)", "popz.v", "exit.i"]
assert body("scrDrawBorderTextBlack",2) == border
print("Pinned black-border text caller fields, cardinal draw order and final white color")
