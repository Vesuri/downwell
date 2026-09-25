#!/usr/bin/env python3
"""Lossless packing proof, including writable DMA isolation."""
import importlib.util
import json
import random
from pathlib import Path

root = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("pack", root / "tools/generate-amiga-hud.py")
pack = importlib.util.module_from_spec(spec)
spec.loader.exec_module(pack)
rng = random.Random(84)
payload, blocks, records = bytearray(), {}, []
samples = [b"\x00\x01\x02\x03\x04\x05", b"\x02\x03", b"\x04\x05\x06\x07",
           b"\x01\x02", b"\x06\x07", bytes(64)]
samples += [bytes(rng.randrange(4) for _ in range(2*rng.randrange(1, 64))) for _ in range(1000)]
for data in samples:
    offset = pack.intern_planar(data, payload, blocks)
    assert offset % 2 == 0
    records.append((offset, data))
for offset, data in records:
    assert payload[offset:offset+len(data)] == data

extracted = root / "extracted/utmt-0.9.2.0"
manifest = json.loads((extracted / "manifest.json").read_text())
optimized = (root / "generated/amiga-game.dwap").read_bytes()

def unshared(data, payload, blocks):
    offset = len(payload)
    payload.extend(data)
    return offset

# Same asset derivation, deliberately no payload sharing in the oracle.
pack.intern_planar = unshared
reference = pack.build_pack(manifest, extracted, "game", root / "data/language.ini")

def directory(data):
    _, _, size, count, _ = pack.HEADER.unpack_from(data)
    return [pack.RECORD.unpack_from(data, size+i*pack.RECORD.size) for i in range(count)]

actual, expected = directory(optimized), directory(reference)
assert len(actual) == len(expected)
for a, b in zip(actual, expected):
    assert a[:8] == b[:8] and a[9] == b[9]
    assert a[8] % 2 == 0
    assert optimized[a[8]:a[8]+a[9]] == reference[b[8]:b[8]+b[9]], a[:2]
    if a[7] == 4:  # Garden DMA headers are writable; no aliasing at all.
        for other in actual:
            if other is a:
                continue
            assert a[8]+a[9] <= other[8] or other[8]+other[9] <= a[8], (a[:2], other[:2])
print(f"Lossless packing: {len(records)} synthetic blocks, {len(actual)} source records, writable DMA isolation passed")
