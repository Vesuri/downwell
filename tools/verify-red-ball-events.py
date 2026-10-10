#!/usr/bin/env python3
"""Pin reviewed red-ball event bytecode; not a Windows execution test."""
from hashlib import sha256
from pathlib import Path
import runpy

audit = runpy.run_path(str(Path(__file__).with_name("verify-enemy-create.py")))
export, manifest = audit["export"], audit["manifest"]
payload = (export / "Assembly/gml_Object_enmRedBall_Step_0.asm").read_bytes()
assert sha256(payload).hexdigest() == "3029a4e80c7d2afac996c5b15234e64c137bebfbf8a9c1173f760c475f93b939"
event = next(e for e in manifest["resources"]["objects"][118]["events"]
             if e["eventType"] == 3 and e["eventSubtype"] == 0)
assert event["actions"][0]["codeId"] == 1247
print("Pinned red-ball Step: view-gated acceleration, ordered reflection and synchronous death effects")
