#!/usr/bin/env python3
"""Pin reviewed object pushenv/popenv capture; not Windows execution evidence."""
from hashlib import sha256
from pathlib import Path
import runpy
import struct

region = runpy.run_path(str(Path(__file__).with_name("verify-runner-choose.py")))["region"]
for start, end, digest in (
    (0x57A6F0, 0x57AD26, "c8c667df5d4229ed19e794bcb207bbd06e1fd111eaed09259fc4b3b3370a60d3"),
    (0x57AD70, 0x57AE30, "11a0963e0b8467382f063ac1a2083888e7b137b01c55d777126824118d0df9d6"),
    (0x57F490, 0x57F503, "c676817351443f2acc549b47c213024b16285a65f5cf02b8513d0286f9bf15e1"),
    (0x57FA60, 0x57FAE5, "16bac4426c59a52a2b02df82311bbad78b862d51d5675db02c72103cc556f387"),
    (0x57BCE0, 0x57BD1F, "d94700c23e06600f67443bd9b1d7d10e56c0abcbf64597402581a49ed277f9a7"),
    # Object-change queue/setter, activation queue producers, deferred rebind.
    (0x464460, 0x464522, "a07c8a5c86486411b9cb3c4b8d660d5fbba4c99ac283ce0113b622482dd77834"),
    (0x467640, 0x467678, "cba757068aff96e0b61205df89cbbb28685f611c14f5b507a3d681e9b888ef72"),
    (0x4627D0, 0x46281C, "0ca6095a8151103bf924542f817fd86226f3edbce56733c6faf45d694bc4975e"),
    (0x46A500, 0x46A526, "6c10e2ded6c6458c51c65781b094afe14f66c3f262f7c25702ddd0f541f030b4"),
):
    assert sha256(region(start, end - start)).hexdigest() == digest, hex(start)

assert struct.unpack("<II", region(0x8A6C08, 8)) == (0x57A6F0, 0x57AD70)
assert struct.unpack("<I", region(0x8A6C50, 4))[0] == 0x16C
for address, encoded in (
    # Ancestor-inclusive list; admission flags checked during capture.
    (0x57A9FD, "8b 71 44"),
    (0x57AA13, "f7 43 78 03 00 10 00"),
    # Counts <=2 branch to inline storage.
    (0x57AB47, "83 f8 02 0f 8e c4 00 00 00"),
    # Larger capture: store previous instance, then publish current self.
    (0x57AB8A, "89 0b 83 c3 04 89 45 14 8b c8"),
    # Two: last instance followed by null; one: last becomes initial self.
    (0x57AC1C, "c7 07 00 00 00 00 83 ef 04 89 17"),
    (0x57AC3B, "c7 07 00 00 00 00 89 55 14"),
    # popenv decrements the larger capture's cursor before loading it.
    (0x57AD8C, "8b 46 04 83 e8 04 8b 38 89 46 04"),
    (0x57ADD5, "89 7b 14"),
    # Pending-array append helper remembers whether the node was present.
    (0x57FA75, "0f 94 c3"),
):
    expected = bytes.fromhex(encoded)
    assert region(address, len(expected)) == expected, hex(address)

print("Pinned Windows object-with capture, inline/snapshot traversal and scratch-list helpers")
