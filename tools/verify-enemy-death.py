#!/usr/bin/env python3
"""Pin shared enemy-death bookkeeping and gunshot audio order to the VM export."""
from pathlib import Path

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
