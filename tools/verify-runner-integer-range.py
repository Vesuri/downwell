#!/usr/bin/env python3
"""Pin the Windows integer-range path used by fractional shard bounce bounds."""
from hashlib import sha256
from pathlib import Path
import runpy
import struct

runner = runpy.run_path(str(Path(__file__).with_name("verify-runner-choose.py")))
region = runner["region"]  # Includes the immutable executable hash check.
for start, end, digest in (
    (0x53f7b2, 0x53f7c1, "f88a7d5bbc79de1822745fd33efe2b2f9bccaf9a3548d8934e0f646f0ce1d308"),
    (0x53c2e0, 0x53c356, "33e846f1a6d6774cb5ccc5a22ab03dc21abb78b817ddb7ed6014bc8df5bcc88d"),
    (0x56d0c0, 0x56d118, "9f6cc0b0225de51ade79fc64329e5c49b8b7b71825e41ed2d6dc0351534f930c"),
    (0x56d18c, 0x56d1b2, "fadb56bd94aad811592c07384c4919ddb98f12afd48938bd5e0ed8d2fc2fbbc7"),
    (0x715a80, 0x715ac8, "d41f85f5bb3d4c8b0d16665c4a68d352f885664def4217df512045d038f2d3ec"),
    (0x5410c0, 0x54111c, "61e9e9345d82e366f174195b4fae7f4966d1985c77820fa565c6e76a7a8fa36b"),
):
    assert sha256(region(start, end-start)).hexdigest() == digest
assert region(0x813898, 14) == b"irandom_range\0"
assert region(0x56d1a4, 1) == b"\0"  # Real-valued argument dispatch.
assert struct.unpack("<I", region(0x56d18c, 4))[0] == 0x56d0f6
print("Pinned fractional irandom_range conversion and unconditional two-word draw verified")
