#!/usr/bin/env python3
"""Pin reviewed instance_place dispatch; does not execute the Windows runner."""
from hashlib import sha256
from pathlib import Path
import runpy

audit = runpy.run_path(str(Path(__file__).with_name("verify-runner-choose.py")))
region = audit["region"]
for start, end, digest in (
    (0x52cd08, 0x52cd1b, "44e72dafd1f48a74fb4709652b9dcce60d86ed1502e3f142f57ff91afb8a4e08"),
    (0x529e50, 0x529ebb, "0a53a026073fef6cdb42aaac33df23820cc84026c94c69601d9a219afddfa30c"),
    (0x4fe560, 0x4fe8b1, "92aba2c026ca90179868445275b9736946e775ef155ac85e9b3757dc1ff5599a"),
    (0x500d30, 0x5010d5, "8ab166c1fa3996370f74bdf24ac388173e7cd745d5fe59f818ceeb7d84b575ef"),
    (0x5006f0, 0x5007e5, "00e84ad3ec45574f04c6171402e1ee0b690853971f12a44d6e7d57f5d9b5077d"),
    (0x5007f0, 0x500851, "69a31ba1914e1939fd06b7dddcf9b66e545a07ae61b42340793d0441ef970024"),
    (0x5016a0, 0x50175e, "f5a5bb7bac93310410275af37e5b88a7011205e9a21326f037843223cb16b542"),
):
    assert sha256(region(start, end-start)).hexdigest() == digest
assert region(0x80cd7c,15) == b"instance_place\0"
print("Pinned Windows instance_place wrapper, list/spatial dispatch and first-hit traversal")
