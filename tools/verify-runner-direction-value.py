#!/usr/bin/env python3
"""Pin direction setter/getter and its mandatory velocity side effects."""
from pathlib import Path
from hashlib import sha256
import runpy
import struct

audit = runpy.run_path(str(Path(__file__).with_name("verify-runner-choose.py")))
region = audit["region"]
for start, end, digest in (
    (0x430b76, 0x430b8c, "fdde38fb1a7e0333169143e134723d78c81798ad261cb8b174f3632ca8a52600"),
    (0x431db0, 0x431df2, "afff95ba28daeed5eeb27cfa28ae1b7aa57dbc81abb3147e130e932008e87d0c"),
    (0x42d850, 0x42d871, "4076814ff204064e940dd8e2c654f1a2cecd5905e0bb9848cbea12516245ce84"),
    (0x46b3d0, 0x46b42c, "a3a11c7c819adc4314e90f2dc8a2e97710944537fe40bd361eb310a6816ce778"),
    (0x466910, 0x466a60, "95c61fdff0241c6f7310620b7462964f2490e3c430ef3796371ec627054c7311"),
    (0x74bea0, 0x74bebe, "fb4a648ae8cd3a7c6c936fba63503de4d0b555f54b1c6a8ea0a237d4a07ed79d"),
):
    assert sha256(region(start, end-start)).hexdigest() == digest
assert region(0x7fe2f4, 10) == b"direction\0"
assert struct.unpack("<d", region(0x801d28, 8))[0] == 360.0
assert struct.unpack("<f", region(0x801234, 4))[0] == 360.0
assert region(0x871741, 5) == b"fmod\0"
assert struct.unpack("<I", region(0x871750, 4))[0] == 0x74beb0
assert region(0x8795ae+8, 1) == region(0x8795ae+12, 1) == b"\0"
print("Pinned direction: binary32 narrowing/storage, fmod(360), negative adjustment and velocity refresh")
