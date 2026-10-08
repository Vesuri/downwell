#!/usr/bin/env python3
"""Pin reviewed objCurrency Create; not a claim of complete live gem fidelity."""
from hashlib import sha256
import json
from pathlib import Path
import re
root = Path(__file__).resolve().parents[1]
export = root / "extracted/utmt-0.9.2.0"
manifest = json.loads((export / "manifest.json").read_text())
assert manifest["source"]["sha256"] == "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"
assert manifest["extractor"]["version"] == "0.9.2.0"
assert manifest["extractor"]["commit"] == "03c7048438347799e0717b6e57a9358d000472e0"
code = (export / "Assembly/gml_Object_objCurrency_Create_0.asm").read_bytes()
assert sha256(code).hexdigest() == "306c47960dd5a67345f80fd7cb0b8066ed23c01a13fcd06557016df5c75c73e1"
assert sha256((export / "Assembly/gml_GlobalScript_scrCurrencySpawn.asm").read_bytes()).hexdigest() == \
    "cb0077955a13e851726c15bde3e4724e75b46d64a57c784b168b8ea584229161"
step = (export / "Assembly/gml_Object_objCurrency_Step_0.asm").read_bytes()
assert sha256(step).hexdigest() == "eef98cd1ea85b43303c5a5b58bcb7725655fa3c65d8836176afbea41bbc66f95"
prefix = step.decode().split(":[5]", 1)[0]
assert "call.i choose(argc=3)" in prefix and "call.i choose(argc=2)" in prefix
assert "pop.v.v self.mySound\npushi.e 1\npop.v.i self.allSet" in prefix
for index, digest in enumerate((
    "7610291431a2beba999c2f2423d6afcb2acafd11e5a7c213481c8fe14d3817b6",
    "f8d51bf98f36cb26780d36e66ee884b1dc26a91bb51c02d762a72af0bf8b85ee",
    "c144afec0b630fb6a83d0b6b3cf5091b4b3f6bf2f6aa6667b379f887f37f337b",
    "f8c95365d1fe9b7a31dac0df3494a5b2a6fd4cea65427ed711c782808411052f",
)):
    assert sha256((export / f"Assembly/gml_Object_objCurrency_Alarm_{index}.asm").read_bytes()).hexdigest() == digest
obj = manifest["resources"]["objects"][248]
for name,digest in (
    ("gml_Object_objCurrency_Draw_0","db549eaf1588afac76046e7d4c13374cf40e8975d5e0cca7384e57d7e968f9bd"),
    ("gml_Object_objCurrency_Alarm_4","f8e93656b9ac0f9d600435a800f360afc6c8b2a46ee07cd979beb7eb7b63a771"),
    ("gml_GlobalScript_scrCheckInWater","9eedd20fad0b3732ad066a63ee1af04b3d6407bb5922d20c0eeef67fd17feaf6"),
    ("gml_GlobalScript_scrFxNol","fe76a7898f52d717dc47f864c924facf2cea92a5d87e26b3bfd9ba27ce0c2d6c"),
    ("gml_GlobalScript_scrGainGem","5514c1b786bec32f67950bf7d92a6ca0ed32bb9a8af1f466cd405d21e4739f0f"),
    ("gml_Object_objCurrency_Collision_objPlayer_n","21bd764c6f576791bd20f5a09ab52609b4d232b4fd62a75fc8d470b8193a3af4"),
):
    assert sha256((export / f"Assembly/{name}.asm").read_bytes()).hexdigest() == digest
assert obj["name"] == "objCurrency" and obj["parentId"] == -1
for sprite_id, name in ((133, "sprGemS"), (134, "sprGemM")):
    sprite = manifest["resources"]["sprites"][sprite_id]
    assert sprite["name"] == name and len(sprite["frames"]) == 4
    assert sprite["separateMasks"] == "AxisAlignedRect"
# The standalone animation traversal must not bypass an inherited or local
# Animation End handler when a four-frame currency sequence wraps.
assert not any(e["eventType"] == 7 and e["eventSubtype"] == 7 for e in obj["events"])
event = next(e for e in obj["events"] if e["eventType"] == 0 and e["eventSubtype"] == 0)
assert event["actions"][0]["codeName"] == "gml_Object_objCurrency_Create_0"
# Reviewed literal-index writers, not a general dynamic-call reachability proof.
# Currency Alarm 4 rearms itself, but its Create/ordinary Step do not start it.
assembly = {p.stem: p.read_text() for p in (export / "Assembly").glob("*.asm")}
literal_four = re.compile(r"pushi\.e 4\npop\.[^\n]*\[array\]self\.alarm")
assert {name for name, text in assembly.items() if literal_four.search(text)} == {
    "gml_Object_FINALBOSS_Alarm_4", "gml_Object_FINALBOSS_Alarm_6",
    "gml_Object_FINALBOSS_Step_0", "gml_Object_BatteryCharge_Alarm_4",
    "gml_Object_BatteryCharge_Create_0", "gml_Object_Result_Create_0",
    "gml_Object_Credit_Step_0", "gml_GlobalScript_scrCheckInWater",
    "gml_Object_objCurrency_Alarm_4",
}
water_call = "call.i gml_Script_scrCheckInWater(argc=0)"
assert {name for name, text in assembly.items() if water_call in text} == {"gml_Object_objPlayer_n_Step_1"}
print("Pinned currency Create: area-specific RNG, defaults, alarms, image rate, direction and wall/tile relocation")
print("Currency Alarm 4: reviewed literal writers and player-only water-script call; no inferred dynamic reachability claim")
