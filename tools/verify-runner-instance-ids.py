#!/usr/bin/env python3
"""Pin the selected Windows runner's initial dynamic instance-ID counter."""
from hashlib import sha256
from pathlib import Path
import runpy
import struct

root = Path(__file__).resolve().parents[1]
# Reuse the hash-checked PE reader; this also validates the selected executable.
region = runpy.run_path(str(root / "tools/verify-runner-choose.py"))["region"]
data = (root / "data/data.win").read_bytes()
assert sha256(data).hexdigest() == "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"
assert data[8:12] == b"GEN8"
assert struct.unpack_from("<I", data, 16 + 12)[0] == 105186

# GEN8 loader passes payload+12 to the counter setter, not a derived room max.
assert region(0x497a95, 8) == bytes.fromhex("ff770c e883f5ffff")
assert region(0x497020, 10) == bytes.fromhex("8b442404 a3285b9400 c3")
# Dynamic depth allocation loads, increments, stores and passes the new ID
# before constructing the instance. Counter address is shared with the setter.
assert region(0x487928, 6) == bytes.fromhex("8b0d285b9400")
assert region(0x487933, 13) == bytes.fromhex("41 6a01 ff7514 890d285b9400 51")
assert region(0x487955, 5) == bytes.fromhex("e8969dfdff")
# The constructor writes the ID argument into the instance's numeric-ID field.
assert region(0x4618f8, 3) == bytes.fromhex("8b4510")
assert region(0x46190b, 3) == bytes.fromhex("89467c")
print("Pinned GEN8.LastObj counter and pre-construction increment: first runtime ID 105187")
