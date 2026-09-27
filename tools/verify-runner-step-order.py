#!/usr/bin/env python3
"""Pin reviewed Step event-table construction and exact-object list traversal.

This is static binary evidence, not a claim that either live scheduler already
implements these rules. The imported reader verifies the untouched input hash.
"""
from hashlib import sha256
from pathlib import Path
import runpy
import struct

region = runpy.run_path(str(Path(__file__).with_name("verify-runner-choose.py")))["region"]
for start, end, digest in (
    (0x47A330, 0x47A41B, "98d9fca1105ce2c75f7b5728b1c2ee19591bbe72c570d7140e18d331d75872ac"),
    (0x47A585, 0x47A58E, "d44d0d2060e25796189347f540b108dd74d7f79936730f1acb83c83315c2704c"),
    (0x478380, 0x47841D, "ef7694656f4ae8ac1934fb8d1dfac089f085d12408bd86352ef5ad609758e4a9"),
    (0x478830, 0x47893A, "df52e5bc0af37aeaded7f320f819e64c1ec95ed9e68c358a1e51f2d34a2b5104"),
    (0x478AC1, 0x478B5D, "290f385c8a232f7e4b4465fe5a90a55aa904df4b0b15eb6a8bf925e64f407230"),
    (0x478F40, 0x478F94, "29da858b06705fc8b80315c44d4ff37041632c73e0d0b10cb864ad06c84951d9"),
    (0x50B839, 0x50B8E7, "ecbf85fb06ea3b135acdcb2ab3ad997358c342dad1705221fe0c029058245791"),
):
    assert sha256(region(start, end-start)).hexdigest() == digest, hex(start)

# Build event type 3's arrays with subtype upper bound 16 (inclusive).
assert region(0x47A585, 4) == bytes.fromhex("6a 10 6a 03")
call = region(0x47A589, 5)
assert call[0] == 0xE8 and 0x47A58E + struct.unpack("<i", call[1:])[0] == 0x47A330
# Shared table addressing: index = event type * 256 + subtype.
assert 0x941D40 + 3*256*4 == 0x942940
assert 0x93A544 + 3*256*8 == 0x93BD44
# IDs start at zero, append at current count, then advance by one.
assert region(0x47A365, 2) == bytes.fromhex("33 f6")
assert region(0x47A3F2, 3) == bytes.fromhex("89 34 88")
assert region(0x47A401, 1) == bytes.fromhex("46")
# Step selects exact-object head +0x38, not ancestor-family head +0x44.
assert region(0x50B895, 3) == bytes.fromhex("8b 76 38")
assert region(0x4783B0, 3) == bytes.fromhex("89 4e 38")
assert region(0x4783F8, 3) == bytes.fromhex("89 4e 44")
print("Pinned Windows ascending Step object tables and exact-object registration-order traversal")
