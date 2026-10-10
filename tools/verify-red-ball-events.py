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
for name, digest in (
    ("gml_GlobalScript_scrInView", "cf66675bc8ea809f5bd42270ceee29d3e50eb9c3aa9d916205006db32e0e3432"),
    ("gml_Object_enmRedBall_Draw_0", "b096cd9f4f3c0e147ef554447d561bbe8dd27305558ce7c1651f8f66093db0d7"),
    ("gml_Object_enmRedBall_Alarm_0", "180a4e611d1fcecef7bce7b7a098d02bd852d41c4923c7abe60b3b5a4513b313"),
    ("gml_Object_enmRedBall_Alarm_1", "b5f534ec4da2c4f3cf6f841a4fbb21b2b3846c8995fa505c362c2154c1f32047"),
    ("gml_Object_enmRedBall_Step_2", "a65e0a28be3dadaf81709858f11c61ffa20e15055e19ec1ecc3341ca8c9bf7e0"),
    ("gml_Object_enmRedBall_Collision_enmRedBall", "cc08f648fc3d66e3738134da397a1a29eb5b5285090fe7698050fe1a118e873d"),
    ("gml_Object_enmRedBall_Collision_objPlayer_n", "f22787d495400d04352062395c9bd22f267a044c009cf9404c0559e752789b77"),
    ("gml_GlobalScript_scrOutofview", "e06b55bcc8952fd6eb918461ffce8196a41cff92c7f7dfe97060160468a2fcff"),
):
    assert sha256((export / "Assembly" / (name + ".asm")).read_bytes()).hexdigest() == digest
events = manifest["resources"]["objects"][118]["events"]
for kind, subtype, code in ((2,1,1244),(2,0,1245),(3,2,1246),(4,118,1248),(4,35,1249),(8,0,1250)):
    event = next(e for e in events if e["eventType"] == kind and e["eventSubtype"] == subtype)
    assert event["actions"][0]["codeId"] == code
print("Pinned red-ball contacts, alarm resets and both End Step out-of-view checks")
print("Pinned red-ball Draw: shade/body order, retained directional eye fields and final 359-degree bound")
