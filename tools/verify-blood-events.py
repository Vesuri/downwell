#!/usr/bin/env python3
"""Pin the moving-effect VM instructions used by checked blood Step."""
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
payload = (export / "Assembly/gml_Object_parentMovingFx_Step_0.asm").read_bytes()
assert sha256(payload).hexdigest() == "e2d91724e976a16d0025b17b15b7ab34c824fbb022f5e9c13431b1597e2680a5"
step = payload.decode()
assert step.index("pop.v.v self.imgSp") < step.index("call.i gml_Script_TimeStopBound")
assert step.index("pop.v.v self.xx") < step.index("pop.v.v self.yy") < step.index("gml_Script_roundPosition")
payload = (export / "Assembly/gml_GlobalScript_roundPosition.asm").read_bytes()
assert sha256(payload).hexdigest() == "8f2c499d8085446183837ed2101bd9252e46494f3cfca1e090a108a7b79d78e4"
round_position = payload.decode()
assert round_position.index("pop.v.v self.y") < round_position.index("pop.v.v self.x")
obj = manifest["resources"]["objects"][220]
assert obj["name"] == "parentMovingFx" and obj["spriteId"] == -1 and obj["parentId"] == -1
assert len(manifest["resources"]["sprites"][84]["frames"]) == 9
print("Pinned blood Step: capture speed before time-stop, ordered motion, Y then X publication")
