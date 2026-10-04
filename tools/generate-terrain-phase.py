#!/usr/bin/env python3
"""Prepare the ordinary six-pixel terrain alignment, never a bank of all phases."""
import hashlib
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
pack = (ROOT / "generated/amiga-game.dwap").read_bytes()
source = json.loads((ROOT / "generated/amiga-game.packing.json").read_text())
digest = lambda data: hashlib.sha256(data).hexdigest()
assert digest(pack) == source["packSha256"], "stale game-pack provenance"
magic, version, directory, count = struct.unpack_from(">4sHHI", pack)
assert magic == b"DWAP" and version == 2
frames = {}
for i in range(count):
    sprite, frame, ox, oy, width, height, stride, flags, offset, size = struct.unpack_from(">HHhhHHHHII", pack, directory + 24*i)
    if sprite != 51:
        continue
    assert (ox, oy, width, height, stride, flags) == (8, 8, 16, 16, 16, 3)
    assert offset + 64 <= len(pack)
    frames[frame] = pack[offset:offset+64]
assert set(frames) == set(range(26))
output = bytearray()
for frame in range(26):
    for row in range(16):
        low = struct.unpack_from(">H", frames[frame], row*2)[0] << 10
        high = struct.unpack_from(">H", frames[frame], 32+row*2)[0] << 10
        output += struct.pack(">III", low | high, low, high)
assert len(output) == 4992
path = ROOT / "generated/amiga-terrain-phase6.bin"
manifest = dict(schemaVersion=1, generator="generate-terrain-phase.py/v1",
    generatorSha256=digest(Path(__file__).read_bytes()), sourceSha256=source["sourceSha256"],
    extractor=source["extractor"], inputPackSha256=digest(pack), outputSha256=digest(output),
    spriteId=51, frames=list(range(26)), phase=6, bytes=len(output),
    layout="frame-major 16 rows of big-endian uint32 coverage, low, high")
if not path.exists() or path.read_bytes() != output:
    path.write_bytes(output)
path.with_suffix(".json").write_text(json.dumps(manifest, indent=2)+"\n")
print(f"Prepared {len(output)} bytes for sprite 51, phase 6")
