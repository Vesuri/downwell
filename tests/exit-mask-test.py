#!/usr/bin/env python3
"""Verify every offline cover bit against the recovered planar edge."""
from pathlib import Path
import struct

root = Path(__file__).resolve().parent.parent
pack = (root / "generated/amiga-game.dwap").read_bytes()
mask = (root / "generated/amiga-game.exit-mask").read_bytes()
count = struct.unpack_from(">I", pack, 8)[0]
record = next(struct.unpack_from(">HHhhHHHHII", pack, 20 + i * 24)
              for i in range(count)
              if struct.unpack_from(">HH", pack, 20 + i * 24) == (65005, 0))
offset = record[8]
assert len(mask) == 648 * 48
for y in range(648):
    for x in range(384):
        expected = False
        if 110 <= x < 270:
            if y < 284:
                expected = True
            elif y < 364:
                sx = x - 110
                expected = bool(pack[offset + (y - 284) * 20 + sx // 8] & (128 >> (sx % 8)))
        assert bool(mask[y * 48 + x // 8] & (128 >> (x % 8))) == expected, (x, y)
print("248832 exit-mask bits match recovered edge and shaft-only coverage")
