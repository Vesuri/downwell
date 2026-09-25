#!/usr/bin/env python3
"""Recover the precise exit-field mask; keep all derived payloads ignored."""
import base64
import hashlib
import json
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
SHA = "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"
source = ROOT / "data/data.win"
assert hashlib.sha256(source.read_bytes()).hexdigest() == SHA
output = ROOT / "generated/downwell-goal-mask.h"
export = ROOT / "extracted/utmt-0.9.2.0/goal-mask.json"
output.parent.mkdir(exist_ok=True)
if not export.exists():
    tool = subprocess.check_output([str(ROOT / "tools/bootstrap-utmt.sh")], text=True).strip()
    export.parent.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, DOWNWELL_GOAL_MASK_OUTPUT=str(export), DOWNWELL_DATA_WIN_SHA256=SHA)
    subprocess.run([tool, "load", str(source), "-s", str(ROOT / "tools/export-goal-mask.csx"),
                    "-o", str(ROOT / "generated/goal-mask-roundtrip.win"), "-f"], env=env, check=True)
record = json.loads(export.read_text())
assert record["sourceSha256"] == SHA and record["version"] == "0.9.2.0"
assert record["extractor"] == "UndertaleModTool"
assert record["spriteId"] == 616 and record["width"] == record["height"] == 128
raw = base64.b64decode(record["data"], validate=True)
assert len(raw) == 2048
assert hashlib.sha256(raw).hexdigest() == "4ac0480f82bfa47416ca4fd72e653819c9f27ce2ddc28028b4ddd43f0afef58b"
metadata = json.loads((export.parent / "manifest.json").read_text())
assert metadata["source"]["sha256"] == SHA
field = metadata["resources"]["objects"][255]
sprite = metadata["resources"]["sprites"][616]
assert field["name"] == "goalTimeField" and field["spriteId"] == 616 and field["textureMaskId"] == -1
assert sprite["separateMasks"] == "Precise" and sprite["originX"] == sprite["originY"] == 64
assembly = (export.parent / "Assembly/gml_Object_goalTimeField_Create_0.asm").read_text()
for axis in ("x", "y"):
    assert f"pushi.e 2\npop.v.i self.image_{axis}scale" in assembly
rows = []
for y in range(128):
    pixels = [x for x in range(128) if raw[y * 16 + x // 8] & (128 >> (x % 8))]
    left, right = (pixels[0], pixels[-1] + 1) if pixels else (0, 0)
    # Exact row intervals, not an ellipse fit: reject any holes in the source.
    assert pixels == list(range(left, right))
    rows.append(f"    {{{left}, {right}}},")
text = ("// Generated precise mask, sprite 616. Do not commit recovered payloads.\n"
        f"// data.win SHA256: {SHA}; UndertaleModTool 0.9.2.0\n"
        "#ifndef DOWNWELL_GENERATED_GOAL_MASK_H\n#define DOWNWELL_GENERATED_GOAL_MASK_H\n"
        "namespace downwell {\nstatic constexpr unsigned char goalMaskRows[128][2] = {\n" +
        "\n".join(rows) + "\n};\n}\n#endif\n")
output.write_text(text)
output.with_suffix(".mask").write_bytes(raw)
manifest = {"sourceSha256": SHA, "extractor": "UndertaleModTool", "version": "0.9.2.0",
            "spriteId": 616, "rawMaskSha256": hashlib.sha256(raw).hexdigest(),
            "outputSha256": hashlib.sha256(text.encode()).hexdigest(), "rows": 128}
output.with_suffix(".json").write_text(json.dumps(manifest, indent=2) + "\n")
print("128 exact exit-field mask rows recovered; every source bit verified")
