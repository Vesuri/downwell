#!/usr/bin/env python3
"""Pin reviewed Windows bbox/meeting code; not a runner execution test."""
from hashlib import sha256
from pathlib import Path
import runpy

audit = runpy.run_path(str(Path(__file__).with_name("verify-runner-choose.py")))
region = audit["region"]
for start, end, digest in (
    (0x42d500, 0x42d58d, "01d352d29507697e25bcc4c1321372c36791afcc0bb8f99b6276e7caf04f15f7"),
    (0x52b1f0, 0x52b27f, "32dad8432b67077aa41fdcf5a2a9fe7aba74fdeaeb856f435ecbadcacb80755d"),
    (0x466010, 0x4666fe, "44030a7ef3c09e6484c14483fb429217d33f1e087d6b97a8c7327e1b0b91f22f"),
    (0x47bcba, 0x47bce0, "da4711425d941efd63fecf899c4d91e0aa274a2b22620cf9a7ca79b8f86c9e14"),
):
    assert sha256(region(start, end-start)).hexdigest() == digest
assert region(0x7fe38c,10) == b"bbox_left\0"
assert region(0x80c958,14) == b"place_meeting\0"
print("Pinned Windows bbox transforms, compatibility branch and place_meeting wrapper")
