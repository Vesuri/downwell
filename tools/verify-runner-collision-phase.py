#!/usr/bin/env python3
"""Pin reviewed collision collection, phase placement and paired dispatch."""
from hashlib import sha256
from pathlib import Path
import runpy

review = runpy.run_path(str(Path(__file__).with_name("verify-runner-choose.py")))
region = review["region"]
for start, size, digest in (
    (0x5030D0, 0x200, "59a68f082497c0c2ed134b237e01ffe0be43e3355d5b12a0a23daa5f73be5015"),
    (0x501EF0, 0x7F, "e1929af99fd7ef2ae676ad677567236da7a1acbf410a363640afb1ffabd545c6"),
    (0x5022F5, 0x2C1, "858ae24f21aa85da46936ec6c95289d1e8374d32e508bf0bc8f97973c85e78d0"),
    (0x50DD76, 15, "ff6e0054f27c2990155a0e75c553c375bb3b2aacd83f6bee67b8c1400a6de0cb"),
    (0x50DEC8, 58, "d75c0f48915733e6dd39e56a7bc490ef55c29f9ba43bb32a3bc779b2745d0d3d"),
    (0x5030D0, 55, "4f1b418205dc219c0e0925395bc3ae13570e5936a81ffd46e5366b5ff170fab0"),
    (0x50319C, 30, "441de3cc63d7054fb528064259f2ebb82a67def23d7bd9f9e7c9e399b04a7be9"),
    (0x503000, 203, "ef27e8f83e2e84f7f5bf0ae1e016d853575cbe0f994137aa919417b083439b1f"),
    (0x502474, 11, "1c84e8e221f19a624e2653871cb83ebaf27ba14d49545b8cc114ef5d9713916f"),
    (0x5025B6, 34, "2b4124b89f8a8d062eafda88599770fb0c026460d4cbb274698f166ce08eac39"),
):
    assert sha256(region(start, size)).hexdigest() == digest

def call_target(address):
    code = region(address, 5)
    assert code[0] == 0xE8
    return address + 5 + int.from_bytes(code[1:], "little", signed=True)

assert call_target(0x50DECC) == 0x5022C0
assert call_target(0x5025CB) == 0x5030D0
assert call_target(0x5031A6) == call_target(0x5031B5) == 0x444910
print("Pinned Windows collision pass before End Step and paired event dispatch verified")
