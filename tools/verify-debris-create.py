#!/usr/bin/env python3
"""Pin the resource identity and complete object-370 Create used by DebrisBirth."""
from hashlib import sha256
from pathlib import Path
import json

root = Path(__file__).resolve().parents[1]
export = root / "extracted/utmt-0.9.2.0"
manifest = json.loads((export / "manifest.json").read_text())
source_hash = "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"
assert sha256((root / "data/data.win").read_bytes()).hexdigest() == source_hash
assert manifest["source"]["sha256"] == source_hash
assert manifest["extractor"]["version"] == "0.9.2.0"
assert manifest["extractor"]["commit"] == "03c7048438347799e0717b6e57a9358d000472e0"
obj = manifest["resources"]["objects"][370]
assert (obj["id"], obj["name"], obj["parentId"], obj["spriteId"]) == (370, "breakablesDebris", -1, 91)
event = next(e for e in obj["events"] if e["eventType"] == 0 and e["eventSubtype"] == 0)
assert event["actions"] == [{"codeId": 2722, "codeName": "gml_Object_breakablesDebris_Create_0"}]
assembly = (export / "Assembly/gml_Object_breakablesDebris_Create_0.asm").read_bytes()
assert sha256(assembly).hexdigest() == "7eeeed80e7b0eeb7c293fd5dbf872ab9644985906dbe9addc7bbefaba1a91417"
print("Pinned object-370 Create: no parent or relocation, four RNG draws and area artwork")
