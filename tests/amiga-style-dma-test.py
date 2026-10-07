#!/usr/bin/env python3
"""Verify style DMA art, writable header ownership and retained-bank lifetime."""
import re
import struct
from pathlib import Path

pack = Path("generated/amiga-front.dwap").read_bytes()
magic, version, directory, count, source = struct.unpack_from(">4sHHI8s", pack)
assert (magic, version, directory) == (b"DWAP", 2, 20)
assert source.hex() == "c07daf5fbe940ae0"
records = {}
for index in range(count):
    record = struct.unpack_from(">HHhhHHHHII", pack, directory + index * 24)
    assert record[:2] not in records
    records[record[:2]] = record
prefix = int(re.search(r"DOWNWELL_FRONT_STYLE_BYTES (\d+)UL",
    Path("generated/amiga-scene-banks.h").read_text()).group(1))
ranges = []
checks = 0
for ident, frames in ((65510, 8), (65511, 4), (65512, 1), (65513, 1)):
    for frame in range(frames):
        r = records[ident, frame]
        original = records[ident - 10, 0 if ident == 65511 else frame]
        _, _, ox, oy, width, height, dma, flags, offset, size = r
        _, _, ax, ay, aw, ah, adma, aflags, aoffset, asize = original
        assert (ox, oy, width, height, dma, flags, size) == (ax, ay, 16, ah, 16, 4, 272)
        assert offset % 2 == 0 and offset + size <= prefix <= len(pack)
        assert all(offset + size <= first or offset >= last for first, last in ranges)
        ranges.append((offset, offset + size))
        stride = adma // 8
        plane = stride * ah
        for buffer in range(2):
            stream = pack[offset + buffer * 136:offset + (buffer + 1) * 136]
            assert stream[:4] == bytes(4)
            assert stream[4 + height * 4:] == bytes(136 - 4 - height * 4)
            for y in range(height):
                low, high = struct.unpack_from(">HH", stream, 4 + y * 4)
                for x in range(16):
                    pen = bool(low & (0x8000 >> x)) + 2 * bool(high & (0x8000 >> x))
                    expected = 0
                    if x < aw:
                        at = aoffset + y * stride + x // 8
                        bit = 0x80 >> (x & 7)
                        expected = bool(pack[at] & bit) + 2 * bool(pack[at + plane] & bit)
                        assert bool(expected) == bool(pack[at + plane * 2] & bit)
                    assert pen == expected
                    checks += 1
assert len(ranges) == 14
print(f"Style DMA: {checks} pixels, 28 independent streams, retained prefix and opaque-black coverage exact")
