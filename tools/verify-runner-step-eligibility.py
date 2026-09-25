#!/usr/bin/env python3
"""Pin reviewed instance creation timestamps and Step-phase exclusion."""
from hashlib import sha256
from pathlib import Path
import runpy

review = runpy.run_path(str(Path(__file__).with_name("verify-runner-choose.py")))
region = review["region"]
for start, end, digest in (
    (0x461778, 0x46178B, "ac5ab164ca5315e780e0f74ab0f04bf07ce6212abcc7fa7faab188bfe6ef5791"),
    (0x50B813, 0x50B839, "b98da9dd76e4ef3220ead3bc4f1f8b839fedea5a4a7fcb1000f5542f37d0752f"),
    (0x50B8A0, 0x50B8D1, "e9ce3ce3b053ecc1cccaa1accb666b873834b36bd1ed509dfd58fd4892b2d9d0"),
    (0x50DD62, 0x50DD80, "564aa955e34cf0fbc0fbdcd03339c05f88d47197449722abe80f62720dc9fb90"),
    (0x50DEDF, 0x50DEFD, "552a5a845d998ad3bd0172210c823227848186efd9d8591adb2e5aa8172d3aa9"),
):
    assert sha256(region(start, end-start)).hexdigest() == digest
print("Pinned Windows instance timestamp and Step-phase eligibility regions verified")
