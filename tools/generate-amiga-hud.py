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
        1: None,     # sprPlayerSpin
        3: None,     # sprPlayerShoot
        12: None,    # sprPlayerLegswing
        15: None,    # sprPlayerBalancing
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
        173: None,   # sprFrogDamage
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

# Splash credits are centred on these recovered application-surface X
# coordinates.  Their visible pixels must stay there, but the A500 cache
# composer must start every masked blit on a word boundary: the OCS blitter's
# shifted B channel otherwise carries pixels from the end of one row into the
# beginning of the next.  Transparent left padding moves the record origin by
# the same amount, preserving the visible placement while aligning the blit.
CREDIT_TEXT_ANCHOR_X = {
    65200: 190,
    65201: 190,
    65202: 278,
    65203: 278,
    65204: 102,
    65205: 102,
}

# The recovered streak counter uses font 2 and is drawn over the playfield by
# the fixed HUD layer.  Pre-render the bounded two-digit forms so the 68000
# never needs a glyph compositor in the frame loop.
TEXT_SELECTIONS["game"].update({65100 + value: str(value)
                                for value in range(100)})

# The original 320x800 style-transition sprites are almost entirely opaque
# black.  Only the 160x80 gradient edge that crosses the logical playfield
# needs packed opacity; the native renderer pads it into an immutable PF2
# mask with transparent/solid rows before presenting the scene.
DITHER_STRIPS = {
    "game": {
        65004: (129, 80, 0),    # visible centre of sprEnterDither's top edge
        65005: (128, 80, 720),  # visible centre of sprVertDither's bottom edge
    },
}

