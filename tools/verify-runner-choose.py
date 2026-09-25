#!/usr/bin/env python3
"""Pin the reviewed Windows choose routine; does not execute or modify it."""
from hashlib import sha256
from pathlib import Path
import struct

root = Path(__file__).resolve().parents[1]
binary = (root / "data/Downwell.exe").read_bytes()
assert sha256(binary).hexdigest() == "340b43dae1a18d2861cba9f6a9cdbb92f3754699854e0dc1a1ca367a8cd11085"
pe = struct.unpack_from("<I", binary, 60)[0]
optional = pe + 24
image_base = struct.unpack_from("<I", binary, optional + 28)[0]
section_start = optional + struct.unpack_from("<H", binary, pe + 20)[0]
sections = []
for index in range(struct.unpack_from("<H", binary, pe + 6)[0]):
    _, rva, raw_size, raw = struct.unpack_from("<IIII", binary, section_start + index * 40 + 8)
    sections.append((image_base + rva, raw_size, raw))

def region(address, size):
    for base, length, offset in sections:
        if base <= address and address + size <= base + length:
            start = offset + address - base
            return binary[start:start + size]
    raise AssertionError(f"Unmapped region {address:x}")

# Reviewed registration, choose wrapper and one-word modulo helper. Hashes
# pin the audit, not a portable semantic proof for other runner versions.
for start, end, digest in (
    (0x53FAB5, 0x53FAC4, "c4dd89b74ed8d2747f0f24d410d732ea3370b48073190bf820e210352436ac56"),
    (0x53B860, 0x53B930, "38aa26d37d9a1435257b1f733018919b95a669d2956a918bc2c7ef4bbd6cdd0e"),
    (0x438DA0, 0x438DBD, "78dccd15cf2c9b5b777ce89e15f7cc2ebb52248b26db69e005793e262c0bcfbf"),
):
    assert sha256(region(start, end - start)).hexdigest() == digest
assert region(0x8139D8, 7) == b"choose\0"
assert struct.unpack("<f", region(0x7FCC88, 4))[0] == 0.0
print("Pinned Windows choose registration, wrapper and single-word modulo helper verified")
