#!/usr/bin/env python3
"""Pin the reviewed box Step VM event, not decompiled GML or live integration."""
from hashlib import sha256
import json
from pathlib import Path

root = Path(__file__).resolve().parents[1]
export = root / "extracted/utmt-0.9.2.0"
manifest = json.loads((export / "manifest.json").read_text())
assert manifest["source"]["sha256"] == "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"
assert manifest["extractor"]["version"] == "0.9.2.0"
assert manifest["extractor"]["commit"] == "03c7048438347799e0717b6e57a9358d000472e0"
for name, digest in (
    ("scrSmokefx", "0716085dba28d96bd72012e69c700d6d3b3c789c1895c5a89ad8b9fa1d960bcc"),
    ("scrFlashballfx", "5947c017e6edcbd4f0c32c0a764da4d704e1778692100cb2e39d9764129af9f9"),
):
    script = (export / f"Assembly/gml_GlobalScript_{name}.asm").read_bytes()
    assert sha256(script).hexdigest() == digest
code = (export / "Assembly/gml_Object_objBox_n_Step_0.asm").read_bytes()
assert sha256(code).hexdigest() == "47f9fd6e6f980e368f8f655da488f65b605e53876bb8818f80bd4e9d3626f977"
text = code.decode()
calls = [line for line in text.splitlines() if line.startswith("call.i ")]
assert calls == [
    "call.i gml_Script_soundPlayOL(argc=5)",
    "call.i gml_Script_scrSmokefx(argc=4)",
    "call.i gml_Script_scrFlashballfx(argc=5)",
    "call.i gml_Script_scrEffectSpawn(argc=6)",
    "call.i gml_Script_instance_create(argc=3)",
    "call.i gml_Script_soundPlay(argc=4)",
] + ["call.i instance_place(argc=3)"] * 4 + [
    "call.i instance_destroy(argc=0)",
    "call.i gml_Script_instance_create(argc=3)",
    "call.i gml_Script_instance_create(argc=3)",
    "call.i gml_Script_scrCurrencySpawn(argc=1)",
    "call.i gml_Script_scrCurrencySpawn(argc=1)",
    "call.i gml_Script_alarmStop(argc=1)",
]
print("Pinned box Step: sound/effects, chain reaction, destroy request, debris, gems and alarmStop order")
