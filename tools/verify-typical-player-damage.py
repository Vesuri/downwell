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
assert manifest["resources"]["objects"][35]["name"] == "objPlayer_n"
assert manifest["resources"]["objects"][82]["name"] == "parentTimeField"
print("Pinned original scrTypicalDamage VM order and player/time-field resource IDs")
