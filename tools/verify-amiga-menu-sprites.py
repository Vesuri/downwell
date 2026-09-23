#!/usr/bin/env python3
"""Compare host-prepared garden DMA streams with the former pixel builder."""
import struct
from pathlib import Path


def verify(path):
    pack = path.read_bytes()
    magic, version, start, count, _ = struct.unpack_from(">4sHHI8s", pack)
    assert magic == b"DWAP" and version == 2
    records = {}
    for i in range(count):
        r = struct.unpack_from(">HHhhHHHHII", pack, start + 24 * i)
        records[r[:2]] = r
    checked = 0
    for (sprite, frame), r in records.items():
        if sprite not in (12, 27, 31, 37):
            continue
        _, _, ox, oy, width, height, dma_width, _, offset, _ = r
        row_bytes = dma_width // 8
        plane_bytes = row_bytes * height
        def pixel(x, y, plane):
            return bool(pack[offset + plane * plane_bytes + y * row_bytes + x // 8]
                        & (0x80 >> (x & 7)))
        occupied = [x for y in range(height) for x in range(width)
                    if pixel(x, y, 0) or pixel(x, y, 1)]
        left, right = (min(occupied), max(occupied)) if occupied else (0, 0)
        for flip in (0, 1):
            low, high = (width - 1 - right, width - 1 - left) if flip else (left, right)
            crop = max(0, min(width - 16, low - (16 - (high - low + 1)) // 2))
            prepared = records[64000 + sprite, frame * 2 + flip]
            assert prepared[2:8] == ((width - ox if flip else ox) - crop,
                                      oy, 16, height, 16, 4)
            data = pack[prepared[8]:prepared[8] + prepared[9]]
            expected = bytearray(136)
            for y in range(height):
                for plane in (0, 1):
                    bits = 0
                    for x in range(16):
                        source_x = width - 1 - crop - x if flip else crop + x
                        if pixel(source_x, y, plane):
                            bits |= 0x8000 >> x
                    struct.pack_into(">H", expected, 4 + y * 4 + plane * 2, bits)
            assert data == expected * 2, (sprite, frame, flip)
            checked += 1
    assert checked == 50
    print(f"Garden hardware sprites: {checked} mirrored/frame variants, both DMA copies match")


if __name__ == "__main__":
    verify(Path(__file__).resolve().parent.parent / "generated/amiga-game.dwap")
