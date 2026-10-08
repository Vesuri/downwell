#!/usr/bin/env python3
"""Reject unrotated/half-turn and rounded-anchor shortcuts for blood art.

This uses the offline nearest-pixel model, not an original Windows GPU capture.
It shows that the existing integer-anchor muzzle proof cannot be generalized.
"""
import importlib.util
import json
import math
from pathlib import Path

root = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("pack", root / "tools/generate-amiga-hud.py")
pack = importlib.util.module_from_spec(spec)
spec.loader.exec_module(pack)
extracted = root / "extracted/utmt-0.9.2.0"
manifest = json.loads((extracted / "manifest.json").read_text())
assert manifest["source"]["sha256"] == pack.SOURCE_SHA256
resources = manifest["resources"]
sprite = resources["sprites"][84]
assert sprite["name"] == "sprSmoke" and not sprite["smooth"]
pages = [pack.Image.open(extracted / "TexturePages" / f"{i}.png").convert("RGBA")
         for i in range(manifest["counts"]["embeddedTextures"])]

def raster(canvas, degrees, phase_x=0, phase_y=0):
    angle = math.radians(degrees)
    c, s = math.cos(angle), math.sin(angle)
    # A conservative square containing every orientation of the whole source
    # rectangle around its origin, including transparent borders.
    radius = math.ceil(math.hypot(canvas.width, canvas.height)) + 2
    result = {}
    for y in range(-radius, radius):
        for x in range(-radius, radius):
            dx, dy = x + 0.5 - phase_x, y + 0.5 - phase_y
            sx = math.floor(sprite["originX"] + c*dx - s*dy)
            sy = math.floor(sprite["originY"] + s*dx + c*dy)
            if 0 <= sx < canvas.width and 0 <= sy < canvas.height:
                pen = pack.pixel_to_pen(canvas.getpixel((sx, sy)))
                if pen:
                    result[x, y] = pen
    return result

angle_witnesses = phase_witnesses = 0
for frame in sprite["frames"]:
    canvas = pack.frame_canvas(sprite, frame, resources["texturePageItems"], pages)
    unrotated = raster(canvas, 0)
    half_turn = raster(canvas, 180)
    for degrees in (0.25, 5, 10, 15, 19.75, 160.25, 165, 170, 175, 179.75):
        base = raster(canvas, degrees)
        angle_witnesses += base != (unrotated if degrees < 90 else half_turn)
        for phase_x, phase_y in ((0.25, 0), (0, 0.25), (0.25, 0.25)):
            # All these anchors round to zero, yet their inverse samples can
            # differ: preserve the fractional state until rasterization.
            phase_witnesses += raster(canvas, degrees, phase_x, phase_y) != base
assert angle_witnesses and phase_witnesses
print(f"Blood 84 nearest-model counterexamples: {angle_witnesses} angle and {phase_witnesses} fractional-anchor variants")
