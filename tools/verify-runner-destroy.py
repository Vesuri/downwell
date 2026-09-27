#!/usr/bin/env python3
"""Pin reviewed ordinary instance_destroy and instance_number paths, not execution."""
from hashlib import sha256
from pathlib import Path
import runpy

audit = runpy.run_path(str(Path(__file__).with_name("verify-runner-choose.py")))
region = audit["region"]
for start, end, digest in (
    (0x52cd80, 0x52cd93, "988b282202da578811b0bdcf1c4ae8d3187eb017f46ee5271839bac4523edbfa"),
    (0x529b30, 0x529b81, "bd6623afef5c83108be06a9186fb6eff7eeb2bedfbe8bc4324026c6bc8c77730"),
    (0x528490, 0x5285a6, "467ffd26772e76fb055a178c2162c0b63cc2de980212ba8a07ef331156560b66"),
    (0x4fdf60, 0x4fdfcc, "382908378dc168f5fcf60d7b545982bd8500b9b57586d9a4755016c3d5a172c1"),
    (0x4fe420, 0x4fe555, "fb2d0a3d6b660dc91a5fbf1fc14d05319b370fa5a8a3ccfba6a3df887f6becef"),
    (0x48bbb0, 0x48c111, "ce0f059ca053a6757054d7b4a9eb6e9bcb2cabcb41994c68cc87dba428dc39c6"),
    (0x50da30, 0x50da99, "ff064ca17a97c1644b1964b2d77efceb4c8c19ee33f1c707aa90a5f63a844f3e"),
    (0x5108d0, 0x5109ef, "f63cc7f447bc7b7d7fed387df0b8f91fc23ab5f97ce70047d3f33049bc3e4a08"),
    (0x50db60, 0x50df26, "ef00c233951574519cbf318d6ebe28ce58e05ebb632e9d663ea45d480fc36153"),
):
    assert sha256(region(start, end - start)).hexdigest() == digest
assert region(0x80cdf0, 17) == b"instance_destroy\0"
# No-argument wrapper selects self (-1), enables Destroy, and forwards to
# dispatch. The ordinary path calls Destroy/Clean Up, marks bit 0 and returns;
# it does not call the spatial removal helper (0x501f70) here.
assert region(0x4fdfa4, 4) == bytes.fromhex("83 4e 78 01")
assert region(0x4fe49b, 7) == bytes.fromhex("f7 41 78 03 00 14 00")
assert region(0x800180, 5) == b"Draw\0"
print("Pinned ordinary Destroy exclusion, marked-record sweep and Draw-entry cleanup")
