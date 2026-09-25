#!/usr/bin/env python3
"""Pin reviewed smoke geometry wrappers; not a Windows execution test."""
from pathlib import Path
from hashlib import sha256
import runpy

audit = runpy.run_path(str(Path(__file__).with_name("verify-runner-choose.py")))
region = audit["region"]
for start, end, digest in (
    (0x53c8e0, 0x53c9a5, "6ecd39c59858e5dd28bc307aed2edece383a346dbe4ae22b23e36cfba0def40d"),
    (0x53c9b0, 0x53ca7c, "1c925004c35c96013007f5918d3fbd998e225d86d030703557663317ba4f40ab"),
    (0x53d5b0, 0x53d654, "9e3425ff681846dd2fadc78949e12524fce9a79af1373c237c40ea976188b55d"),
    (0x759d50, 0x759de9, "02796544151465375223364088a3b98c3457b37f0ab36422ba11f62f28e62f00"),
):
    assert sha256(region(start, end-start)).hexdigest() == digest
assert region(0x7fe7d0, 4).hex() == "db0f4940"  # float pi, not double pi
assert region(0x7fe7d4, 4).hex() == "00003443"  # float 180
assert region(0x801618, 4).hex() == "17b7d138"  # float 1e-4, direction snap
print("Pinned float-rounded smoke direction/distance wrappers and constants")
