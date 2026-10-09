#!/usr/bin/env python3
"""Check the shared selected profiles against the pinned VM table writes."""
import json
import hashlib
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
export = root / "extracted/utmt-0.9.2.0"
manifest = json.loads((export / "manifest.json").read_text())
assert manifest["source"]["sha256"] == "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"
assert manifest["extractor"]["version"] == "0.9.2.0"
asm = (export / "Assembly/gml_GlobalScript_bStatInitLevel1.asm").read_text()
assert "pushi.e 0\npop.v.i self.gunnum" in asm
first = asm.split("push.v self.gunnum\npushi.e 1\nadd.i.v\npop.v.v self.gunnum", 1)[0]
assert 'push.s "MACHINEGUN"' in first
writes = re.findall(
    r"pushi.e (-?\d+)\nconv.i.v\npushi.e -5\npush.v self.gunnum\nconv.v.i\n"
    r"push.v \[arraypopaf\]self.(bStat\w+)\npushi.e ([01])\npopaf.e", first)
table = {}
for value, field, level in writes:
    key = (field, int(level))
    assert key not in table, ("duplicate write", key)
    table[key] = int(value)

fields = ["Object", "BulletSprite", "Sound", "Muzzle", "Damage", "Speed",
          "Accuracy", "ConRate", "Rof", "Recoil", "ScreenShake",
          "ScreenShakeDur", "RangeRandom", "RangeTimer", "Pierce", "Wave", "Explode"]
header = (root / "src/core/MachineGunProfile.h").read_text()
rows = re.findall(r"MachineGunProfile\{([^{}]+)\}", header)
assert len(rows) == 2, "expected exactly two literal source profiles"
levels = set()
for row in rows:
    tokens = [v.strip() for v in row.split(",")]
    values = [int(v.replace("false", "0").replace("true", "1")) for v in tokens]
    level, actual = values[0], values[1:]
    assert level in (0, 1) and level not in levels
    levels.add(level)
    expected = [table[("bStat" + field, level)] for field in fields]
    assert actual == expected, (level, dict(zip(fields, actual)), dict(zip(fields, expected)))
print("Both machine-gun profiles match 34 selected VM table assignments")
for level in (0, 1):
    assert table[("bStatAimAngleAccl", level)] == 5
    assert table[("bStatAimAngleDccl", level)] == 2
    assert table[("bStatAimAngleLimit", level)] == 0
aim = (export / "Assembly/gml_GlobalScript_scrAimControl.asm").read_text()
assert "push.v self.aimAngle\ncall.i abs(argc=1)\npushglb.v global.aimAngleLimit\ncmp.v.v GT" in aim
assert "pushglb.v global.aimAngleLimit\npush.v self.aimAngle\ncall.i sign(argc=1)\nmul.v.v\npop.v.v self.aimAngle" in aim
update = (export / "Assembly/gml_GlobalScript_bStatUpdate.asm").read_text()
assert ("pushi.e -5\npush.v self.bnum\nconv.v.i\n"
        "push.v [arraypushaf]self.bStatAimAngleLimit\npush.v self.blvl\n"
        "conv.v.i\npushaf.e\npop.v.v global.aimAngleLimit") in update
player_step = (export / "Assembly/gml_Object_objPlayer_n_Step_0.asm").read_text()
assert "call.i gml_Script_scrAimControl(argc=0)\npopz.v" in player_step
print("Both machine-gun levels clamp player aim to zero; other weapons need their own aim rules")

