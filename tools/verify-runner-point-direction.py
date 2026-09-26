#!/usr/bin/env python3
"""Pin point_direction wrapper evidence, not full atan2 equivalence."""
from pathlib import Path
from hashlib import sha256
import runpy
import struct
audit = runpy.run_path(str(Path(__file__).with_name("verify-runner-choose.py")))
region = audit["region"]
for start, end, digest in (
    (0x53fd7c,0x53fd8b,"33960b4cf52e1f92da0908bc1f1bcf541cfe0e5d5edafedb8402d88b06be38be"),
    (0x53d490,0x53d5ae,"9362d69a39db3ad9c4a1f3e371ef8949a76742e03cce80e870eb90aac4726e69"),
    (0x56cec9,0x56cedf,"315ac90c8f00e704776eb0a96953222c23f4efcb9ffcea277e1635a3ef080373"),
    (0x74bec0,0x74beca,"47c357e86274e9c65cbabe40b87f9578f943a55c23840ae92258a57c46f1e647"),
):
    assert sha256(region(start,end-start)).hexdigest() == digest
assert region(0x813bc8,16) == b"point_direction\0"
for address, value in ((0x801d34,270.0),(0x801d30,90.0),(0x7fe7d4,180.0),(0x801234,360.0)):
    assert struct.unpack("<f",region(address,4))[0] == value
assert struct.unpack("<I",region(0x7fe7d0,4))[0] == 0x40490fdb
print("Pinned point_direction float narrowing, axes and degree conversion verified; atan2 remains separate")
