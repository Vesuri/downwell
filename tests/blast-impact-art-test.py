#!/usr/bin/env python3
"""Compare blast-impact planar variants against source pixels at their anchor."""
import importlib.util
import json
from pathlib import Path

root = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("pack", root / "tools/generate-amiga-hud.py")
pack = importlib.util.module_from_spec(spec)
spec.loader.exec_module(pack)
export = root / "extracted/utmt-0.9.2.0"
manifest = json.loads((export / "manifest.json").read_text())
assert manifest["source"]["sha256"] == pack.SOURCE_SHA256
resources = manifest["resources"]
sprite = resources["sprites"][605]
assert sprite["name"] == "sprFxImpactSplosion" and len(sprite["frames"]) == 3 and not sprite["smooth"]
pages = [pack.Image.open(export / "TexturePages" / f"{i}.png").convert("RGBA")
         for i in range(manifest["counts"]["embeddedTextures"])]
data = (root / "generated/amiga-game.dwap").read_bytes()
magic, version, size, count, digest = pack.HEADER.unpack_from(data)
assert (magic, version, digest) == (pack.MAGIC, pack.VERSION, bytes.fromhex(pack.SOURCE_SHA256)[:8])
rows = [pack.RECORD.unpack_from(data, size + i * pack.RECORD.size) for i in range(count)]
records = {row[:2]: row for row in rows}
for frame, source in enumerate(sprite["frames"]):
    canvas = pack.frame_canvas(sprite, source, resources["texturePageItems"], pages)
    expected = {(x - sprite["originX"], y - sprite["originY"]): pack.pixel_to_pen(canvas.getpixel((x, y)))
                for y in range(canvas.height) for x in range(canvas.width)
                if pack.pixel_to_pen(canvas.getpixel((x, y)))}
    for turn in (0, 3):
        _, _, ox, oy, width, height, stride, flags, offset, length = records[605, turn * 256 + frame]
        assert flags == 3
        pixels = pack.decode_planar(data[offset:offset+length], width, height, stride)
        actual = {(x-ox, y-oy): pixels[y*width+x] for y in range(height) for x in range(width) if pixels[y*width+x]}
        wanted = {((-y-1, x) if turn else (x, y)): pen for (x, y), pen in expected.items()}
        assert actual == wanted, (frame, turn)
print("Blast impact: six planar variants match source pixels and non-square anchors")
