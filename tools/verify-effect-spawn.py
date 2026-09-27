#!/usr/bin/env python3
"""Validate the original resource and VM contract used by EffectSpawnScript."""
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
script = (export / "Assembly/gml_GlobalScript_scrEffectSpawn.asm").read_bytes()
assert sha256(script).hexdigest() == "78323d141dd7d4f1a70d58d95afa88cc18a96cecb1f8379a5f94db06779f6773"
text = script.decode()
writes = [line for line in text.splitlines() if line.startswith("pop.v.v ")]
assert writes[:5] == ["pop.v.v self.myFx"] + [
    "pop.v.v [stacktop]self." + field
    for field in ("sprite_index", "image_speed", "image_angle", "depth")]
parent = manifest["resources"]["objects"][219]
assert (parent["name"], parent["spriteId"], parent["parentId"], parent["Depth"]) == ("parentNoloopFx", -1, -1, 0)
assert {(e["eventType"], e["eventSubtype"]) for e in parent["events"]} == {(7, 7), (14, 0)}
for sprite_id, name in ((602, "sprFxBoingRed"), (603, "sprFxBoingball")):
    sprite = manifest["resources"]["sprites"][sprite_id]
    assert sprite["name"] == name and len(sprite["frames"]) == 1
print("Pinned scrEffectSpawn: synchronous object 219, caller myFx, sprite/speed/angle/depth; usual-gun single-frame resources 602/603")
