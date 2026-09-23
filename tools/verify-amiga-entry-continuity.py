#!/usr/bin/env python3
"""Check both pre-rendered foreground views agree at the garden/entry handoff."""
import struct
from pathlib import Path
from PIL import Image

root = Path(__file__).resolve().parent.parent
data = (root / "amiga/.run/menu-entry-frame.bin").read_bytes()
assert len(data) == 12 + 27648 + 24576 * 2
fg, bg, scroll, _ = struct.unpack_from(">IIHH", data)
entry = data[12:12 + 27648]
background = data[12 + 27648:12 + 27648 + 24576]
garden = data[12 + 27648 + 24576:]
y, byte = divmod(fg, 24)
x = byte * 8 + 15 - (scroll & 15)
by, bx = divmod(bg, 64)
bx = bx * 8 + 15 - (scroll >> 4)
def pixel(plane, stride, x, y):
    return (plane[y * stride + x // 8] >> (7 - (x & 7))) & 1
assert 284 <= y <= 384
pixels = []
background_lit = 0
for row in range(284):
    for col in range(160):
        foreground = pixel(entry, 24, x + col, y + row)
        assert foreground == pixel(garden, 64, 240 + x + col, y - 284 + row), (col, row)
        sky = pixel(background, 64, bx + col, by + row) if row < max(0, 576 - y) else 0
        background_lit += sky
        pixels.append((255, 255, 255) if foreground or sky else (0, 0, 0))
assert background_lit > 0, "handoff lost the sky"
image = Image.new("RGB", (160, 284))
image.putdata(pixels)
image.save(root / "build/amiga-menu-entry-frame.png")
print("Entry continuity verified: identical foreground world pixels; parallax background present")
