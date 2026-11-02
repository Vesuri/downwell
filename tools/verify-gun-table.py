#!/usr/bin/env python3
"""Check src/core/GunTable.h against bStatInitLevel1 in the pinned UTMT export.

The decompiled view is compared after pinning the assembly it was produced
from, so a different export cannot silently pass.
"""
import hashlib
import re
import sys
from fractions import Fraction
from pathlib import Path

root = Path(__file__).resolve().parent.parent
export = root / "extracted/utmt-0.9.2.0"
ASM_SHA256 = "8c6a8a39c4a1c7c6fedcf7e12144f938e6d1ad7572f8402bb14fd0d31406f757"

asm = (export / "Assembly/gml_GlobalScript_bStatInitLevel1.asm").read_bytes()
digest = hashlib.sha256(asm).hexdigest()
if digest != ASM_SHA256:
    sys.exit(f"unreviewed bStatInitLevel1 assembly {digest}")
gml = (export / "CodeEntries/gml_GlobalScript_bStatInitLevel1.gml").read_text()

guns = []
for line in gml.splitlines():
    line = line.strip()
    if line in ("gunnum = 0;", "gunnum += 1;"):
        guns.append({})
        continue
    m = re.match(r"global\.(\w+)\[gunnum\](?:\[(\d)\])? = (.+);$", line)
    if m:
        key, level, value = m.groups()
        guns[-1].setdefault(key, {})[None if level is None else int(level)] = value
assert "global.bulletMaxNum = gunnum;" in gml and len(guns) == 7, "seven guns"

header = (root / "src/core/GunTable.h").read_text()
rows = re.findall(r'\{"([A-Z]+)",([^}]*)\}', header)
assert len(rows) == 14, "fourteen table rows"

FIELDS = ["letter", "object", "sprite", "sound", "muzzle", "consumption", "rof",
          "recoilFifths", "shake", "shakeDuration", "delayKill", "burst", "burstRate",
          "burstAmount", "damage", "speed", "accuracyHalves", "pierce", "rangeRandom",
          "rangeTimer", "spType", "sp1", "sp2", "aimAcceleration", "aimDeceleration", "aimLimit"]
SOURCE = {"object": "bStatObject", "sprite": "bStatBulletSprite", "sound": "bStatSound",
          "muzzle": "bStatMuzzle", "consumption": "bStatConRate", "rof": "bStatRof",
          "recoilFifths": "bStatRecoil", "shake": "bStatScreenShake",
          "shakeDuration": "bStatScreenShakeDur", "delayKill": "bStatDelayKill",
          "burst": "bStatBurst", "burstRate": "bStatBurstRate", "burstAmount": "bStatBurstAmount",
          "damage": "bStatDamage", "speed": "bStatSpeed", "accuracyHalves": "bStatAccuracy",
          "pierce": "bStatPierce", "rangeRandom": "bStatRangeRandom", "rangeTimer": "bStatRangeTimer",
          "spType": "bStatSpType", "sp1": "bStatSp1", "sp2": "bStatSp2",
          "aimAcceleration": "bStatAimAngleAccl", "aimDeceleration": "bStatAimAngleDccl",
          "aimLimit": "bStatAimAngleLimit"}
SCALE = {"recoilFifths": 1280, "accuracyHalves": 2}

for index, (name, body) in enumerate(rows):
    gun, level = guns[index // 2], index % 2
    values = [v.strip() for v in body.split(",")]
    assert len(values) == len(FIELDS), f"row {index} field count"
    assert gun["bStatName"][None] == f'"{name}"', f"row {index} name"
    for field, text in zip(FIELDS, values):
        actual = {"true": 1, "false": 0}.get(text)
        actual = Fraction(int(text)) if actual is None else Fraction(actual)
        if field == "letter":
            expected = Fraction(gun["bStatGunSprite"][None])
        else:
            expected = Fraction(gun[SOURCE[field]][level]) * SCALE.get(field, 1)
        assert expected.denominator == 1, f"{name} {field} is not exact in its unit"
        assert actual == expected, f"{name}[{level}].{field}: header {actual}, source {expected}"
    for unused in ("bStatWave", "bStatExplode"):
        assert gun[unused][level] == "0", f"{name} {unused} needs explicit support"
print(f"Gun table matches bStatInitLevel1 (assembly {digest})")
