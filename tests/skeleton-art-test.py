#!/usr/bin/env python3
"""Compare offline skeleton planar/DMA artwork at source-relative anchors."""
import importlib.util
import json
import struct
from pathlib import Path

root = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("pack", root / "tools/generate-amiga-hud.py")
pack = importlib.util.module_from_spec(spec)
spec.loader.exec_module(pack)
extracted = root / "extracted/utmt-0.9.2.0"
manifest = json.loads((extracted / "manifest.json").read_text())
assert manifest["source"]["sha256"] == pack.SOURCE_SHA256
resources = manifest["resources"]
pages = [pack.Image.open(extracted / "TexturePages" / f"{i}.png").convert("RGBA")
         for i in range(manifest["counts"]["embeddedTextures"])]
data = (root / "generated/amiga-game.dwap").read_bytes()
magic, version, size, count, source_hash = pack.HEADER.unpack_from(data)
assert (magic, version, size, source_hash) == (pack.MAGIC, pack.VERSION, pack.HEADER.size,
                                             bytes.fromhex(pack.SOURCE_SHA256)[:8])
records = {}
for i in range(count):
    row = pack.RECORD.unpack_from(data, size + i * pack.RECORD.size)
    assert row[:2] not in records
    records[row[:2]] = row

def planar_pixels(row):
    _, _, ox, oy, width, height, stride, flags, offset, length = row
    assert flags == 3
    pixels = pack.decode_planar(data[offset:offset+length], width, height, stride)
    return {(x-ox, y-oy): pixels[y*width+x]
            for y in range(height) for x in range(width) if pixels[y*width+x]}

checked = 0
for ident in (205, 206, 207, 208, 460):
    sprite = resources["sprites"][ident]
    for frame, source_frame in enumerate(sprite["frames"]):
        canvas = pack.frame_canvas(sprite, source_frame, resources["texturePageItems"], pages)
        for flip in (0, 1):
            expected = {}
            for y in range(canvas.height):
                for x in range(canvas.width):
                    pen = pack.pixel_to_pen(canvas.getpixel((x,y)))
                    if pen:
                        wx = sprite["originX"]-1-x if flip else x-sprite["originX"]
                        expected[wx, y-sprite["originY"]] = pen
            assert planar_pixels(records[ident+62000*flip, frame]) == expected, (ident,frame,flip,"planar")
            checked += 1
            if ident == 206:
                assert (63000+ident, frame*8+flip*4) not in records
                continue
            first = records[63000+ident, frame*8+flip*4]
            parts = first[7] >> 8
            assert 1 <= parts <= 4
            actual = {}
            for part in range(parts):
                row = records[63000+ident, frame*8+flip*4+part]
                _, _, ox, oy, width, height, stride, flags, offset, length = row
                assert width == stride == 16 and flags == 4 | (parts << 8)
                assert length == height*4
                for y in range(height):
                    a,b = struct.unpack_from(">HH", data, offset+y*4)
                    for x in range(16):
                        pen = ((a >> (15-x)) & 1) | (((b >> (15-x)) & 1) << 1)
                        if pen:
                            key = (x-ox, y-oy)
                            assert key not in actual or actual[key] == pen
                            actual[key] = pen
            assert actual == expected, (ident,frame,flip,"DMA")
            checked += 1
print(f"Skeleton/bone artwork: {checked} anchored planar/DMA facings match original pixels")
