#!/usr/bin/env python3
"""Pin reviewed alarm registration/accessors/dispatch; never modify the runner."""
from hashlib import sha256
from pathlib import Path
import runpy

# Reuse the pinned executable's PE section reader and whole-file hash guard.
review = runpy.run_path(str(Path(__file__).with_name("verify-runner-choose.py")))
region = review["region"]
assert region(0x7FE358, 6) == b"alarm\0"
for start, end, digest in (
    (0x431260, 0x4312D4, "8cf4a80076db498a1404651a662c655ed46ecdbaec746a1abffb6e81337c1d2a"),
    (0x430C23, 0x430C39, "e15a251dc60fe69a29a261501a96be03957e8fb689e90d0e8ecb22d78cfe3ee4"),
    (0x469060, 0x469079, "daf18e15f98ef2353fc253076011ad3dd0207bd2d52065db971014a4148bb17a"),
    (0x46BF80, 0x46BF97, "1b5f87022151e5f2c1823ec64107e30555c1a27c89d96af3906cf2b95275d1c2"),
    (0x509D97, 0x509E84, "62c667a3ee7dc779d0b6220cef0d47ec7d084a81389bd62be19df20f7a29732e"),
):
    assert sha256(region(start, end - start)).hexdigest() == digest
print("Pinned Windows alarm registration, storage and ordered dispatch verified")
