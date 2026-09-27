#!/usr/bin/env python3
"""Check all wall-impact planar and DMA pixels at the original sprite origin."""
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
rows = [pack.RECORD.unpack_from(data, size + i * pack.RECORD.size) for i in range(count)]
records = {row[:2]: row for row in rows}
assert len(records) == count
sprite = resources["sprites"][101]
assert sprite["name"] == "sprFxBulletHitWall" and len(sprite["frames"]) == 8 and not sprite["smooth"]
checks = 0
for frame, source in enumerate(sprite["frames"]):
    canvas = pack.frame_canvas(sprite, source, resources["texturePageItems"], pages)
    ox, oy = sprite["originX"], sprite["originY"]
    expected = {(x-ox, y-oy): pack.pixel_to_pen(canvas.getpixel((x, y)))
                for y in range(canvas.height) for x in range(canvas.width)
                if pack.pixel_to_pen(canvas.getpixel((x, y)))}
    _, _, px, py, width, height, stride, flags, offset, length = records[101, frame]
    assert flags == 3
    pixels = pack.decode_planar(data[offset:offset+length], width, height, stride)
    actual = {(x-px, y-py): pixels[y*width+x] for y in range(height)
              for x in range(width) if pixels[y*width+x]}
    assert actual == expected, (frame, "planar")
    checks += 1
    for flip in (0, 1):
        wanted = {((-x-1, y) if flip else (x, y)): pen for (x, y), pen in expected.items()}
        parts = records[63101, frame*8+flip*4][7] >> 8
        assert 1 <= parts <= 4
        actual = {}
        for part in range(parts):
            _, _, px, py, width, height, stride, flags, offset, length = records[63101, frame*8+flip*4+part]
            assert width == stride == 16 and flags == 4 | (parts << 8) and length == height*4
            for y in range(height):
                a, b = struct.unpack_from(">HH", data, offset+y*4)
                for x in range(16):
                    pen = ((a >> (15-x)) & 1) | (((b >> (15-x)) & 1) << 1)
                    if pen:
                        key = (x-px, y-py)
                        assert key not in actual or actual[key] == pen
                        actual[key] = pen
        assert actual == wanted, (frame, flip, "DMA")
        checks += 1
print(f"Wall impact: {checks} anchored planar/DMA variants match all eight source frames")
