#!/usr/bin/env python3
"""Check entry-cover DMA windows against original sprite alpha, not exit art."""
import importlib.util
import json
from pathlib import Path

root = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("pack", root / "tools/generate-amiga-hud.py")
pack = importlib.util.module_from_spec(spec)
spec.loader.exec_module(pack)
extracted = root / "extracted/utmt-0.9.2.0"
manifest = json.loads((extracted / "manifest.json").read_text())
assert manifest["source"]["sha256"] == pack.SOURCE_SHA256
resources = manifest["resources"]
sprite = resources["sprites"][129]
assert (sprite["width"], sprite["height"], sprite["originX"], sprite["originY"]) == (320,800,160,400)
pages = [pack.Image.open(extracted / "TexturePages" / f"{i}.png").convert("RGBA")
         for i in range(manifest["counts"]["embeddedTextures"])]
canvas = pack.frame_canvas(sprite, sprite["frames"][0], resources["texturePageItems"], pages)
mask = (root / "generated/amiga-game.entry-mask").read_bytes()
assert len(mask) == 48*648
# Compare whole DMA rows, including the untouched HUD margins. Threshold is
# the existing Amiga binary-opacity policy; this does not claim GPU alpha=1
# blending is bit-identical to fully transparent holes.
source = []
for y in range(800):
    row = bytearray(48)
    for x in range(160):
        pixel = canvas.getpixel((80+x,y))
        if pixel[3] >= 128:
            assert pixel[:3] == (0,0,0), (x,y,pixel)
            px = 110+x
            row[px//8] |= 128 >> (px%8)
    source.append(bytes(row))
blank = bytes(48)
for screen_y in range(-116,801):
    row = max(0,min(364,684-screen_y))
    actual = mask[row*48:(row+284)*48]
    expected = b"".join(source[y-screen_y+400] if 0 <= y-screen_y+400 < 800 else blank
                        for y in range(284))
    assert actual == expected, screen_y
print("917 entry-cover viewports match original sprite opacity, including all HUD margins")
