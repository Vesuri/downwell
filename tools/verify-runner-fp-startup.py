#!/usr/bin/env python3
"""Pin CRT precision initialization; deliberately not a live FP-mode proof."""
from hashlib import sha256
from pathlib import Path
import runpy
import struct

root = Path(__file__).resolve().parents[1]
# Reuse the hash-checked PE mapping, without loading or executing the game.
pe = runpy.run_path(str(root / "tools/verify-runner-choose.py"))
region = pe["region"]
for start, end, digest in (
    (0x716D7E, 0x716D93, "8b6ecfd4425d6e80b52e1b41de36c20248c88d3c1f1bc3e9b0e4631b44d936d9"),
    (0x7683A7, 0x7683EA, "50516ca988d81c9dc206b027246d0ad0e92d38f83d2a8dbf1a35d12af782120a"),
    (0x79572C, 0x795754, "b14703cb8f46249d59af47ac3be6da95096aa088b997337e406deca1b5cd2e6a"),
    (0x716C68, 0x716D0B, "48fe6191451c85fb2b3cb21136eaa0150a7086322b6216e59d0783441c8b214a"),
    (0x717E8A, 0x717EAB, "b632f4a7dafb5cc1acada88a578f78ca214e77f60e73d6aeef7351bfcf5e572d"),
    (0x76A6CF, 0x76A72E, "b4717205710ec5d0adde12c41f79299c16caef1c1ab15501c65012a5cf394056"),
    (0x77EE78, 0x77EF16, "6626f7a07b62f8bf3bd95559e52bed2acfe64c8869e0837c88f94cc8b1e44c5b"),
    (0x77EB25, 0x77EE26, "daa9ae22ccf783dcfe5f2e1de769632d2e0c9d25bfd7f441a61bbc0e1be561f5"),
    (0x77EE40, 0x77EE78, "6ef751e046511848d4f873412a3b60df57a298b755142bc8b8f07e06af0f2aae"),
    (0x76B96A, 0x76B9E2, "0e2ac6ee2a19d7c3ed197e0d8d366e376ef4e9be4d64a5e830d212531269c780"),
    (0x757002, 0x757076, "660288b3bc6df83c0a7dc3ae8683371cbbfbb96e8c0853596811c0409e94143a"),
):
    assert sha256(region(start, end-start)).hexdigest() == digest

def push32(address):
    assert region(address, 1) == b"\x68"
    return struct.unpack("<I", region(address+1, 4))[0]

def call(address):
    assert region(address, 1) == b"\xe8"
    return address+5+struct.unpack("<i", region(address+1, 4))[0]

assert (push32(0x716D7E),push32(0x716D83),call(0x716D88)) == (0x795754,0x79572C,0x7683A7)
initializers = struct.unpack("<10I", region(0x79572C,40))
assert initializers[:2] == (0,0x716C68)
assert call(0x716CDD) == 0x717E8A
mask, value = push32(0x717E8A), push32(0x717E8F)
assert region(0x717E94,2) == b"\x6a\x00"  # null current-control output
assert call(0x717E96) == 0x76A6CF
assert (mask,value) == (0x30000,0x10000)  # _MCW_PC, _PC_53
assert mask & 0x300 == 0  # _MCW_RC is NOT selected by this initialization.
for rounding in (0,0x100,0x200,0x300):
    old = 0x9001f | rounding
    updated = (old & ~mask) | (value & mask)
    assert updated & 0x300 == rounding
assert region(0x77EE4A,2) == b"\xdb\xe3"  # separate FNINIT reset helper
assert call(0x77EE4C) == 0x717E8A
assert push32(0x77EE51) == 0x1f80 and call(0x77EE56) == 0x76B96A
print("Pinned CRT startup selects 53-bit precision but preserves rounding control; live x87/MXCSR remain unverified")
