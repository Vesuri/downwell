#!/usr/bin/env python3
"""Mask admission payload fidelity and malformed-input rejection."""
from pathlib import Path
import runpy
import struct

root = Path(__file__).resolve().parents[1]
module = runpy.run_path(str(root / "tools/generate-encounter-masks.py"))
select = module["select_pack"]
source = (root / "generated/downwell-sprite-masks.dwsm").read_bytes()
subset = select(source)

def masks(pack):
    result = {}
    for i in range(struct.unpack_from("<I", pack, 16)[0]):
        sprite, frame, width, height, offset = struct.unpack_from("<IIIII", pack, 24+20*i)
        result[sprite, frame] = (width, height, pack[offset:offset+(width+7)//8*height])
    return result

original, chosen = masks(source), masks(subset)
assert tuple(sprite for sprite, frame in chosen) == module["MASKS"]
assert all(value == original[key] for key, value in chosen.items())
assert len(subset) == module["BYTE_LIMIT"]
bad_hash = bytearray(source)
bad_hash[8] ^= 1
bad_offset = bytearray(source)
struct.pack_into("<I", bad_offset, 40, 0)
missing = bytearray(source)
for i in range(struct.unpack_from("<I", source, 16)[0]):
    if struct.unpack_from("<I", source, 24+20*i)[0] == 31:
        struct.pack_into("<I", missing, 24+20*i, 30)
        break
for damaged in (b"", source[:-1], source+b"\0", bad_hash, bad_offset, missing):
    try:
        select(damaged)
    except ValueError:
        pass
    else:
        raise AssertionError("invalid mask admission accepted")
print("Encounter mask subset: exact source bytes, fixed budget, bad input and missing dependency rejection passed")
