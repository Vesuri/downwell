#!/usr/bin/env python3
"""Pin reviewed collision_line/point wrappers and selector branches; no execution."""
from hashlib import sha256
from pathlib import Path
import runpy

audit = runpy.run_path(str(Path(__file__).with_name("verify-runner-choose.py")))
region = audit["region"]
for start, end, digest in (
    (0x52cc57, 0x52cc6a, "bb8683649fa4067d38811084c30812e8577d4500ca2e390d2716065c640069d4"),
    (0x52cbb9, 0x52cbcc, "9d22a4b4c3dd0eeac53ec4a6a9768a7e40866740664dd51680801667a286f96f"),
    (0x528ba0, 0x528c7e, "67f4b9e035fb309a8b32eb7e14a3882e6d468c8e7d433e99d007b9e0342ed157"),
    (0x528df0, 0x528e79, "b2721ad716bffee35ef00e11d901967112ec32aab87f2949613b7f5558701e5e"),
    (0x4fd370, 0x4fd6d6, "0f0da904dd3ae825559ee1b3aa85242ae0e85564ca7e2baabd12def101a3226b"),
    (0x4fd6e0, 0x4fd8d3, "169f6db40ad58a26512f10356fa05adee57131dc7c5958282f6edb53056cc222"),
    (0x5014c0, 0x501590, "0ffeae73fb72fb0907a7964b85686b38f8a9e41f8d51a818e7102609e402ffcf"),
    (0x500860, 0x500af7, "294ec6aaac83299027514fb0eefd88bc2f255267f2866ac2be650a2a5c727f3a"),
):
    assert sha256(region(start, end-start)).hexdigest() == digest
assert region(0x80ccd4, 15) == b"collision_line\0"
assert region(0x80cc24, 16) == b"collision_point\0"
assert region(0x795ab8, 4) == bytes.fromhex("0000803f")  # broad-phase far-edge extension +1
# fstp dword stores from argument conversion: all four line coordinates.
for address, data in ((0x528bb3,"d95c2428"), (0x528bbf,"d95c242c"),
                      (0x528bcb,"d95c2430"), (0x528bd8,"d95c2438")):
    assert region(address, 4) == bytes.fromhex(data)
# Object-selector point traversal: ancestor-inclusive list head, then next;
# skip source flags 0x100003. No preparation helper or spatial flush here.
assert region(0x4fd7b5, 3) == bytes.fromhex("8b7044")
assert region(0x4fd7cb, 2) == bytes.fromhex("8b36")
assert region(0x4fd7d6, 7) == bytes.fromhex("f7477803001000")
print("Pinned line/point float wrappers, direct point-list traversal and line selector branches")
