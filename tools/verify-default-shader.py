#!/usr/bin/env python3
"""Guard reviewed default palette classification; no GPU execution claim."""
from pathlib import Path
from hashlib import sha256
import json
root = Path(__file__).resolve().parents[1]
metadata = json.loads((root / "generated/default-shader.json").read_text())
assert metadata["sourceSha256"] == "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"
assert metadata["version"] == "0.9.2.0" and metadata["commit"] == "03c7048438347799e0717b6e57a9358d000472e0"
assert metadata["id"] == 0 and metadata["shader"]["Name"]["Content"] == "shaderTemplate"
fragment = metadata["shader"]["GLSL_Fragment"]["Content"]
assert sha256(fragment.encode()).hexdigest() == "bc537b4bf32ff6915c65b894aac3f7b1686186cc143c144619315fa7ace8fb2f"
code = root / "extracted/utmt-0.9.2.0/Assembly"
init = (code / "gml_GlobalScript_scrShaderListInit.asm").read_text()
assert 'pushi.e 0\nconv.i.v\npushi.e -5\npushi.e 0\npush.v [arraypopaf]self.shaderAr\npushi.e 0\npopaf.e\npush.s "DOWNWELL"' in init
print("Pinned embedded default GLSL palette source: flash 254 is light; 7 is dark")