# Rendering must follow the firing direction even before the remaining spread
# and fractional-motion work is connected. Do not mistake frame-zero artwork
# (authored pointing right) for an already downward-facing projectile.
shoot = (export / "Assembly/gml_GlobalScript_scrPlayerShootN.asm").read_text()
step = (export / "Assembly/gml_Object_bulletStandard_Step_0.asm").read_text()
assert "pushi.e 270\npush.v self.aimAngle\nadd.v.i\npop.v.v self.shotAngle" in shoot
assert "push.v self.bDir\npop.v.v self.image_angle" in step
assert step.index("call.i random_range(argc=2)") < step.index("pop.v.v self.image_angle")
print("VM projectile image angle follows firing direction plus first-Step spread")
end_step = (export / "Assembly/gml_Object_bulletStandard_Step_2.asm").read_bytes()
assert hashlib.sha256(end_step).hexdigest() == "5daab67fc60337936392d83c43dc43d2068d3b73e937650cc1c5e8fc1264913f"
assert re.findall(r"call.i ([^\n]+)", end_step.decode()) == [
    "gml_Script_scrOutofview(argc=0)", "gml_Script_scrBulletOov(argc=0)"]
for name, digest in {
    "scrOutofview": "e06b55bcc8952fd6eb918461ffce8196a41cff92c7f7dfe97060160468a2fcff",
    "scrBulletOov": "1b393802ddd4cc84545f3571f4a244d0baf692d7ddc1ae49a3b8c9aefeb1f022",
}.items():
    assert hashlib.sha256((export / "Assembly" / ("gml_GlobalScript_" + name + ".asm")).read_bytes()).hexdigest() == digest
print("Usual-gun view retirement is source-ordered End Step, not movement-time removal")
create = (export / "Assembly/gml_Object_bulletStandard_Create_0.asm").read_text()
parent_create = (export / "Assembly/gml_Object_parentPBullet_Create_0.asm").read_text()
assert "pushi.e 101\npop.v.i self.hitWallFx" in parent_create
assert "push.d 0.3\npop.v.d self.image_speed" in create
assert "pushi.e 0\npop.v.i self.image_index" in create
assert all(len(manifest["resources"]["sprites"][i]["frames"]) == 6 for i in (458, 459))
print("VM projectile animation starts at zero, advances by 0.3, and uses six-frame sprites")
assert "push.v self.allSet\nconv.v.b\nnot.b\nbf [2]" in step
assert "push.v self.bDir\npush.v self.bSpeed\ncall.i lengthdir_x(argc=2)\npop.v.v self.xsp" in step
assert "push.v self.bDir\npush.v self.bSpeed\ncall.i lengthdir_y(argc=2)\npop.v.v self.ysp" in step
assert step.index("pop.v.i self.allSet") < step.index("call.i gml_Script_scrBulCheckSolid(argc=0)")
assert step.index("call.i gml_Script_scrBulCheckSolid(argc=0)") < step.index("pop.v.v self.x\n")
assert "push.v self.x\npush.v self.xsp\nadd.v.v\npop.v.v self.x" in step
assert "push.v self.y\npush.v self.ysp\nadd.v.v\npop.v.v self.y" in step
print("VM spread/velocity initialization precedes solid checks and retained vector movement")
solid = (export / "Assembly/gml_GlobalScript_scrBulCheckSolid.asm").read_text()
assert hashlib.sha256((export / "Assembly/gml_GlobalScript_scrBulCheckSolid.asm").read_bytes()).hexdigest() == \
    "7e2e5107502d9671858a0e7c4cdf23501f9e21c4e7c810a3a1fdabe3e2053e4c"
