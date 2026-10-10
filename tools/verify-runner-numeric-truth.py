#!/usr/bin/env python3
"""Pin the selected runner's numeric truth helper, not a general VM emulator."""
from hashlib import sha256
from pathlib import Path
import runpy
import struct

runner = runpy.run_path(str(Path(__file__).with_name("verify-runner-choose.py")))
region = runner["region"]  # Validates the original immutable executable hash.
assert sha256(region(0x56a390, 0x138)).hexdigest() == "042f24aa3bd17e5398654817cee77eb871d82b990177012e103ebf461c47a91e"
table = struct.unpack("<14I", region(0x56a490, 56))
assert table[0] == table[13] == 0x56a3b6  # real/bool payload
assert table[7] == 0x56a429              # signed int32
assert table[10] == 0x56a40c             # signed int64
assert struct.unpack("<d", region(0x7f8290, 8))[0] == 0.5
# comisd against 0.5, then SETA; strictly greater, not nonzero.
assert region(0x56a3ba, 8) == bytes.fromhex("66 0f 2f 05 90 82 7f 00")
assert region(0x56a3c3, 3) == bytes.fromhex("0f 97 c3")
# cmp [eax],0; pop esi; SETG: a negative integer is false.
assert region(0x56a429, 7) == bytes.fromhex("83 38 00 5e 0f 9f c3")
# The int64 arm tests the signed high word first, then the low word when
# high==0. Negative and zero are false, positive values true.
assert region(0x56a40c, 15) == bytes.fromhex("83 78 04 00 7c 0f 7f 05 83 38 00 76 08 b3 01")
print("Pinned runner numeric truth: real >0.5, signed integers >0; -1 is false")
