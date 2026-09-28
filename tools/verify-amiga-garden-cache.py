#!/usr/bin/env python3
"""Check both captured garden caches against original RGBA atlas pixels.

The host test validates Create RNG and exact serialized layer membership order
before emitting placements. This checker does not read DWAP pixels or invoke
the native blitter, so a matching host/native planar conversion bug cannot
make the raster comparison pass. It checks the waking frame, before title
animation changes the foreground. Sprite DMA and Copper words have separate
checks in verify_menu_wake_frame.gdb.
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
WIDTH, HEIGHT = 512, 384
PLANE_BYTES = WIDTH * HEIGHT // 8


def check(capture):
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
    expected = [bytearray(PLANE_BYTES), bytearray(PLANE_BYTES)]
    for instance, obj, sprite_id, frame_id, anchor_x, anchor_y in placements:
        placed = original[instance]
        if (obj, anchor_x, anchor_y) != (placed["objectId"], placed["x"], placed["y"]):
            raise ValueError(f"placement differs from original room: {instance}")
        if (placed["scaleX"], placed["scaleY"], placed["rotation"]) != (1, 1, 0):
            raise ValueError(f"unsupported transformed scenery: {instance}")
        sprite = sprites[sprite_id]
        frame = sprite["frames"][frame_id % len(sprite["frames"])]
        item = items[frame["texturePageItemId"]]
        page_id = item["texturePageId"]
        if page_id not in pages:
            pages[page_id] = Image.open(extracted / f"TexturePages/{page_id}.png").convert("RGBA")
        if (item["sourceWidth"], item["sourceHeight"]) != (item["targetWidth"], item["targetHeight"]):
            raise ValueError(f"unsupported scaled garden atlas item {instance}")
        plane = expected[1 if obj == 312 else 0]
        if obj == 312:
            anchor_x, anchor_y = 120, 129
        else:
            anchor_y -= 284
        left = anchor_x - sprite["originX"] + item["targetX"]
        top = anchor_y - sprite["originY"] + item["targetY"]
        pixels = pages[page_id].load()
        for sy in range(item["sourceHeight"]):
            y = top + sy
            if not 0 <= y < HEIGHT:
                continue
            for sx in range(item["sourceWidth"]):
                x = left + sx
                if not 0 <= x < WIDTH:
                    continue
                r, g, b, alpha = pixels[item["sourceX"] + sx, item["sourceY"] + sy]
                if not alpha:
                    continue
                if alpha != 255 or (r, g, b) not in ((0, 0, 0), (255, 255, 255)):
                    raise ValueError(f"non-monochrome/partial-alpha scenery {instance}")
                offset, bit = y * (WIDTH // 8) + x // 8, 0x80 >> (x & 7)
                if r:
                    plane[offset] |= bit
                else:
                    plane[offset] &= 255 ^ bit
    payload = capture.read_bytes()
    if len(payload) != 12 + 2 * PLANE_BYTES + 136 or struct.unpack_from(">H", payload, 10)[0] != 0:
        raise ValueError("expected waking frame with two caches and one player sprite")
    for index, name in enumerate(("foreground", "background")):
        actual = payload[12 + index * PLANE_BYTES:12 + (index + 1) * PLANE_BYTES]
        errors = [(i, a ^ b) for i, (a, b) in enumerate(zip(actual, expected[index])) if a != b]
        if errors:
            first, bits = errors[0]
            raise ValueError(f"{name}: {len(errors)} differing bytes; first at "
                             f"x={first % 64 * 8}, y={first // 64}, xor={bits:02x}")
    print(f"Garden caches: all {2 * WIDTH * HEIGHT} bits match source RGBA art, "
          f"{len(placements)} source-ordered placements")
    print(f"Capture SHA-256: {hashlib.sha256(payload).hexdigest()}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("capture", type=Path)
    check(parser.parse_args().capture)