assert solid.count("call.i collision_line(argc=7)") == 3
assert "call.i object_get_parent(argc=1)\npushi.e 84\ncmp.i.v EQ" in solid
assert "pushi.e 90\nconv.i.v\npush.v self.y\npush.v self.x\ncall.i place_meeting(argc=3)" in solid
assert "pushi.e 90\nconv.i.v\npush.v self.y\npush.v self.x\ncall.i instance_place(argc=3)" in solid
assert solid.index("call.i instance_destroy(argc=0)", solid.index(":[16]")) < solid.index("pushi.e 272")
assert "push.v self.bDir\npush.d 0.4\nconv.d.v\npushi.e 605" in solid
print("Solid script branches pinned: repeated wall selection, direct parent 84, shootable 90 and post-destroy explosion")
box_create = (export / "Assembly/gml_Object_objBox_n_Create_0.asm").read_text()
assert "pushi.e 1\npop.v.i self.wallHp\npushi.e 0\npop.v.i self.drilled" in box_create
assert "pushi.e 0\npop.v.i self.autoTile\npushi.e 0\npop.v.i self.checked" in box_create
assert ("pushi.e 174\nconv.i.v\npushi.e 173\nconv.i.v\npushi.e 172\nconv.i.v\n"
        "pushi.e 171\nconv.i.v\ncall.i choose(argc=4)\npop.v.v self.breakSound") in box_create
assert "pushi.e 50\nconv.i.v\ncall.i irandom(argc=1)\npushi.e 1\ncmp.i.v LT\npop.v.b self.haveGem" in box_create
assert "pushi.e 2\npop.v.i self.haveGem" in box_create
assert "pushi.e 22\npop.v.i self.image_index" in box_create
print("Box Create health, flags and cavern sound/gem choices are VM-pinned")
assert ("push.v self.y\npush.v self.ysp\nadd.v.v\n"
        "push.v self.x\npush.v self.xsp\nadd.v.v\n"
        "push.v self.y\npush.v self.x\ncall.i collision_line(argc=7)") in solid
assert step.index("call.i gml_Script_scrEffectSpawn(argc=6)") < step.index("pop.v.i self.allSet")
print("VM computes upcoming endpoint arguments after synchronous muzzle Create; runner wrapper owns narrowing")
approach = solid.split(":[7]", 1)[1].split(":[10]", 1)[0]
assert ("push.v self.y\npush.v self.ysp\npushi.e 10\nconv.i.d\ndiv.d.v\nadd.v.v\npop.v.v self.y\n"
        "push.v self.x\npush.v self.xsp\npushi.e 10\nconv.i.d\ndiv.d.v\nadd.v.v\npop.v.v self.x") in approach
assert "push.v self.whileStop\npushi.e 1\nadd.i.v\npop.v.v self.whileStop" in approach
assert "push.v self.whileStop\npushi.e 100\ncmp.i.v GT\nbf [9]" in approach
assert "pushi.e 0\npop.v.i self.whileStop" in solid
assert step.index("pop.v.v self.x\n") < step.index("pop.v.v self.y\n")
print("VM approach writes Y then X with division by ten and permits 101 iterations; final Step writes X then Y")
impact = (export / "Assembly/gml_GlobalScript_scrTypicalBulletDamage.asm").read_text()
bat = (export / "Assembly/gml_Object_enmBat_Step_0.asm").read_text()
assert "push.v self.bDir\npop.v.v other.hitDir" in impact
assert "push.v self.hitDir\npop.v.v self.direction\npushi.e 3\npop.v.i self.movesp" in bat
print("VM bat hit heading receives the projectile's spread-adjusted bDir")
init = (export / "Assembly/gml_GlobalScript_scrPlayerInit.asm").read_text()
assert "pushglb.v global.pugLasersight\nconv.v.b\nbf [7]" in init
assert "pushi.e 0\npop.v.i self.shootLeg" in init
assert "pushglb.v global.pugLasersight\nconv.v.b\nbf [8]" in shoot
assert "pushi.e 0\npush.v self.myBullet\npushi.e -9\npop.v.i [stacktop]self.bdirRand" in shoot
# Reviewed runner random_range at VA 0x53d840 unconditionally calls 0x438e60
# at 0x53d8b4, then multiplies by range width, including zero. That helper
# calls WELL at 0x438dc0 once. Pin the executable underlying that evidence.
assert hashlib.sha256((root / "data/Downwell.exe").read_bytes()).hexdigest() == \
    "340b43dae1a18d2861cba9f6a9cdbb92f3754699854e0dc1a1ca367a8cd11085"
