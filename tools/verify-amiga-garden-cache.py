#!/usr/bin/env python3
"""Check both captured garden caches against original RGBA atlas pixels.

The host test validates Create RNG and exact serialized layer membership order
before emitting placements. This checker does not read DWAP pixels or invoke
the native blitter, so a matching host/native planar conversion bug cannot
make the raster comparison pass. Title and entry checks include the final
title frame at the source Step position. Sprite DMA and Copper words have
separate debugger checks.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess

from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
SOURCE = "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"


def check(capture, scene="wake"):
    extracted = ROOT / "extracted/utmt-0.9.2.0"
    manifest = json.loads((extracted / "manifest.json").read_text())
    if manifest["source"]["sha256"] != SOURCE:
        raise ValueError("unexpected source manifest")
    result = subprocess.run(
        [str(ROOT / "build/amiga-menu-test"), "generated/downwell-rooms.dwrm",
         "generated/downwell-objects.dwob"], cwd=ROOT,
        env={**os.environ, "DOWNWELL_DUMP_GARDEN_SCENERY": "1"},
        check=True, text=True, capture_output=True, timeout=30)
    placements = [list(map(int, line.split()[1:])) for line in result.stdout.splitlines()
                  if line.startswith("SCENERY ")]
    if not placements:
        raise ValueError("no verified scenery placements")
    resources = manifest["resources"]
    original = {i["instanceId"]: i for i in resources["rooms"][20]["instances"]}
    sprites = {s["id"]: s for s in resources["sprites"]}
    items = {i["id"]: i for i in resources["texturePageItems"]}
    pages = {}
    geometry = {"foreground": (512, 384, 0, 284), "background": (512, 384, 0, 0),
                "entry": (192, 1152, 240, 0)}
    expected = {name: bytearray(w * h // 8) for name, (w, h, _, _) in geometry.items()}

    def draw(name, sprite_id, frame_id, anchor_x, anchor_y):
        width, height, world_x, world_y = geometry[name]
        plane = expected[name]
        sprite = sprites[sprite_id]
        frame = sprite["frames"][frame_id % len(sprite["frames"])]
        item = items[frame["texturePageItemId"]]
        page_id = item["texturePageId"]
        if page_id not in pages:
            pages[page_id] = Image.open(extracted / f"TexturePages/{page_id}.png").convert("RGBA")
        if (item["sourceWidth"], item["sourceHeight"]) != (item["targetWidth"], item["targetHeight"]):
            raise ValueError(f"unsupported scaled garden atlas sprite {sprite_id}")
        left = anchor_x - world_x - sprite["originX"] + item["targetX"]
        top = anchor_y - world_y - sprite["originY"] + item["targetY"]
        pixels = pages[page_id].load()
        for sy in range(item["sourceHeight"]):
            y = top + sy
            if not 0 <= y < height:
                continue
            for sx in range(item["sourceWidth"]):
                x = left + sx
                if not 0 <= x < width:
                    continue
                r, g, b, alpha = pixels[item["sourceX"] + sx, item["sourceY"] + sy]
                if not alpha:
                    continue
                if alpha != 255 or (r, g, b) not in ((0, 0, 0), (255, 255, 255)):
                    raise ValueError(f"non-monochrome/partial-alpha sprite {sprite_id}")
                offset, bit = y * (width // 8) + x // 8, 0x80 >> (x & 7)
                if r:
                    plane[offset] |= bit
                else:
                    plane[offset] &= 255 ^ bit

    title_drawn = scene == "wake"
    for instance, obj, sprite_id, frame_id, anchor_x, anchor_y, depth in placements:
        placed = original[instance]
        if (obj, anchor_x, anchor_y) != (placed["objectId"], placed["x"], placed["y"]):
            raise ValueError(f"placement differs from original room: {instance}")
        if (placed["scaleX"], placed["scaleY"], placed["rotation"]) != (1, 1, 0):
            raise ValueError(f"unsupported transformed scenery: {instance}")
        if not title_drawn and depth <= 0:
            # objWell Create: (x+56,y-80). Title Step at final frame:
            # y=ystart-3*image_index/(image_number-1). Dynamic depth zero
            # draws before the serialized fixed depth-zero layer.
            well = next(i for i in original.values() if i["objectId"] == 315)
            title = sprites[654]
            for name in ("foreground", "entry"):
                draw(name, 654, len(title["frames"])-1, well["x"]+56, well["y"]-83)
            title_drawn = True
        for name in (("background",) if obj == 312 else ("foreground", "entry")):
            draw(name, sprite_id, frame_id, 120 if obj == 312 else anchor_x,
                 129 if obj == 312 else anchor_y)
    if not title_drawn:
        raise ValueError("title layer boundary not reached")
    payload = capture.read_bytes()
    names = ("entry", "background", "foreground") if scene == "entry" else ("foreground", "background")
    expected_size = 12 + sum(len(expected[name]) for name in names) + (0 if scene == "entry" else 136)
    if len(payload) != expected_size or struct.unpack_from(">H", payload, 10)[0] != 0:
        raise ValueError(f"unexpected {scene} capture layout")
    offset = 12
    for name in names:
        actual = payload[offset:offset+len(expected[name])]
        offset += len(actual)
        errors = [(i, a ^ b) for i, (a, b) in enumerate(zip(actual, expected[name])) if a != b]
        if errors:
            first, bits = errors[0]
            raise ValueError(f"{name}: {len(errors)} differing bytes; first at "
                             f"x={first % (geometry[name][0]//8) * 8}, "
                             f"y={first // (geometry[name][0]//8)}, xor={bits:02x}")
    print(f"Garden {scene} caches: all {sum(len(expected[n])*8 for n in names)} bits match source RGBA art, "
          f"{len(placements)} source-ordered placements")
    print(f"Capture SHA-256: {hashlib.sha256(payload).hexdigest()}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("capture", type=Path)
    parser.add_argument("--scene", choices=("wake", "title", "entry"), default="wake")
    args = parser.parse_args()
    check(args.capture, args.scene)
