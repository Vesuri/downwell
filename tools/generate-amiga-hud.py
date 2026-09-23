#!/usr/bin/env python3
"""Convert selected recovered art to planar Amiga 2bpp data plus a mask."""

import argparse
import hashlib
import json
import os
import struct
import tempfile
from pathlib import Path

from PIL import Image


MAGIC = b"DWAP"
VERSION = 2
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
        0: None,     # sprPlDead
        3: None,     # sprPlayerShoot
        12: None,    # sprPlayerLegswing
        27: None,    # sprPlayerRunExg
        31: None,    # sprPlayerIdle
        37: None,    # sprPlayerAir
        43: None,    # sprLocked
        44: None,    # sprSelectArrow
        51: None,    # sprTile1
        76: None,    # sprTileSurface
        127: None,   # sprTouchWide (generated wide trigger presentation)
        133: None,   # sprGemSmall
        134: None,   # sprGemLarge
        139: None,   # sprModBtry
        153: None,   # sprBat
        163: None,   # sprEnmSnake
        164: None,   # sprSnakeAir
        166: None,   # sprFrogIdle
        168: None,   # sprFrogAir
        170: None,   # sprFrogPrepare
        177: None,   # sprEnmSnail
        186: None,   # sprGroundedTurtle
        187: None,   # sprGroundedTurtleFall
        188: None,   # sprGroundedTurtleWalk
        278: None,   # sprCrawler
        369: None,   # bgNightsky
        459: [0],    # sprBulScatter1 (usual gunboot projectile)
        550: None,   # sprScattered (generated plants)
        568: None,   # sprDungTorch
        569: None,   # sprGemChunk
        631: None,   # sprTree
        635: None,   # sprGrass
        636: None,   # sprGrass2
        637: None,   # sprGrass3
        638: None,   # sprGrass4
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

TEXT_SELECTIONS = {
    "game": {
        65000: "STYLE SELECT",
        65001: "USUAL STYLE",
        65002: "standard",
        65003: "LOCKED!",
    },
}

# Splash stage 3 is a static 4:3 composition.  Keep its six bounded strings
# as ready-to-blit outlined assets so the 68000 never runs a glyph compositor.
CREDIT_TEXT_SELECTIONS = {
    "game": {
        65200: ("a game by\nOJIRO FUMOTO", "white"),
        65201: ("@MOPPIN_", "red"),
        65202: ("EIRIK SUHRKE", "white"),
        65203: ("@STROTCHY", "red"),
        65204: ("JOONAS TURNER", "white"),
        65205: ("@KISSAKOLME", "red"),
    },
}

# The recovered streak counter uses font 2 and is drawn over the playfield by
# the fixed HUD layer.  Pre-render the bounded two-digit forms so the 68000
# never needs a glyph compositor in the frame loop.
TEXT_SELECTIONS["game"].update({65100 + value: str(value)
                                for value in range(100)})

