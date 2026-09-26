#!/usr/bin/env python3
"""Pin the corpse-expiration child's resource and inherited event contracts."""
from hashlib import sha256
from pathlib import Path
import json

root = Path(__file__).resolve().parents[1]
export = root / "extracted/utmt-0.9.2.0"
manifest = json.loads((export / "manifest.json").read_text())
source = "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"
assert sha256((root / "data/data.win").read_bytes()).hexdigest() == source
assert manifest["source"]["sha256"] == source
assert manifest["extractor"]["version"] == "0.9.2.0"
assert manifest["extractor"]["commit"] == "03c7048438347799e0717b6e57a9358d000472e0"
child, parent = (manifest["resources"]["objects"][i] for i in (242, 219))
assert (child["name"], child["spriteId"], child["parentId"]) == ("fxJobutsu", 102, 219)
assert (parent["name"], parent["parentId"]) == ("parentNoloopFx", -1)
assert {(e["eventType"], e["eventSubtype"]) for e in child["events"]} == {(0, 0), (14, 0)}
assert {(e["eventType"], e["eventSubtype"]) for e in parent["events"]} == {(7, 7), (14, 0)}
for event, digest in {
    "fxJobutsu_Create_0": "5ed65d76c1ba17fc0c1cef26989ae1f62457d42e82fc6e77eb9055df31062239",
    "fxJobutsu_PreCreate_0": "4c193a4f963773b2c70b60323cfcdd763b259c208f342702a253d03b3f9e966b",
    "parentNoloopFx_Other_7": "7fa420002321759f60d9103f49806b50a0673341e139e47b46bb18672ccf6d78",
    "parentNoloopFx_PreCreate_0": "4c193a4f963773b2c70b60323cfcdd763b259c208f342702a253d03b3f9e966b",
}.items():
    assert sha256((export / f"Assembly/gml_Object_{event}.asm").read_bytes()).hexdigest() == digest
print("Pinned corpse expiry: object 242, sprite 102, speed 0.3, inherited Animation End retirement; no Step/Draw overrides")
