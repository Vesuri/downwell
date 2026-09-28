#!/usr/bin/env python3
"""Pin reviewed objCurrency Create; not a claim of complete live gem fidelity."""
from hashlib import sha256
import json
from pathlib import Path
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
event = next(e for e in obj["events"] if e["eventType"] == 0 and e["eventSubtype"] == 0)
assert event["actions"][0]["codeName"] == "gml_Object_objCurrency_Create_0"
print("Pinned currency Create: area-specific RNG, defaults, alarms, image rate, direction and wall/tile relocation")
