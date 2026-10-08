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
    ("gml_Object_Shop_Step_2.asm", "e124d7e7d595c81e9e222a01e8570eb9f95ad58bdc37b111cc7e5909a861ea6b"),
    ("gml_Object_objTransDither_Step_0.asm", "2a201cbf2233b297030ec00109b90de2e916a53d4ad505de870337226564b9eb"),
    ("gml_GlobalScript_scrBulletOov.asm", "1b393802ddd4cc84545f3571f4a244d0baf692d7ddc1ae49a3b8c9aefeb1f022"),
    ("gml_GlobalScript_scrInView.asm", "cf66675bc8ea809f5bd42270ceee29d3e50eb9c3aa9d916205006db32e0e3432"),
    ("gml_Object_enmFrog_Step_0.asm", "92cfa42170603c8ac63c358fffb17bfa669185edea32cb8d3920370b9e173756"),
    ("gml_Object_enmSnake_Step_0.asm", "690662b8e7584558e39c3f785ae875fb00ed16be334c2f8d1116d8b08e5b37d0"),
    ("gml_Object_enmGroundTurtle_Step_0.asm", "0fd08eea84b434ed58aa0dee3bc2ae4157e95493d41bd27699f0371cdabc5161"),
    ("gml_Object_enmCrawler_Step_0.asm", "0d566dfa4ce1ad04c0fbfe85ffd6afb96b24040406d9746b73a60acbdcf901e0"),
    ("gml_GlobalScript_outOfView.asm", "3e5f05e4af17e1ad76e707bc326648595c927dfdc3fe3f2832cf8da4ad1444fd"),
    ("gml_Object_fxTimeShard_Step_0.asm", "ffc9df3254393892f836845f420fb92b601704d5419987fb2ce4a795fddd6e3b"),
):
    assert sha256((assembly / name).read_bytes()).hexdigest() == digest, name
camera_step = (assembly / "gml_Object_camMain_Step_0.asm").read_text()
assert "push.v self.xxFinal\ncall.i round(argc=1)\npop.v.v self.x" in camera_step
assert "push.v self.yyFinal\ncall.i round(argc=1)\npop.v.v self.y" in camera_step
assert "pushglb.v global.ending\nconv.v.b\nnot.b\nbf [end]" in camera_step
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
    (0x48B6E1, 0x48B7F7, "a35cb39fd52c681c4e11b49ff4314ab4f1c189a583bf8b54ba0b73361843235d"),
    (0x454880, 0x45495E, "2aa742345adfcdf47d1b2f6e00d01cb16245689200818858a2996518c26e01f8"),
    (0x4881C3, 0x488204, "6e2495488078acaffa4fc4b9df358eed3202c368fd13b05831ce27a6b6cd4c7e"),
    (0x4543F0, 0x454450, "4c5e861da32116f24076779eff093616e6093ae3d8a65bb295a7e09e23a20ee1"),
    (0x456CC0, 0x456CC7, "b7f5d0966c39e26e1b0fbd581b9926e2171899f266ea01686257a0a2c989b87f"),
    (0x50FDF9, 0x50FE16, "0ee0b3e1f354ee642cff3fcdd150ffed9cfe09f6d8ab6d3f570ecd1612464f15"),
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
# Resource initialization passes the serialized view to camera construction.
# The constructor converts its integer X/Y to the same float cache read above.
call(0x48B7C3, 0x454880)
assert region(0x454895, 5) == bytes.fromhex("66 0f 6e 67 04")
assert region(0x4548A4, 8) == bytes.fromhex("f3 0f 11 a6 84 01 00 00")
assert region(0x4548AC, 5) == bytes.fromhex("66 0f 6e 57 08")
assert region(0x4548B4, 8) == bytes.fromhex("f3 0f 11 96 88 01 00 00")
# New-room loading copies the resource and clones its camera. The clone copies
# cached coordinates; this is not a follow of the newly created camMain actor.
call(0x50FE11, 0x487FA0)
call(0x4881D7, 0x4543F0)
assert region(0x45441E, 6) == bytes.fromhex("8b 8f 84 01 00 00")
assert region(0x454426, 6) == bytes.fromhex("89 8e 84 01 00 00")
assert region(0x45442C, 6) == bytes.fromhex("8b 8f 88 01 00 00")
assert region(0x454432, 6) == bytes.fromhex("89 8e 88 01 00 00")
print("Pinned camera resource initialization, room cloning and phase-aware view publication")
