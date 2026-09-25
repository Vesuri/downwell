#!/usr/bin/env python3
"""Pin the reviewed range wrappers used by skeleton effect bursts."""
from pathlib import Path
from hashlib import sha256
import runpy

audit = runpy.run_path(str(Path(__file__).with_name("verify-runner-choose.py")))
region = audit["region"]
# irandom_range orders its integer arguments, passes inclusive width to the
# two-word 63-bit modulo helper, then adds the lower bound.
# random_range orders doubles, computes width, draws random(1), multiplies by
# width and adds lower. Integer bounds +/-10 keep the range arithmetic exact
# before the script adds its VM-real position; no general x87 claim is made.
for start, end, digest in (
    (0x53c2e0, 0x53c356, "33e846f1a6d6774cb5ccc5a22ab03dc21abb78b817ddb7ed6014bc8df5bcc88d"),
    (0x5410c0, 0x54111c, "61e9e9345d82e366f174195b4fae7f4966d1985c77820fa565c6e76a7a8fa36b"),
    (0x53d840, 0x53d8cb, "e26551ca1e035cdd75a2167d66540e04e0f38b90a211a37ef93bcca0ed181ad2"),
):
    assert sha256(region(start, end-start)).hexdigest() == digest
print("Pinned effect integer/real range wrappers and two-word modulo helper verified")
