#!/usr/bin/env python3
"""Pin reviewed alarm registration/accessors/dispatch; never modify the runner."""
from hashlib import sha256
from pathlib import Path
import runpy
import struct

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
assert region(0x509DFB, 7) == bytes.fromhex("f7 47 78 03 00 10 00")
# Alarm and Step tables share the reviewed ascending-object builder. Reuse its
# guards for exact-object registration insertion/removal, not just the call.
runpy.run_path(str(Path(__file__).with_name("verify-runner-step-order.py")))
assert region(0x47A57C, 4) == bytes.fromhex("6a 0c 6a 02")
call = region(0x47A580, 5)
assert call[0] == 0xE8 and 0x47A585 + struct.unpack("<i", call[1:])[0] == 0x47A330
assert 0x941D40 + 2*256*4 == 0x942540
assert 0x93A544 + 2*256*8 == 0x93B544
assert region(0x509D97, 10) == bytes.fromhex("b8 40 25 94 00 ba 44 b5 93 00")
assert region(0x509DE6, 3) == bytes.fromhex("8b 76 38")  # exact-object head
assert region(0x509DF4, 3) == bytes.fromhex("8b 7e 08")  # node instance
assert region(0x509E02, 2) == bytes.fromhex("8b 36")     # save next before event
assert region(0x509E51, 5) == bytes.fromhex("8b 70 0c eb 90")
print("Pinned Alarm tables: ascending exact-object IDs, registration-order lists, saved next node")
print("Pinned Windows alarm registration, storage, ordered dispatch and destruction exclusion verified")
