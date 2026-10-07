"""Check each prepared row/plane against the original offline DWAP planes."""
from pathlib import Path
import hashlib
import struct

source = Path("generated/amiga-game.dwap").read_bytes()
bank = Path("generated/amiga-effect-stamps.dwef").read_bytes()
magic, version, count, record_size, payload, digest = struct.unpack_from(">4sHHII32s", bank)
assert (magic, version, record_size, payload) == (b"DWEF", 1, 20, 48 + count * 20)
assert digest == hashlib.sha256(source).digest()
assets = {}
for i in range(struct.unpack_from(">I", source, 8)[0]):
    r = struct.unpack_from(">8H2I", source, 20 + i * 24)
    assets[r[:2]] = r
seen = set()
checked = 0
end = payload
for i in range(count):
    sprite, frame, width, height, stride, pixels, mask = struct.unpack_from(">HHHHIII", bank, 48 + i * 20)
    assert (sprite, frame) not in seen
    seen.add((sprite, frame))
    r = assets[sprite, frame]
    assert (width, height, stride * 8, 3) == r[4:8]
    assert pixels == end and mask == pixels + stride * height * 3
    end = mask + stride * height * 3
    assert end <= len(bank)
    for y in range(height):
        for x in range(stride * 8):
            bit = 0x80 >> (x % 8)
            original = [bool(source[r[8] + plane * stride * height + y * stride + x // 8] & bit)
                        for plane in range(3)]
            for plane, expected in enumerate((original[2], original[0], original[1])):
                at = (y * 3 + plane) * stride + x // 8
                assert bool(bank[pixels + at] & bit) == expected
                assert bool(bank[mask + at] & bit) == original[2]
                checked += 1
assert end == len(bank)
assert seen == {(84, f) for f in range(9)} | {(s, f) for s in (133, 134) for f in range(4)}
print(f"Prepared effect bank: {checked} plane pixels and repeated mask pixels exact")
