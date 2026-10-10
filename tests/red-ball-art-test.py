#!/usr/bin/env python3
"""Compare anchored red-ball planar pixels with the untouched atlas export."""
import importlib.util
import json
from pathlib import Path

root = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("art", root / "tools/generate-amiga-hud.py")
art = importlib.util.module_from_spec(spec)
spec.loader.exec_module(art)
extracted = root / "extracted/utmt-0.9.2.0"
manifest = json.loads((extracted / "manifest.json").read_text())
assert manifest["source"]["sha256"] == art.SOURCE_SHA256
resources = manifest["resources"]
pages = [art.Image.open(extracted / "TexturePages" / f"{i}.png").convert("RGBA")
         for i in range(manifest["counts"]["embeddedTextures"])]
data = (root / "generated/amiga-game.dwap").read_bytes()
magic, version, header, count, source = art.HEADER.unpack_from(data)
assert magic == b"DWAP" and version == 2 and source == bytes.fromhex(art.SOURCE_SHA256)[:8]
records = {}
for i in range(count):
    record = art.RECORD.unpack_from(data, header + i * art.RECORD.size)
    assert record[:2] not in records
    records[record[:2]] = record
checked = 0
for ident in (181, 182, 183, 184, 185, 702, 703):
    sprite = resources["sprites"][ident]
    assert len(sprite["frames"]) == (4 if ident == 702 else 1 if ident == 703 else 5)
    for frame, metadata in enumerate(sprite["frames"]):
        canvas = art.frame_canvas(sprite, metadata, resources["texturePageItems"], pages)
        for flip in (False, True):
            expected = {}
            for y in range(canvas.height):
                for x in range(canvas.width):
                    pen = art.pixel_to_pen(canvas.getpixel((x, y)))
                    if pen:
                        dx = sprite["originX"] - x - 1 if flip else x - sprite["originX"]
                        expected[dx, y - sprite["originY"]] = pen
            _, _, ox, oy, w, h, stride, planes, offset, size = records[(62000 + ident if flip else ident, frame)]
            assert planes == 3
            pixels = art.decode_planar(data[offset:offset + size], w, h, stride)
            actual = {(x - ox, y - oy): pixels[y * w + x]
                      for y in range(h) for x in range(w) if pixels[y * w + x]}
            assert actual == expected, (ident, frame, flip)
            # Hardware variants must also have been built offline, with at
            # least the first channel present for each facing and frame.
            dma = records[63000 + ident, frame * 8 + int(flip) * 4]
            assert dma[7] & 4
            checked += 1
print(f"Red-ball art: {checked} anchored planar images match original atlas pixels; offline DMA variants present")
