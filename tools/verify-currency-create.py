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
obj = manifest["resources"]["objects"][248]
event = next(e for e in obj["events"] if e["eventType"] == 0 and e["eventSubtype"] == 0)
assert event["actions"][0]["codeName"] == "gml_Object_objCurrency_Create_0"
print("Pinned currency Create: area-specific RNG, defaults, alarms, image rate, direction and wall/tile relocation")
