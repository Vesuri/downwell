#!/usr/bin/env python3
"""Compare captured Bitmap draws with packed pixels, including word-edge guards."""
import struct
from pathlib import Path

root = Path(__file__).resolve().parent.parent
pack = (root / "generated/amiga-game.dwap").read_bytes()
start, count = struct.unpack_from(">HI", pack, 6)
records = {r[:2]: r for i in range(count)
           for r in [struct.unpack_from(">HHhhHHHHII", pack, start + i * 24)]}
frame = (root / "amiga/.run/style-frame.bin").read_bytes()


def pixel(data, offset, stride, height, x, y):
    index = offset + y * stride + x // 8
    bit = 0x80 >> (x & 7)
    return int(bool(data[index] & bit)) + 2 * int(bool(data[index + stride * height] & bit))


for ident, anchor_x, anchor_y in ((44, 96, 132), (65000, 80, 72), (65001, 80, 152)):
    r = records[ident, 0]
    _, _, ox, oy, width, height, dma_width, _, offset, _ = r
    red = 0
    for y in range(height):
        # Also check the blank word to the right: the old arrow leaked here.
        for x in range(width + (16 if ident == 44 else 0)):
            expected = pixel(pack, offset, dma_width // 8, height, x, y) if x < width else 0
            actual = pixel(frame, 0, 22, 284, anchor_x - ox + x, anchor_y - oy + y)
            assert actual == expected, (ident, x, y, actual, expected)
            red += expected == 2
    assert red > 0, (ident, "missing red pixels")
print("Captured Bitmap arrow (including spill guard) and both shadowed headings match packed pixels")
frame = (root / "amiga/.run/locked-style-frame.bin").read_bytes()
r = records[65003, 0]
_, _, ox, oy, width, height, dma_width, _, offset, _ = r
for y in range(height):
    for x in range(width):
        expected = pixel(pack, offset, dma_width // 8, height, x, y)
        actual = pixel(frame, 0, 22, 284, 80 - ox + x, 152 - oy + y)
        assert actual == expected, ("locked", x, y, actual, expected)
print("Locked-style shadowed heading matches packed pixels")
