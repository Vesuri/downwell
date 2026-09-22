#!/usr/bin/env python3
"""Convert selected recovered art to interleaved Amiga 2bpp data."""

import argparse
import hashlib
import json
import os
import struct
import tempfile
from pathlib import Path

from PIL import Image


MAGIC = b"DWAP"
VERSION = 1
SOURCE_SHA256 = "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"
HEADER = struct.Struct(">4sHHI8s")
RECORD = struct.Struct(">HHhhHHHHII")

# Exact resources needed by the first native HUD and gameplay milestones.
SELECTIONS = {
    "hud": {
        376: None,   # sprHudCurrency
        408: None,   # sprTabletBorder
        413: None,   # sprSpriteNumber (white/red/black digit banks)
        414: None,   # sprSpriteSlash
        415: None,   # sprPcHudHpGauge
        416: None,   # sprPcHudStammoGauge
    },
    "game": {
        12: None,    # sprPlayerLegswing
        27: None,    # sprPlayerRunExg
        31: None,    # sprPlayerIdle
        37: None,    # sprPlayerAir
        43: None,    # sprLocked
        44: None,    # sprSelectArrow
        51: None,    # sprTile1
        76: None,    # sprTileSurface
        369: None,   # bgNightsky
        631: None,   # sprTree
        635: None,   # sprGrass
        640: None,   # sprBench
        642: None,   # sprWell2
        646: None,   # sprTrees
        654: None,   # sprTitleFade
        729: None,   # infosp
        695: None,   # sprControls
        696: None,   # sprDevolverLogo
        733: None,   # sprDitherFade
    },
}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def pixel_to_pen(pixel):
    red, green, blue, alpha = pixel
    if alpha == 0:
        return 0
    if (red, green, blue) == (255, 255, 255):
        return 1
    if (red, green, blue) == (255, 0, 0):
        return 2
    if (red, green, blue) == (0, 0, 0):
        return 3
    raise ValueError(f"HUD pixel is outside the exact four-pen palette: {pixel}")


def frame_canvas(sprite, frame, items, pages):
    item = items[frame["texturePageItemId"]]
    source = pages[item["texturePageId"]].crop((
        item["sourceX"], item["sourceY"],
        item["sourceX"] + item["sourceWidth"],
        item["sourceY"] + item["sourceHeight"],
    ))
    if source.size != (item["targetWidth"], item["targetHeight"]):
        source = source.resize((item["targetWidth"], item["targetHeight"]),
                               Image.Resampling.NEAREST)
    canvas = Image.new("RGBA", (sprite["width"], sprite["height"]), (0, 0, 0, 0))
    canvas.alpha_composite(source, (item["targetX"], item["targetY"]))
    return canvas


def encode_planar(pens, width, height):
    dma_width = (width + 15) & ~15
    plane_bytes = dma_width // 8
    output = bytearray(plane_bytes * 2 * height)
    for y in range(height):
        row = y * plane_bytes * 2
        for x in range(width):
            pen = pens[y * width + x]
            mask = 0x80 >> (x & 7)
            if pen & 1:
                output[row + x // 8] |= mask
            if pen & 2:
                output[row + plane_bytes + x // 8] |= mask
    return dma_width, bytes(output)


def decode_planar(data, width, height, dma_width):
    plane_bytes = dma_width // 8
    pens = []
    for y in range(height):
        row = y * plane_bytes * 2
        for x in range(width):
            mask = 0x80 >> (x & 7)
            pen = int(bool(data[row + x // 8] & mask))
            pen |= int(bool(data[row + plane_bytes + x // 8] & mask)) << 1
            pens.append(pen)
    return pens


def build_pack(manifest, extracted, selection):
    require(manifest["schemaVersion"] == 3, "unexpected extraction schema")
    require(manifest["source"]["sha256"] == SOURCE_SHA256,
            "manifest does not describe the pinned Steam build")
    resources = manifest["resources"]
    sprites = resources["sprites"]
    items = resources["texturePageItems"]
    pages = [Image.open(extracted / "TexturePages" / f"{index}.png").convert("RGBA")
             for index in range(manifest["counts"]["embeddedTextures"])]

    encoded = []
    for sprite_id in selection:
        sprite = sprites[sprite_id]
        require(sprite["id"] == sprite_id, "sprite IDs are not stable")
        for frame_index, frame in enumerate(sprite["frames"]):
            canvas = frame_canvas(sprite, frame, items, pages)
            pens = [pixel_to_pen(pixel) for pixel in canvas.get_flattened_data()]
            dma_width, planar = encode_planar(pens, sprite["width"], sprite["height"])
            require(decode_planar(planar, sprite["width"], sprite["height"], dma_width) == pens,
                    f"planar round trip failed for sprite {sprite_id}:{frame_index}")
            encoded.append((sprite, frame_index, dma_width, planar))

    payload_offset = HEADER.size + RECORD.size * len(encoded)
    records = []
    payload = bytearray()
    for sprite, frame_index, dma_width, planar in encoded:
        records.append(RECORD.pack(
            sprite["id"], frame_index, sprite["originX"], sprite["originY"],
            sprite["width"], sprite["height"], dma_width, 2,
            payload_offset + len(payload), len(planar)))
        payload.extend(planar)

    source_prefix = bytes.fromhex(SOURCE_SHA256)[:8]
    return b"".join((
        HEADER.pack(MAGIC, VERSION, HEADER.size, len(encoded), source_prefix),
        b"".join(records), bytes(payload)))


def atomic_write(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    descriptor, temporary = tempfile.mkstemp(prefix=path.name + ".", dir=path.parent)
    try:
        with os.fdopen(descriptor, "wb") as output:
            output.write(data)
        os.replace(temporary, path)
        os.chmod(path, 0o644)
    except BaseException:
        os.unlink(temporary)
        raise


def main():
    root = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser()
    parser.add_argument("--extracted", type=Path,
                        default=root / "extracted/utmt-0.9.2.0")
    parser.add_argument("--profile", choices=SELECTIONS, default="hud")
    parser.add_argument("--output", type=Path)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()

    manifest = json.loads((args.extracted / "manifest.json").read_text(encoding="utf-8"))
    if args.output is None:
        args.output = root / f"generated/amiga-{args.profile}.dwap"
    data = build_pack(manifest, args.extracted, SELECTIONS[args.profile])
    digest = hashlib.sha256(data).hexdigest()
    if args.check:
        require(args.output.is_file(), f"missing Amiga asset pack: {args.output}")
        require(args.output.read_bytes() == data,
                "Amiga asset pack is not reproducible")
        print(f"Amiga {args.profile} pack verified: {len(data)} bytes sha256 {digest}")
    else:
        atomic_write(args.output, data)
        print(f"Amiga {args.profile} pack written: {len(data)} bytes sha256 {digest}")


if __name__ == "__main__":
    main()
