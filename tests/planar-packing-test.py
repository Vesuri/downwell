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
# Independent exhaustive oracle for word-aligned overlap, including repeated
# prefixes, no overlap, complete containment and empty streams.
words = [b""] + [bytes(97 + ((bits >> i) & 1) for i in range(length))
                   for length in (2, 4, 6) for bits in range(1 << length)]
for left in words:
    for right in words:
        expected_overlap = max([0] + [n for n in range(2, min(len(left), len(right)) + 1, 2)
                                      if left[-n:] == right[:n]])
        assert pack.planar_overlap(left, right) == expected_overlap
# A later stream may span two original blocks without being contained in
# either one. Only even offsets may be shared.
joined, joined_blocks = bytearray(), {}
assert pack.intern_planar(b"abcd", joined, joined_blocks) == 0
assert pack.intern_planar(b"efgh", joined, joined_blocks) == 4
assert pack.intern_planar(b"cdef", joined, joined_blocks) == 2
assert len(joined) == 8
assert pack.intern_planar(b"bcde", joined, joined_blocks) == 8
ordering_samples = [b"abcdef", b"efgh", b"ghij", b"cd", b"bc", b"abcdef"]
order = pack.planar_payload_order(ordering_samples)
assert order == pack.planar_payload_order(reversed(ordering_samples))
assert set(order) == set(ordering_samples)
ordered_payload, ordered_blocks = bytearray(), {}
for data in sorted(set(ordering_samples), key=order.__getitem__):
    offset = pack.intern_planar(data, ordered_payload, ordered_blocks)
    assert offset % 2 == 0
    assert ordered_payload[offset:offset+len(data)] == data
assert len(ordered_payload) < sum(map(len, set(ordering_samples)))
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
