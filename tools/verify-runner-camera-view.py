#!/usr/bin/env python3
"""Pin reviewed cached-view reads and conditional follow publication.

Static evidence for this executable, not a dynamic camera-mode parity test.
"""
from hashlib import sha256
from pathlib import Path
import runpy
import struct

region = runpy.run_path(str(Path(__file__).with_name("verify-runner-choose.py")))["region"]
assembly = Path(__file__).resolve().parents[1] / "extracted/utmt-0.9.2.0/Assembly"
for name, digest in (
    ("gml_GlobalScript_scrBuilder.asm", "0c95048af06ff612499acd64a717766441b820d19ffc6017c0ff3a95425581f5"),
    ("gml_GlobalScript___view_get.asm", "8ae49fba7a527e34b099812b7ab948f7266daec64f2182b7e20996a28165a13f"),
    ("gml_Object_camMain_Step_0.asm", "7bbb7ff179d72e3e1ebd3e7695ea8acd7723e4002bb89bc0c476978710368450"),
):
    assert sha256((assembly / name).read_bytes()).hexdigest() == digest, name
for start, end, digest in (
    (0x455E90, 0x455EEC, "d5952ca02d9a2707151f88c13a83bd809ddad321871f2cd35300472b0532eda8"),
    (0x453EC0, 0x454208, "9cd6f92210f8d69f112ea686ee8c3a161d47aa9df7ef7202791d1b7384dcdaa9"),
    (0x453B85, 0x453BA7, "762adafbd542aaec328ece0f17664ad701e1a0973738baabf83707f2b0d2aa7b"),
    (0x456F10, 0x456F17, "00d6f85f2494305daba81e91ba16505dadfdc7e71b7e9e08277a07aa126f3150"),
    (0x50A6BA, 0x50A6E3, "a7db9405e6bda0d96038b3f21d950e3741a852fd4f020919745597023e192d44"),
    (0x50DD70, 0x50DF02, "d998e07deff401ce5d97a5c102ef819bfcf4750ad138cf8568d3433cf4126c41"),
    (0x50DA30, 0x50DA5D, "d4c959115af54acae9c90cbebeca5f0539125aa49c319bc9899e81faaa38cf2e"),
    (0x491C12, 0x491C17, "5efbebdf12ce610c0afa9cd8bf3350e0a4288f2e5bd596c8885fcd3f91b541f6"),
    (0x495FF0, 0x496046, "08996788f553411485ccf047da2f2230beffa188b98afcf3688efe21e3599048"),
):
    assert sha256(region(start, end - start)).hexdigest() == digest, hex(start)

def call(address, target):
    code = region(address, 5)
    assert code[0] == 0xE8
    assert address + 5 + struct.unpack("<i", code[1:])[0] == target

# camera_get_view_y returns camera+0x188, rather than looking up its target.
assert region(0x455EDB, 8) == bytes.fromhex("f3 0f 10 80 88 01 00 00")
assert region(0x454200, 8) == bytes.fromhex("f3 0f 11 96 88 01 00 00")
# Projection publication clears the dirty bit; only dirty cameras are followed
# by the post-Step path. Do NOT mistake this for an unconditional follow pass.
assert region(0x453B96, 7) == bytes.fromhex("c6 86 e1 01 00 00 00")
assert region(0x456F10, 7) == bytes.fromhex("8a 81 e1 01 00 00 c3")
call(0x50A6D3, 0x456F10)
assert region(0x50A6D8, 4) == bytes.fromhex("84 c0 74 07")
call(0x50A6DE, 0x453EC0)
call(0x50DEA2, 0x50A1D0)
call(0x50DECC, 0x5022C0)  # Collision
call(0x50DEF5, 0x50B7E0)  # End Step
# The normal Draw wrapper refreshes the followed camera again, unconditionally
# for an enabled view with a valid camera ID (within the selected camera mode).
call(0x50DA58, 0x491960)
call(0x491C12, 0x495FB0)
call(0x496032, 0x453EC0)
print("Pinned cached camera view, dirty-only post-Step follow and pre-Draw follow")