print("Laser Sight VM birth rules and reviewed zero-range RNG runner are pinned")

# Reviewed playerDraw blocks 23..34: zero-length origin query, cumulative
# parentWall(57) then subparentEnemy(83) queries (precise/notme both false),
# Y += lengthdir_y(8,270+aimAngle), X += lengthdir_x(8,270+aimAngle-xsp),
# strict view-bottom exit after advancement, then red line from (x,y+8).
draw = (export / "Assembly/gml_GlobalScript_playerDraw.asm").read_text()
ray = draw.split(":[23]\n", 1)[1].split(":[35]\n", 1)[0]
assert hashlib.sha256(ray.encode()).hexdigest() == \
    "7f15feddae0ae5adcf00d0fdd3e6a9d3e89681322c520020eeee29d797163c9f"
assert ray.count("call.i collision_line(argc=7)") == 2
assert "random" not in ray
print("Laser Sight stepped ray order, collision parents and draw anchors are VM-pinned")

# collision_line selects object ancestry, NOT the Solid flag or the player's
# landing surfaces. Keep the full wall family explicit for the ray adapters.
objects = manifest["resources"]["objects"]
def descendants(parent):
    result = set()
    for obj in objects:
        current, visited = obj["id"], set()
        while current >= 0:
            assert current not in visited, ("inheritance cycle", obj["id"])
            visited.add(current)
            if current == parent:
                result.add(obj["id"])
                break
            current = objects[current]["parentId"]
    return result

walls = descendants(57)
assert walls == {57, 59, 60, 61, 84, 85, 86, 92, 98, 99, 101, 102,
                 254, 278, 279, 292, 293, 319, 354, 355, 356, 360, 368}
enemies = descendants(83)
assert {111, 112, 113, 116, 117, 119, 284} <= enemies
assert not walls & enemies
assert 93 not in walls | enemies  # objThinPlatform is a landing-only family.
assert all(not objects[i]["Solid"] for i in (60, 92, 254, 278, 279))

# Resource defaults only: instance mask/sprite overrides and transforms must
# still be respected. These offsets are inclusive GameMaker bounding margins,
# not a claim about collision_line's floating-point edge rounding.
sprites = manifest["resources"]["sprites"]
expected_bounds = {
    60: (-8, 7, -8, 7), 92: (-8, 7, -8, 7),
    254: (-8, 7, -8, 7), 278: (-30, 29, 0, 7),
    279: (-9, 8, -9, 8),
    111: (-6, 6, -4, 4), 112: (-8, 7, -4, 7),
    113: (-6, 5, -5, 7), 116: (-6, 6, -4, 3),
    117: (-7, 6, -6, 6), 119: (-5, 4, -5, 4),
}
for object_id, expected in expected_bounds.items():
    obj = objects[object_id]
    sprite_id = obj["textureMaskId"]
    if sprite_id < 0:
        sprite_id = obj["spriteId"]
    sprite = sprites[sprite_id]
    assert sprite["separateMasks"] == "AxisAlignedRect"
    actual = (sprite["marginLeft"] - sprite["originX"],
              sprite["marginRight"] - sprite["originX"],
              sprite["marginTop"] - sprite["originY"],
              sprite["marginBottom"] - sprite["originY"])
    assert actual == expected, (object_id, actual, expected)
print("Laser Sight wall ancestry, thin-platform exclusion and selected mask bounds verified")
case_step = (export / "Assembly/gml_Object_shopCase_Step_0.asm").read_text()
assert "push.v self.destroyed\nconv.v.b\nbf [5]" in case_step
assert "push.v self.opened\nconv.v.b\nnot.b\nbf [4]" in case_step
assert "pushi.e 582\npop.v.i self.mask_index" in case_step
assert sprites[582]["name"] == "noMask"
print("Destroyed shop-case first-Step mask override is VM-pinned")
