#!/usr/bin/env python3
"""Pin the reviewed VM script; not proof of live collision dispatch or math."""
from hashlib import sha256
from pathlib import Path
import json

root = Path(__file__).resolve().parents[1]
export = root / "extracted/utmt-0.9.2.0"
source = "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"
assert sha256((root / "data/data.win").read_bytes()).hexdigest() == source
manifest = json.loads((export / "manifest.json").read_text())
assert manifest["source"]["sha256"] == source
assembly = export / "Assembly/gml_GlobalScript_scrTypicalDamage.asm"
assert sha256(assembly.read_bytes()).hexdigest() == "539bd30ec98b6dc05f1bd4dd02c63ebaf852ecc31d90d8293b00f64712ff6d48"
assembly = export / "Assembly/gml_GlobalScript_scrPDamage.asm"
assert sha256(assembly.read_bytes()).hexdigest() == "3955a5529ea0456965cf1aea9280eed407ce66adc9a3130111c637a549f83a32"
assert manifest["resources"]["objects"][35]["name"] == "objPlayer_n"
assert manifest["resources"]["objects"][37]["name"] == "objControlerN"
assert manifest["resources"]["objects"][82]["name"] == "parentTimeField"
number = manifest["resources"]["objects"][221]
assert (number["name"],number["parentId"],number["spriteId"],number["textureMaskId"],number["Visible"],number["Persistent"]) == (
    "objDmgCalcFxPlayer",-1,-1,-1,True,False)
assert {(event["eventType"],event["eventSubtype"]) for event in number["events"]} == {(0,0),(2,0),(3,0),(8,0),(14,0)}
for name,digest in (
    ("gml_Object_objDmgCalcFxPlayer_Create_0","27def18e2dba947dd29714073b1eb23996cd1ff0177a3e2a65fe84bdeb58a7b7"),
    ("gml_Object_objDmgCalcFxPlayer_PreCreate_0","4c193a4f963773b2c70b60323cfcdd763b259c208f342702a253d03b3f9e966b"),
    ("gml_Object_objDmgCalcFxPlayer_Step_0","5dd4c95cc74689d2332837738db301e024ead338056ce892a2ea7468b661ce51"),
    ("gml_Object_objDmgCalcFxPlayer_Alarm_0","d7288b2999ecc62b3f8db25a0313f8144cf903161c68062d4cb8181c8fbcd7dd"),
    ("gml_Object_objDmgCalcFxPlayer_Draw_0","8537d6a71e52404a7e2b69580d62008b66805b0fc4c62a00de798c568ffedf93"),
    ("gml_GlobalScript_scrDmgCalcFxPlayer","bebc7fa225fa39c157d27ed3c115e5633c24ead0d7e6b2e435a5fc14935e508b"),
    ("gml_GlobalScript_scrDrawBorderTextRed","151bc7511159687326d5be1a23220790d9bfe200f338cefbc5c1d712f1f81910"),
):
    assert sha256((export/"Assembly"/(name+".asm")).read_bytes()).hexdigest()==digest,name
print("Pinned original scrTypicalDamage/scrPDamage VM order and player/controller/time-field resource IDs")
