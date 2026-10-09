#!/usr/bin/env python3
"""Pin soundBullet, its source table and admitted terrain materials."""
from hashlib import sha256
from pathlib import Path
import json
root = Path(__file__).resolve().parents[1]
export = root / "extracted/utmt-0.9.2.0"
manifest = json.loads((export / "manifest.json").read_text())
assert manifest["source"]["sha256"] == sha256((root / "data/data.win").read_bytes()).hexdigest() == \
    "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"
assert manifest["extractor"]["version"] == "0.9.2.0"
assert manifest["extractor"]["commit"] == "03c7048438347799e0717b6e57a9358d000472e0"
for name, digest in {
    "gml_GlobalScript_soundBullet": "ad5498adc83580d0a820a799c6b639c87bddcf1b9424cdfb4de80d9dc8151c2c",
    "gml_GlobalScript_initSfxBhit": "14fc6fc64a5a56f8ef54030a9c5cbbaf761bdc5cab0915a7bfc90d938ac7c38d",
    "gml_Object_objWall_n_Create_0": "608d12d79ead23ac5a0692a73161f08a78cc1683d3f37b8ff35628f3ef68fe97",
    "gml_Object_objThinPlatform_Create_0": "8b5168b075caa9a2770bd618fba6940ddf6c5894f23fb5b03ceda423f534eb82",
}.items():
    assert sha256((export / "Assembly" / (name + ".asm")).read_bytes()).hexdigest() == digest
sound = (export / "Assembly/gml_GlobalScript_soundBullet.asm").read_text()
assert sound.index("pop.v.i self.sfxIndex") < sound.index("self.material") < sound.index("irandom_range") < sound.index("soundPlayOL(argc=5)")
assert sound.count("call.i irandom_range(argc=2)") == 1
print("Pinned material-indexed bullet sounds, equal-bound RNG call and gunwall overlap dispatch")
