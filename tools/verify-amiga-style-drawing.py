#!/usr/bin/env python3
"""Compare captured Bitmap draws with packed pixels, including word-edge guards."""
import struct
from pathlib import Path

root = Path(__file__).resolve().parent.parent
pack = (root / "generated/amiga-front.dwap").read_bytes()
start, count = struct.unpack_from(">HI", pack, 6)
records = {r[:2]: r for i in range(count)
           for r in [struct.unpack_from(">HHhhHHHHII", pack, start + i * 24)]}
frame = (root / "amiga/.run/style-frame.bin").read_bytes()


def pixel(data, offset, stride, height, x, y):
    index = offset + y * stride + x // 8
    bit = 0x80 >> (x & 7)
    return int(bool(data[index] & bit)) + 2 * int(bool(data[index + stride * height] & bit))


for original, cropped, frames in ((27, 65500, 8), (43, 65501, 1),
                                  (44, 65502, 1), (65401, 65503, 1)):
    for frame_index in range(frames):
        a, b = records[original, frame_index], records[cropped, frame_index]
        for y in range(a[5]):
            for x in range(a[4]):
                cx, cy = x - a[2] + b[2], y - a[3] + b[3]
                expected = pixel(pack, a[8], a[6] // 8, a[5], x, y)
                actual = pixel(pack, b[8], b[6] // 8, b[5], cx, cy) if 0 <= cx < b[4] and 0 <= cy < b[5] else 0
                assert actual == expected, (original, frame_index, x, y)
right, left = records[44, 0], records[65401, 0]
for y in range(right[5]):
    for x in range(right[4]):
        assert pixel(pack, right[8], right[6] // 8, right[5], x, y) == pixel(pack, left[8], left[6] // 8, left[5], right[4] - 1 - x, y)
print("Offline trimmed bobs and mirrored arrow preserve pixels and anchors")


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