# The original 320x800 style-transition sprites are almost entirely opaque
# black.  Only the 160x80 gradient edge that crosses the logical playfield
# needs packed pixels; the native renderer supplies the black body as a fill.
DITHER_STRIPS = {
    "game": {
        65004: (129, 80, 0),    # visible centre of sprEnterDither's top edge
        65005: (128, 80, 720),  # visible centre of sprVertDither's bottom edge
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
    plane_size = plane_bytes * height
    output = bytearray(plane_size * 3)
    for y in range(height):
        row = y * plane_bytes
        for x in range(width):
            pen = pens[y * width + x]
            mask = 0x80 >> (x & 7)
            if pen & 1:
                output[row + x // 8] |= mask
            if pen & 2:
                output[plane_size + row + x // 8] |= mask
            if pen:
                output[plane_size * 2 + row + x // 8] |= mask
    return dma_width, bytes(output)


def render_text(manifest, pages, text):
    font = manifest["resources"]["fonts"][2]
    item = manifest["resources"]["texturePageItems"][font["texturePageItemId"]]
    atlas = pages[item["texturePageId"]]
    glyphs = {glyph["character"]: glyph for glyph in font["glyphs"]}
    lines = text.split("\n")
    widths = [sum(glyphs[ord(character)]["shift"] for character in line)
              for line in lines]
    width = max(widths)
    height = 10 + 12 * (len(lines) - 1)
    canvas = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    for line_index, line in enumerate(lines):
        draw_x = 0
        for character in line:
            glyph = glyphs[ord(character)]
            source = atlas.crop((
                item["sourceX"] + glyph["sourceX"],
                item["sourceY"] + glyph["sourceY"],
                item["sourceX"] + glyph["sourceX"] + glyph["sourceWidth"],
                item["sourceY"] + glyph["sourceY"] + glyph["sourceHeight"],
            ))
            canvas.alpha_composite(
                source, (draw_x + glyph["offset"], line_index * 12))
            draw_x += glyph["shift"]
    return canvas


def render_bordered_text(manifest, pages, text, color):
    glyphs = render_text(manifest, pages, text)
    alpha = glyphs.getchannel("A")
    canvas = Image.new("RGBA", (glyphs.width + 2, glyphs.height + 2),
                       (0, 0, 0, 0))
    outline = Image.new("RGBA", glyphs.size, (0, 0, 0, 255))
    outline.putalpha(alpha)
    for position in ((0, 1), (2, 1), (1, 0), (1, 2)):
        canvas.alpha_composite(outline, position)
    foreground = Image.new(
        "RGBA", glyphs.size,
        (255, 0, 0, 255) if color == "red" else (255, 255, 255, 255))
    foreground.putalpha(alpha)
    canvas.alpha_composite(foreground, (1, 1))
    return canvas


def render_dither_strip(sprite, frame, items, pages, source_x, source_y):
    canvas = frame_canvas(sprite, frame, items, pages).crop(
        (source_x, source_y, source_x + 160, source_y + 80))
    pixels = []
    for red, green, blue, alpha in canvas.get_flattened_data():
        pixels.append((255, 255, 255, 255)
                      if alpha != 0 and (red, green, blue) == (255, 255, 255)
                      else (0, 0, 0, 0))
    output = Image.new("RGBA", canvas.size, (0, 0, 0, 0))
    output.putdata(pixels)
    return output


def decode_planar(data, width, height, dma_width):
    plane_bytes = dma_width // 8
    plane_size = plane_bytes * height
    pens = []
    for y in range(height):
        row = y * plane_bytes
        for x in range(width):
            mask = 0x80 >> (x & 7)
            pen = int(bool(data[row + x // 8] & mask))
            pen |= int(bool(data[plane_size + row + x // 8] & mask)) << 1
            require(bool(data[plane_size * 2 + row + x // 8] & mask) ==
                    (pen != 0), "generated mask does not match color planes")
            pens.append(pen)
    return pens


def build_pack(manifest, extracted, profile):
    require(manifest["schemaVersion"] == 3, "unexpected extraction schema")
    require(manifest["source"]["sha256"] == SOURCE_SHA256,
            "manifest does not describe the pinned Steam build")
    resources = manifest["resources"]
    sprites = resources["sprites"]
    items = resources["texturePageItems"]
    pages = [Image.open(extracted / "TexturePages" / f"{index}.png").convert("RGBA")
             for index in range(manifest["counts"]["embeddedTextures"])]

    encoded = []
    for sprite_id in SELECTIONS[profile]:
        sprite = sprites[sprite_id]
        require(sprite["id"] == sprite_id, "sprite IDs are not stable")
        selected_frames = SELECTIONS[profile][sprite_id]
        for frame_index, frame in enumerate(sprite["frames"]):
            if selected_frames is not None and frame_index not in selected_frames:
                continue
            canvas = frame_canvas(sprite, frame, items, pages)
            pens = [pixel_to_pen(pixel) for pixel in canvas.get_flattened_data()]
            dma_width, planar = encode_planar(pens, sprite["width"], sprite["height"])
            require(decode_planar(planar, sprite["width"], sprite["height"], dma_width) == pens,
                    f"planar round trip failed for sprite {sprite_id}:{frame_index}")
            encoded.append((sprite, frame_index, dma_width, planar))

    for text_id, text in TEXT_SELECTIONS.get(profile, {}).items():
        canvas = render_text(manifest, pages, text)
        pens = [pixel_to_pen(pixel) for pixel in canvas.get_flattened_data()]
        dma_width, planar = encode_planar(pens, canvas.width, canvas.height)
        sprite = {
            "id": text_id,
            "originX": canvas.width // 2,
            "originY": 0,
            "width": canvas.width,
            "height": canvas.height,
        }
        encoded.append((sprite, 0, dma_width, planar))

    for text_id, (text, color) in CREDIT_TEXT_SELECTIONS.get(profile, {}).items():
        canvas = render_bordered_text(manifest, pages, text, color)
        pens = [pixel_to_pen(pixel) for pixel in canvas.get_flattened_data()]
        dma_width, planar = encode_planar(pens, canvas.width, canvas.height)
        sprite = {
            "id": text_id,
            "originX": canvas.width // 2,
            "originY": 1,
            "width": canvas.width,
            "height": canvas.height,
        }
        encoded.append((sprite, 0, dma_width, planar))

    for strip_id, (source_id, source_x, source_y) in \
            DITHER_STRIPS.get(profile, {}).items():
        source_sprite = sprites[source_id]
        canvas = render_dither_strip(
            source_sprite, source_sprite["frames"][0], items, pages,
            source_x, source_y)
        pens = [pixel_to_pen(pixel) for pixel in canvas.get_flattened_data()]
        dma_width, planar = encode_planar(pens, canvas.width, canvas.height)
        sprite = {
            "id": strip_id,
            "originX": 0,
            "originY": 0,
            "width": canvas.width,
            "height": canvas.height,
        }
        encoded.append((sprite, 0, dma_width, planar))

    # Keep the on-disk directory ordered so the 68000 renderer can locate a
    # frame with a short binary search instead of repeatedly walking records
    # in Chip RAM.
    encoded.sort(key=lambda item: (item[0]["id"], item[1]))

    payload_offset = HEADER.size + RECORD.size * len(encoded)
    records = []
    payload = bytearray()
    for sprite, frame_index, dma_width, planar in encoded:
        records.append(RECORD.pack(
            sprite["id"], frame_index, sprite["originX"], sprite["originY"],
            sprite["width"], sprite["height"], dma_width, 3,
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
    data = build_pack(manifest, args.extracted, args.profile)
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