# sprTitleFade is centred at an eight-pixel destination offset in the wide
# garden cache.  Give that use a host-generated, word-aligned form so the
# 68000 never asks the blitter's shifted B channel to carry a mask between
# rows.  The original resource remains available for the already aligned
# narrow well-entry cache.
MENU_TITLE_ALIGNED_ID = 65400
MENU_TITLE_LEFT_PADDING = 8


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
    # Encode opacity, not RGB: these sprites have opaque black cover pixels
    # and white pixels with alpha=1 (effectively transparent holes).
    # The packed white bits feed an opaque-black Amiga PF2 palette entry.
    canvas = frame_canvas(sprite, frame, items, pages).crop(
        (source_x, source_y, source_x + 160, source_y + 80))
    pixels = []
    for red, green, blue, alpha in canvas.get_flattened_data():
        pixels.append((255, 255, 255, 255)
                      if alpha >= 128
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
            if profile == "game" and sprite_id == 278:
                # Crawler quarter turns are prepared on the host. Crop the
                # transparent 32x32 padding to keep all variants Chip-RAM cheap.
                for turn in range(4):
                    variant = canvas if turn == 0 else canvas.transpose((
                        Image.Transpose.ROTATE_90, Image.Transpose.ROTATE_180,
                        Image.Transpose.ROTATE_270)[turn - 1])
                    ox, oy = sprite["originX"], sprite["originY"]
                    for _ in range(turn):
                        ox, oy = oy, canvas.width - ox
                    bounds = variant.getbbox()
                    require(bounds is not None, "empty crawler frame")
                    variant = variant.crop(bounds)
                    ox -= bounds[0]
                    oy -= bounds[1]
                    if variant.width < 16:
                        padded = Image.new("RGBA", (16, variant.height))
                        padded.paste(variant, (0, 0))
                        variant = padded
                    for flip_x in (False, True):
                        image = variant.transpose(Image.Transpose.FLIP_LEFT_RIGHT) if flip_x else variant
                        pixels = [pixel_to_pen(pixel) for pixel in image.get_flattened_data()]
                        stride, data = encode_planar(pixels, image.width, image.height)
                        require(decode_planar(data, image.width, image.height, stride) == pixels,
                                "crawler rotation planar round trip failed")
                        metadata = dict(sprite, id=62000 + sprite_id if flip_x else sprite_id,
                                        width=image.width, height=image.height,
                                        originX=image.width - ox if flip_x else ox, originY=oy)
                        encoded.append((metadata, turn * 256 + frame_index, stride, data))
                continue
            pens = [pixel_to_pen(pixel) for pixel in canvas.get_flattened_data()]
            dma_width, planar = encode_planar(pens, sprite["width"], sprite["height"])
            require(decode_planar(planar, sprite["width"], sprite["height"], dma_width) == pens,
                    f"planar round trip failed for sprite {sprite_id}:{frame_index}")
            encoded.append((sprite, frame_index, dma_width, planar))
            if profile == "game" and sprite_id == 177:
                # enmSnail Step sets image_yscale=-sign(ysp). Keep the real
                # resource ID; synthetic frame 256+n is the offline Y flip.
                vertical = canvas.transpose(Image.Transpose.FLIP_TOP_BOTTOM)
                for flip_x in (False, True):
                    variant = vertical.transpose(Image.Transpose.FLIP_LEFT_RIGHT) if flip_x else vertical
                    pixels = [pixel_to_pen(pixel) for pixel in variant.get_flattened_data()]
                    stride, data = encode_planar(pixels, variant.width, variant.height)
                    metadata = dict(sprite, id=62000 + sprite_id if flip_x else sprite_id,
                                    originY=sprite["height"] - sprite["originY"],
                                    originX=sprite["width"] - sprite["originX"] if flip_x else sprite["originX"])
                    encoded.append((metadata, 256 + frame_index, stride, data))
            if profile == "game" and sprite_id in (127, 139, 550, 568, 569,
                    0, 1, 3, 12, 15, 27, 31, 37,
                    153, 163, 164, 166, 168, 170, 173, 177, 186, 187, 188, 278):
                mirrored = canvas.transpose(Image.Transpose.FLIP_LEFT_RIGHT)
                pixels = [pixel_to_pen(pixel) for pixel in mirrored.get_flattened_data()]
                stride, data = encode_planar(pixels, mirrored.width, mirrored.height)
                metadata = dict(sprite, id=62000 + sprite_id,
                                originX=sprite["width"] - sprite["originX"])
                encoded.append((metadata, frame_index, stride, data))
            if profile == "game" and sprite_id in (27, 43, 44):
                # Style bobs contain large transparent margins. Trim offline
                # while retaining exactly the same world-space anchor.
                bounds = canvas.getbbox()
                require(bounds is not None, "empty style bob")
                cropped = canvas.crop(bounds)
                pens = [pixel_to_pen(pixel) for pixel in cropped.get_flattened_data()]
                cropped_width, cropped_data = encode_planar(pens, cropped.width, cropped.height)
                cropped_sprite = dict(sprite, id={27: 65500, 43: 65501, 44: 65502}[sprite_id],
                                      width=cropped.width, height=cropped.height,
                                      originX=sprite["originX"] - bounds[0],
                                      originY=sprite["originY"] - bounds[1])
                encoded.append((cropped_sprite, frame_index, cropped_width, cropped_data))
            if profile == "game" and sprite_id == 44:
                mirrored = canvas.transpose(Image.Transpose.FLIP_LEFT_RIGHT)
                mirrored_pens = [pixel_to_pen(pixel) for pixel in mirrored.get_flattened_data()]
                mirrored_width, mirrored_data = encode_planar(
                    mirrored_pens, mirrored.width, mirrored.height)
                mirrored_sprite = dict(sprite, id=65401,
                                       originX=sprite["width"] - sprite["originX"])
                encoded.append((mirrored_sprite, frame_index, mirrored_width, mirrored_data))
                bounds = mirrored.getbbox()
                cropped = mirrored.crop(bounds)
                pens = [pixel_to_pen(pixel) for pixel in cropped.get_flattened_data()]
                cropped_width, cropped_data = encode_planar(pens, cropped.width, cropped.height)
                cropped_sprite = dict(mirrored_sprite, id=65503,
                                      width=cropped.width, height=cropped.height,
                                      originX=mirrored_sprite["originX"] - bounds[0],
                                      originY=mirrored_sprite["originY"] - bounds[1])
                encoded.append((cropped_sprite, frame_index, cropped_width, cropped_data))
            if profile == "game" and sprite_id == 654:
                aligned = Image.new(
                    "RGBA", (sprite["width"] + MENU_TITLE_LEFT_PADDING,
                             sprite["height"]), (0, 0, 0, 0))
                aligned.alpha_composite(canvas, (MENU_TITLE_LEFT_PADDING, 0))
                aligned_pens = [pixel_to_pen(pixel)
                                for pixel in aligned.get_flattened_data()]
                aligned_dma_width, aligned_planar = encode_planar(
                    aligned_pens, aligned.width, aligned.height)
                aligned_sprite = {
                    "id": MENU_TITLE_ALIGNED_ID,
                    "originX": sprite["originX"] + MENU_TITLE_LEFT_PADDING,
                    "originY": sprite["originY"],
                    "width": aligned.width,
                    "height": aligned.height,
                }
                require(decode_planar(
                    aligned_planar, aligned.width, aligned.height,
                    aligned_dma_width) == aligned_pens,
                    f"aligned title round trip failed for frame {frame_index}")
                encoded.append((aligned_sprite, frame_index,
                                aligned_dma_width, aligned_planar))

    if profile == "hud":
        digits = {frame: (sprite, decode_planar(data, sprite["width"], sprite["height"], stride))
                  for sprite, frame, stride, data in encoded if sprite["id"] == 413}
        offsets = ((-1, 0), (0, -1), (1, 0), (0, 1), (-1, 1), (0, 0),
                   (1, 1), (0, 2), (0, 0), (1, -1), (2, 0), (1, 1), (0, 1), (1, 0))
        for digit in range(10):
            pens = [0] * (32 * 24)
            layers = [(digit + (20 if i < 12 else 10), x, y)
                      for i, (x, y) in enumerate(offsets)] + [(digit, 0, 0)]
            for frame, dx, dy in layers:
                source, pixels = digits[frame]
                for y in range(source["height"]):
                    for x in range(source["width"]):
                        pen = pixels[y * source["width"] + x]
                        px, py = 8 + dx + x - source["originX"], 8 + dy + y - source["originY"]
                        if pen:
                            require(0 <= px < 32 and 0 <= py < 24, "ammo digit bounds")
                            pens[py * 32 + px] = pen
            stride, data = encode_planar(pens, 32, 24)
            encoded.append((dict(id=65010, width=32, height=24, originX=8, originY=8),
                            digit, stride, data))

    style_text = {}
    for text_id, text in TEXT_SELECTIONS.get(profile, {}).items():
        canvas = render_text(manifest, pages, text)
        if text_id in (65000, 65001, 65003):
            # Same two draws as Game::renderStyleSelect: red at y+1,
            # followed by white at y. Keep the original anchor unchanged.
            foreground = canvas
            shadow = Image.new("RGBA", foreground.size, (255, 0, 0, 255))
            shadow.putalpha(foreground.getchannel("A"))
            canvas = Image.new("RGBA", (foreground.width, foreground.height + 1))
            canvas.alpha_composite(shadow, (0, 1))
            canvas.alpha_composite(foreground, (0, 0))
        style_text[text_id] = canvas
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

    if profile == "game":
        # Static style text panels, relative to centerY-80. They also serve
        # as clean backing pixels when restoring the moving carousel band.
        for locked in (0, 1):
            panel = Image.new("RGBA", (160, 128), (0, 0, 0, 0))
            for ident, y in ((65000, 10), (65003 if locked else 65001, 90)):
                text = style_text[ident]
                panel.alpha_composite(text, (80 - text.width // 2, y))
            if not locked:
                text = style_text[65002]
                white = Image.new("RGBA", text.size, (255, 255, 255, 255))
                white.putalpha(text.getchannel("A"))
                panel.alpha_composite(white, (80 - text.width // 2, 112))
            pens = [pixel_to_pen(pixel) for pixel in panel.get_flattened_data()]
            dma_width, planar = encode_planar(pens, 160, 128)
            encoded.append((dict(id=65410 + locked, originX=0, originY=0,
                                 width=160, height=128), 0, dma_width, planar))

    for text_id, (text, color) in CREDIT_TEXT_SELECTIONS.get(profile, {}).items():
        canvas = render_bordered_text(manifest, pages, text, color)
        origin_x = canvas.width // 2
        left = CREDIT_TEXT_ANCHOR_X[text_id] - origin_x
        left_padding = left & 15
        if left_padding:
            aligned = Image.new(
                "RGBA", (canvas.width + left_padding, canvas.height),
                (0, 0, 0, 0))
            aligned.alpha_composite(canvas, (left_padding, 0))
            canvas = aligned
            origin_x += left_padding
        pens = [pixel_to_pen(pixel) for pixel in canvas.get_flattened_data()]
        dma_width, planar = encode_planar(pens, canvas.width, canvas.height)
        sprite = {
            "id": text_id,
            "originX": origin_x,
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

    # Gameplay DMA words, including mirrored multi-channel actors, are host work.
    if profile == "game":
        hardware = []
        actors = (0, 1, 3, 12, 15, 27, 31, 37, 133, 134, 153, 163, 164,
                  166, 168, 170, 173, 177, 186, 187, 188, 278, 459)
        for sprite, frame_index, dma_width, planar in encoded:
            if sprite["id"] not in actors:
                continue
            width, height = sprite["width"], sprite["height"]
            require(width <= 64 and height <= 64 and
                    0 <= sprite["originX"] <= width and 0 <= sprite["originY"] <= height,
                    "gameplay actor exceeds conservative visibility bounds")
            stride = dma_width // 8
            def pixel(x, y, plane):
                return 0 <= x < width and bool(
                    planar[plane * stride * height + y * stride + x // 8]
                    & (0x80 >> (x & 7)))
            opaque = [x for x in range(width) if any(
                pixel(x, y, 0) or pixel(x, y, 1) for y in range(height))]
            left, right = (min(opaque), max(opaque)) if opaque else (0, 0)
            parts = (right - left + 16) // 16
            require(parts <= 4 and height <= 32, "gameplay sprite exceeds DMA slot")
            for flip in (0, 1):
                lo, hi = (width - 1 - right, width - 1 - left) if flip else (left, right)
                for part in range(parts):
                    crop = (max(0, min(width - 16, lo - (16 - (hi - lo + 1)) // 2))
                            if parts == 1 else lo + part * 16)
                    stream = bytearray()
                    for y in range(height):
                        for plane in (0, 1):
                            bits = sum(int(pixel(width - 1 - crop - x if flip else crop + x,
                                                 y, plane)) << (15 - x) for x in range(16))
                            stream.extend(struct.pack(">H", bits))
                    metadata = dict(sprite, id=63000 + sprite["id"], width=16,
                                    originX=(width - sprite["originX"] if flip
                                             else sprite["originX"]) - crop,
                                    hardware=True, hardwareParts=parts)
                    hardware.append((metadata, frame_index * 8 + flip * 4 + part,
                                     16, bytes(stream)))
        encoded.extend(hardware)

    # Garden hardware sprites: bounds, centering and mirroring are host work.
    # Two DMA copies let the target change only the inactive position header.
    if profile == "game":
        hardware = []
        for sprite, frame_index, dma_width, planar in encoded:
            if sprite["id"] not in (12, 27, 31, 37):
                continue
            width, height = sprite["width"], sprite["height"]
            stride = dma_width // 8
            plane_size = stride * height
            def pixel(x, y, plane):
                return bool(planar[plane * plane_size + y * stride + x // 8]
                            & (0x80 >> (x & 7)))
            opaque = [x for x in range(width)
                      if any(pixel(x, y, 0) or pixel(x, y, 1)
                             for y in range(height))]
            left, right = (min(opaque), max(opaque)) if opaque else (0, 0)
            require(right - left < 16, "garden actor exceeds one sprite")
            for flip in (0, 1):
                lo, hi = (width - 1 - right, width - 1 - left) if flip else (left, right)
                crop = max(0, min(width - 16, lo - (16 - (hi - lo + 1)) // 2))
                stream = bytearray(4)
                for y in range(height):
                    for plane in (0, 1):
                        bits = 0
                        for x in range(16):
                            sx = width - 1 - (crop + x) if flip else crop + x
                            bits |= int(pixel(sx, y, plane)) << (15 - x)
                        stream.extend(struct.pack(">H", bits))
                stream.extend(bytes(4))
                # Fixed-size slots also keep capture readers within one copy.
                stream.extend(bytes(136 - len(stream)))
                metadata = dict(sprite, id=64000 + sprite["id"], width=16,
                                originX=(width - sprite["originX"] if flip
                                         else sprite["originX"]) - crop,
                                hardware=True)
                hardware.append((metadata, frame_index * 2 + flip, 16,
                                 bytes(stream) * 2))
        encoded.extend(hardware)

    # Keep the on-disk directory ordered so the 68000 renderer can locate a
    # frame with a short binary search instead of repeatedly walking records
    # in Chip RAM.
    encoded.sort(key=lambda item: (item[0]["id"], item[1]))

    payload_offset = HEADER.size + RECORD.size * len(encoded)
    records = []
    payload = bytearray()
    offsets = {}
    immutable_payloads = {}
    # Gameplay DMA words are copied into channel slots, not read by DMA here.
    # Keep them at the end so the target can leave this tail in Fast RAM.
    for sprite, frame_index, dma_width, planar in sorted(
            encoded, key=lambda item: bool(item[0].get("hardwareParts"))):
        # Planar art/masks are immutable. Identical frames and symmetric
        # variants can share bytes without changing any resource/frame ID.
        # Garden DMA records contain writable position headers: never alias
        # those, or one animation buffer could modify the displayed sprite.
        offset = None if sprite.get("hardware") else immutable_payloads.get(planar)
        if offset is None:
            offset = payload_offset + len(payload)
            payload.extend(planar)
            if not sprite.get("hardware"):
                immutable_payloads[planar] = offset
        offsets[(sprite["id"], frame_index)] = offset
    for sprite, frame_index, dma_width, planar in encoded:
        records.append(RECORD.pack(
            sprite["id"], frame_index, sprite["originX"], sprite["originY"],
            sprite["width"], sprite["height"], dma_width,
            (4 | (sprite.get("hardwareParts", 0) << 8)) if sprite.get("hardware") else 3,
            offsets[(sprite["id"], frame_index)], len(planar)))

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
