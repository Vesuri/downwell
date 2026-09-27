#!/usr/bin/env python3
"""Pin reviewed point-query code in the untouched Windows runner, not live FP mode."""
from hashlib import sha256
from pathlib import Path
import runpy
import struct

region = runpy.run_path(str(Path(__file__).with_name("verify-runner-choose.py")))["region"]
for start, end, digest in (
    (0x52b780, 0x52b8e8, "adc067209ef0774956f7c7c7d5f045ee6d46711d5faacd6b1ea7c108bbeb3df1"),
    (0x52d890, 0x52d90d, "017fde212f8f6cf5d1130da6f0c4e70bb4b2eecd1945a563787b4a3ed386df2e"),
    (0x465520, 0x465837, "c122727f367baa4003e583062c82e26d933fcaf85ce7089b817238c977352ccb"),
    (0x4ba850, 0x4baa40, "b8392ec39f41e210976736832686d41d77191064e68c4dc3d29f60302dba160d"),
    (0x4bbbb0, 0x4bbd23, "5a2c64a5f93bb3335407bbb57892af85f1ebfb70c502e91dde7194dabfb226eb"),
):
    assert sha256(region(start, end-start)).hexdigest() == digest
for address, bits in ((0x795ab4, 0x3f000000), (0x801618, 0x38d1b717),
                      (0x7fe7d0, 0x40490fdb), (0x7fe7d4, 0x43340000),
                      (0x801d38, 0xb727c5ac), (0x795ab8, 0x3f800000)):
    assert struct.unpack("<I", region(address, 4))[0] == bits
assert region(0x80ce14, 17) == b"position_meeting\0"
print("Pinned Windows point-query wrapper, candidate gate, precise sampler and constants verified")
