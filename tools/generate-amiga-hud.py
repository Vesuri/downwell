#!/usr/bin/env python3
"""Convert selected recovered art to planar Amiga 2bpp data plus a mask."""

import argparse
import configparser
import hashlib
import json
import math
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
    "clear": {
        # Result Draw uses the current and incoming area border columns.
        **{ident: [7, 13] for ident in (51, 74, 72, 70, 55)},
        # scrUpgrades' resource IDs, frame 0 as drawn by Result Draw.
        **{ident: [0] for ident in (524, 528, 512, 520, 518, 521, 522,
            506, 507, 508, 509, 510, 511, 516, 523, 525, 529, 530,
            531, 532, 533, 534, 535, 152)},
    },
    # Bank separately: gameplay's resident Chip allocation has little headroom.
    # Full-size menu icons are frame 0; the cased world objects use frame 1.
    "shop": {
        143: [0], 147: [0], 149: [0], 150: [0], 151: [0], 152: [0],
        484: None,  # animated shop cursor, plus offline 270-degree variants
        489: [0],  # description panel
        498: [1, 2, 3],  # case-number cursor overlay
    },
    # Separate scene pack: do not make results-only art resident throughout
    # gameplay. All conversion remains host-side, including unlock portraits.
    "results": {
        25: None, 26: None, 28: None, 29: None,
        401: None,  # sprResultStuff
        402: None,  # sprProgress
        403: None,  # sprNumbers
        407: None,  # sprProgPop
        422: None,  # sprPixel
        489: None,  # sprShopDesc
        733: None,  # sprDitherFade
    },
    "hud": {
        378: [2],    # sprHudHpGaugeBar heart-piece dividers
        376: None,   # sprHudCurrency
        408: None,   # sprTabletBorder
        413: None,   # sprSpriteNumber (white/red/black digit banks)
        414: None,   # sprSpriteSlash
        415: None,   # sprPcHudHpGauge
        416: None,   # sprPcHudStammoGauge
        418: None,   # sprPcHudStammoPop
        419: None,   # sprPcHudStammoCharge
        421: None,   # sprPcHudStammoDivide (also first filled row)
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
        84: None,    # sprSmoke: all nine source animation frames
        91: None,    # sprDebris: six frames, default/area-four debris
        96: None,    # sprFlashBall: source shader light/dark classes
        101: None,   # sprFxBulletHitWall: eight source impact frames
        102: None,   # sprFxExplosionS: corpse expiry, twelve planar frames
        115: None,   # sprDebrisRock: four frames, areas one/three/five
        116: None,   # sprDebrisWood: four frames, area two
        117: [0],    # sprLight: opaque-black shade coverage for effect composition
        127: None,   # sprTouchWide (generated wide trigger presentation)
        133: None,   # sprGemSmall
        134: None,   # sprGemLarge
        139: None,   # sprModBtry
        143: [1],    # cased battery
        147: [1],    # cased car battery
        149: [1],    # cased curry
        150: [1],    # cased energy drink
        151: [1],    # cased sushi
        152: [1],    # cased rice ball
        153: None,   # sprBat
        154: None,   # sprBatHit
        157: None,   # sprBatHang
        162: None,   # sprBatHangWall
        163: None,   # sprEnmSnake
        164: None,   # sprSnakeAir
        166: None,   # sprFrogIdle
        168: None,   # sprFrogAir
        170: None,   # sprFrogPrepare
        172: None,   # sprFrogDead: initial corpse artwork, planar only
        173: None,   # sprFrogDamage
        177: None,   # sprEnmSnail
        178: None,   # sprEnmSnailHit
        186: None,   # sprGroundedTurtle
        187: None,   # sprGroundedTurtleFall
        188: None,   # sprGroundedTurtleWalk
        205: None,   # sprEnmSkeletonHit
        206: None,   # sprSkeletonDead (planar; corpse motion is not DMA-owned)
        207: None,   # sprEnmSkeletonThrow
        208: None,   # sprEnmSkeletonIdle
        278: None,   # sprCrawler
        369: None,   # bgNightsky
        458: None,   # sprBulScatter2 (six-frame gem-high projectile)
        459: None,   # sprBulScatter1 (six-frame usual gunboot projectile)
        460: None,   # sprSkeletonBone
        480: [0],    # sprShopCounter
        481: [0],    # sprShopTable
        482: [0],    # sprShopCase, intact fixture (sold cases stay intact)
        485: None,   # merchant idle
        486: None,   # merchant thanks
        487: None,   # merchant angry/blink
        496: None,   # merchant goodbye
        497: None,   # merchant confused
        707: None,   # interaction sign, offline planar and DMA variants
        550: None,   # sprScattered (generated plants)
        568: None,   # sprDungTorch
        569: None,   # sprGemChunk
        602: None,   # sprFxBoingRed (gem-high muzzle)
        603: None,   # sprFxBoingball (usual gunboot muzzle)
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
    # English DeathMenu labels from language.ini. Uncoloured font masks allow
    # the backend to reproduce black outlines and selected red/white layers.
    "results": {
        65300: "GAME OVER",
        65301: "RETRY",
        65302: "STYLE SELECT",
        65303: "SURFACE",
        65304: "MAXED!",
        65305: "STYLE UNLOCKED",
        65306: "PALETTE UNLOCKED",
        **{65320 + digit: str(digit) for digit in range(10)},
        65330: "/",
    },
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
# the fixed HUD layer. IDs 65100..65109 are offline decimal glyphs used by
# native dirty-HUD composition for arbitrary counters. Obsolete whole-number
# strings 10..99 are unnecessary now that counters compose these ten glyphs.
TEXT_SELECTIONS["game"].update({65100 + value: str(value)
                                for value in range(10)})
TEXT_SELECTIONS["game"][65110] = "-"  # Moving damage text uses the same font bank.

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


def flash_palette_canvas(canvas):
    # Resource-96-only mapping, audited against shaderTemplate. This is not
    # arbitrary grayscale quantization: intermediate shades may map to blue.
    # Preserve opaque dark pixels as pen 3, never transparent pen 0.
    mapped = canvas.copy()
    for y in range(canvas.height):
        for x in range(canvas.width):
            pixel = canvas.getpixel((x, y))
            if pixel[3] == 0:
                continue
            require(pixel in ((254,254,254,255), (7,7,7,255)),
                    "unreviewed flash-ball color")
            mapped.putpixel((x,y), (255,255,255,255) if pixel[0] == 254 else (0,0,0,255))
    return mapped


def planar_payload_order(streams):
    """Deterministic offline overlap ordering; no target decompression needed."""
    unique = sorted(set(streams), key=lambda data: (-len(data), data))
    standalone, contained = [], []
    for data in unique:
        for previous in standalone:
            at = previous.find(data)
            while at >= 0 and at % 2:
                at = previous.find(data, at + 1)
            if at >= 0:
                contained.append(data)
                break
        else:
            standalone.append(data)
    payload = bytearray()
    ordered = []
    while standalone:
        best_overlap, best_index = -1, 0
        for index, data in enumerate(standalone):
            overlap = min(len(payload), len(data)) & ~1
            while overlap and payload[-overlap:] != data[:overlap]:
                overlap -= 2
            # Stable ties favor larger streams, then lexical byte order.
            if overlap > best_overlap:
                best_overlap, best_index = overlap, index
        data = standalone.pop(best_index)
        payload.extend(data[best_overlap:])
        ordered.append(data)
    return {data: index for index, data in enumerate(ordered + contained)}


def intern_planar(data, payload, blocks):
    """Share only word-aligned immutable regions, never writable DMA records."""
    if data in blocks:
        return blocks[data]
    for previous, offset in blocks.items():
        at = previous.find(data)
        while at >= 0:
            if at % 2 == 0:
                return offset + at
            at = previous.find(data, at + 1)
    # All calls precede writable DMA payloads. Reuse a matching tail while
    # keeping each asset's complete planar/mask stream contiguous.
    overlap = min(len(payload), len(data)) & ~1
    while overlap and payload[-overlap:] != data[:overlap]:
        overlap -= 2
    offset = len(payload) - overlap
    payload.extend(data[overlap:])
    blocks[data] = offset
    return offset


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


def shop_texts(language_path):
    ini = configparser.ConfigParser(interpolation=None)
    ini.read_string(language_path.read_text(encoding="utf-8-sig"))
    english = ini["english"]
    keys = {65410: "shopExit", 65412: "shopPurchased", 65413: "menuBack"}
    keys.update({65420 + i: f"ugName{100 + i}" for i in range(6)})
    keys.update({65430 + i: f"ugDesc{100 + i}" for i in range(6)})
    # langString replaces '*': GameMaker draw_text interprets '#' as newline.
    result = {ident: english[key].replace("*", "\n").replace("#", "\n")
              for ident, key in keys.items()}
    result[65411] = "BUY"  # literal in scrShopAssignText, not a language key
    # Compose dynamic prices from these original-font masks, never host fonts.
    result.update({65440 + i: str(i) for i in range(10)})
    result[65450] = "G"
    return result


def clear_texts(language_path):
    ini = configparser.ConfigParser(interpolation=None)
    ini.read_string(language_path.read_text(encoding="utf-8-sig"))
    english = ini["english"]
    keys = {64000: "transitionCleared", 64001: "transitionChoose"}
    for upgrade in (*range(23), 100):
        keys[64100 + upgrade] = f"ugName{upgrade}"
        keys[64300 + upgrade] = f"ugDesc{upgrade}"
    # langString uses a single space for absent keys (some cap-zero upgrades).
    texts = {ident: english.get(key, " ").replace("*", "\n").replace("#", "\n")
             for ident, key in keys.items()}
    for area in range(1, 5):
        for level in range(1, 4):
            texts[64500 + (area - 1) * 3 + level - 1] = f"{area}-{level}\n{texts[64000]}"
    # Source upgrade caps are at most one; Result Draw appends owned+1.
    for upgrade in range(23):
        texts[64600 + upgrade] = texts[64100 + upgrade] + " 2"
    return texts


def render_text(manifest, pages, text, centered=False):
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
        draw_x = (width - widths[line_index]) // 2 if centered else 0
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


def render_bordered_text(manifest, pages, text, color, centered=False):
    glyphs = render_text(manifest, pages, text, centered=centered)
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


def verify_projectile_spread_pixels(canvas, origin_x, origin_y):
    """Conservative nearest-pixel inverse-map proof over 267..273 degrees.

    Bound each source coordinate independently using endpoints and every
    stationary point of a*cos(angle)+b*sin(angle). Every pixel in the Cartesian
    product of those ranges must have the quarter-turn's RGBA. This deliberately
    includes impossible coordinate pairs, so passing cannot hide a transition.
    Epsilon widens bounds rather than excluding floating-point boundary ties.
    This checks pixel-centre nearest sampling, not Windows GPU raster rules.
    """
    low, high = math.radians(267), math.radians(273)

    def bounds(a, b, origin):
        candidates = [low, high]
        stationary = math.atan2(b, a)
        for period in range(-3, 4):
            angle = stationary + period * math.pi
            if low <= angle <= high:
                candidates.append(angle)
        values = [origin + a * math.cos(t) + b * math.sin(t) for t in candidates]
        return range(math.floor(min(values) - 1e-10), math.floor(max(values) + 1e-10) + 1)

    def pen(x, y):
        pixel = canvas.getpixel((x, y)) if 0 <= x < canvas.width and 0 <= y < canvas.height else (0, 0, 0, 0)
        return pixel if pixel[3] else (0, 0, 0, 0)

    # Cover the whole rotated canvas, including potential pixels outside its
    # original bounds. Source canvases are square and centrally anchored.
    require(canvas.width == canvas.height and origin_x * 2 == canvas.width and
            origin_y * 2 == canvas.height, "spread proof requires a centred square projectile")
    radius = math.ceil(math.hypot(canvas.width, canvas.height) / 2) + 1
    for y in range(origin_y - radius, origin_y + radius):
        for x in range(origin_x - radius, origin_x + radius):
            dx, dy = x + 0.5 - origin_x, y + 0.5 - origin_y
            expected = pen(math.floor(origin_x + dy), math.floor(origin_y - dx))
            for sx in bounds(dx, -dy, origin_x):
                for sy in bounds(dy, dx, origin_y):
                    require(pen(sx, sy) == expected,
                            "projectile spread changes pixels: add offline angle variants")


def build_pack(manifest, extracted, profile, language_path=None):
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
            if profile == "game" and sprite_id == 96:
                canvas = flash_palette_canvas(canvas)
            if profile == "game" and sprite_id in (84, 96, 101, 153, 154, 157, 162, 163, 164,
                    166, 168, 170, 173, 177, 178, 186, 187, 188, 205, 207, 208, 278,
                    458, 459, 460, 485, 486, 487, 496, 497, 602, 603):
                # Prepare snail Y flips and crawler quarter turns on the host.
                # Crop transparent padding to keep variants Chip-RAM cheap.
                snail = sprite_id in (177, 178)
                gun_spread = sprite_id in (458, 459, 602, 603)
                if gun_spread:
                    require(not sprite["smooth"], "spread proof requires nearest-sampled source artwork")
                    verify_projectile_spread_pixels(canvas, sprite["originX"], sprite["originY"])
                turns = (0, 3) if gun_spread else range(2 if snail else 4 if sprite_id == 278 else 1)
                for turn in turns:
                    if turn == 0:
                        variant = canvas
                    elif snail:
                        variant = canvas.transpose(Image.Transpose.FLIP_TOP_BOTTOM)
                    else:
                        variant = canvas.transpose((Image.Transpose.ROTATE_90,
                            Image.Transpose.ROTATE_180, Image.Transpose.ROTATE_270)[turn - 1])
                    ox, oy = sprite["originX"], sprite["originY"]
                    if snail:
                        if turn: oy = canvas.height - oy
                    else:
                        for _ in range(turn):
                            ox, oy = oy, canvas.width - ox
                    bounds = variant.getbbox()
                    require(bounds is not None, "empty oriented actor frame")
                    if gun_spread or sprite_id in (84, 101):
                        # Projectile art and smoke's final single-pixel frame
                        # can sit beyond their origin. Keep that anchor inside
                        # the DMA frame when cropping, without moving pixels.
                        bounds = (min(bounds[0], ox), min(bounds[1], oy),
                                  max(bounds[2], ox + 1), max(bounds[3], oy + 1))
                    original = variant
                    variant = variant.crop(bounds)
                    for y in range(original.height):
                        for x in range(original.width):
                            restored = variant.getpixel((x - bounds[0], y - bounds[1])) \
                                if bounds[0] <= x < bounds[2] and bounds[1] <= y < bounds[3] \
                                else (0, 0, 0, 0)
                            require(pixel_to_pen(restored) == pixel_to_pen(original.getpixel((x, y))),
                                    "actor crop changes source pixels at their original anchor")
                    ox -= bounds[0]
                    oy -= bounds[1]
                    if variant.width < 16:
                        padded = Image.new("RGBA", (16, variant.height))
                        padded.paste(variant, (0, 0))
                        variant = padded
                    # shopKeeper never changes image_xscale. Keep only the
                    # used planar facing; DMA variants remain independently
                    # generated below and checked against the pixel oracle.
                    # fxSmoke also leaves image_xscale at its default +1.
                    facings = (False,) if gun_spread or sprite_id in (84, 101, 485, 486, 487, 496, 497) else (False, True)
                    for flip_x in facings:
                        image = variant.transpose(Image.Transpose.FLIP_LEFT_RIGHT) if flip_x else variant
                        pixels = [pixel_to_pen(pixel) for pixel in image.get_flattened_data()]
                        stride, data = encode_planar(pixels, image.width, image.height)
                        require(decode_planar(data, image.width, image.height, stride) == pixels,
                                "oriented actor planar round trip failed")
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
            if profile == "shop" and sprite_id == 484:
                # Leave draws at 270 degrees. As with crawler
                # art, encode quarter turns in the high byte of the frame ID.
                rotated = canvas.transpose(Image.Transpose.ROTATE_270)
                pixels = [pixel_to_pen(pixel) for pixel in rotated.get_flattened_data()]
                stride, data = encode_planar(pixels, rotated.width, rotated.height)
                require(decode_planar(data, rotated.width, rotated.height, stride) == pixels,
                        "quarter-turn planar round trip failed")
                metadata = dict(sprite, width=rotated.width, height=rotated.height,
                                originX=canvas.height - sprite["originY"],
                                originY=sprite["originX"])
                encoded.append((metadata, 768 + frame_index, stride, data))
            if profile == "game" and sprite_id in (127, 139, 550, 568, 569,
                    0, 1, 3, 12, 15, 27, 31, 37,
                    153, 154, 163, 164, 166, 168, 170, 172, 173, 177, 186, 187, 188, 206, 278):
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
            occupied = [(x, y) for y in range(24) for x in range(32) if pens[y * 32 + x]]
            left = min(x for x, y in occupied)
            top = min(y for x, y in occupied)
            width = max(x for x, y in occupied) + 1 - left
            height = max(y for x, y in occupied) + 1 - top
            cropped = [pens[y * 32 + x] for y in range(top, top + height)
                       for x in range(left, left + width)]
            stride, data = encode_planar(cropped, width, height)
            require(decode_planar(data, width, height, stride) == cropped,
                    "cropped ammo digit round trip")
            encoded.append((dict(id=65010, width=width, height=height, originX=8-left, originY=8-top),
                            digit, stride, data))

    style_text = {}
    if profile == "game":
        glyphs = {glyph["character"]: glyph
                  for glyph in manifest["resources"]["fonts"][2]["glyphs"]}
        require(all(glyphs[ord(digit)]["shift"] == 8 and not glyphs[ord(digit)]["kernings"]
                    for digit in "-0123456789"),
                "native numeric text requires eight-pixel advances without kerning")
    texts = (shop_texts(language_path) if profile == "shop" else
             clear_texts(language_path) if profile == "clear" else TEXT_SELECTIONS.get(profile, {}))
    for text_id, text in texts.items():
        canvas = render_text(manifest, pages, text, centered=profile in ("shop", "clear"))
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
            "originY": canvas.height // 2 if profile == "shop" else 0,
            "width": canvas.width,
            "height": canvas.height,
        }
        encoded.append((sprite, 0, dma_width, planar))

        if profile == "clear" and text_id not in (64000, 64001) and not 64500 <= text_id < 64512:
            # Preserve frame 0 as the five-pass oracle. Frame 1 is the exact
            # four black neighbours followed by white foreground, host-baked.
            bordered = render_bordered_text(manifest, pages, text, "white", centered=True)
            pens = [pixel_to_pen(pixel) for pixel in bordered.get_flattened_data()]
            dma_width, planar = encode_planar(pens, bordered.width, bordered.height)
            outlined = dict(sprite, width=bordered.width, height=bordered.height,
                            originX=sprite["originX"] + 1, originY=1)
            encoded.append((outlined, 1, dma_width, planar))

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
        actors = (0, 1, 3, 12, 15, 27, 31, 37, 84, 96, 101, 133, 134, 153, 154, 157, 162, 163, 164,
                  166, 168, 170, 173, 177, 178, 186, 187, 188, 205, 207, 208, 278, 458, 459, 460,
                  485, 486, 487, 496, 497, 602, 603, 707)
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
            for flip in ((0,) if sprite["id"] == 84 else (0, 1)):
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
    planar_order = planar_payload_order(
        item[3] for item in encoded if not item[0].get("hardware"))
    # Gameplay DMA words are copied into channel slots, not read by DMA here.
    # Keep them at the end so the target can leave this tail in Fast RAM.
    for sprite, frame_index, dma_width, planar in sorted(
            encoded, key=lambda item: (
                2 if item[0].get("hardwareParts") else 1 if item[0].get("hardware") else 0,
                0 if item[0].get("hardware") else planar_order[item[3]])):
        # Planar art/masks are immutable. Identical frames and symmetric
        # variants can share bytes without changing any resource/frame ID.
        # Garden DMA records contain writable position headers: never alias
        # those, or one animation buffer could modify the displayed sprite.
        if sprite.get("hardware"):
            offset = payload_offset + len(payload)
            payload.extend(planar)
        else:
            offset = payload_offset + intern_planar(planar, payload, immutable_payloads)
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


def build_exit_mask(pack, stride=48, left=110):
    """Gameplay-stride exit cover: solid / original edge / transparent.

    Position the 160-pixel shaft at x=110 in a 384-pixel DMA row offline.
    284+80+284 rows permit every visible wipe position by pointer alone.
    """
    count = HEADER.unpack_from(pack)[3]
    records = [RECORD.unpack_from(pack, HEADER.size + i * RECORD.size)
               for i in range(count)]
    record = next(r for r in records if r[0] == 65005 and r[1] == 0)
    _, _, _, _, width, height, dma_width, _, offset, length = record
    require(width == 160 and height == 80 and dma_width == 160 and length >= 1600,
            "unexpected exit-edge planar layout")
    require(stride > 0 and left >= 0 and left + 160 <= stride * 8,
            "exit mask shaft exceeds its DMA row")
    rows = 648
    output = bytearray(stride * rows)
    for y in range(364):
        for x in range(160):
            covered = y < 284 or bool(pack[offset + (y - 284) * 20 + x // 8]
                                      & (0x80 >> (x & 7)))
            if covered:
                px = left + x
                output[y * stride + px // 8] |= 0x80 >> (px & 7)
    return bytes(output)


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
    parser.add_argument("--language", type=Path, default=root / "data/language.ini")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()

    manifest = json.loads((args.extracted / "manifest.json").read_text(encoding="utf-8"))
    if args.output is None:
        args.output = root / f"generated/amiga-{args.profile}.dwap"
    data = build_pack(manifest, args.extracted, args.profile, args.language)
    digest = hashlib.sha256(data).hexdigest()
    if args.check:
        require(args.output.is_file(), f"missing Amiga asset pack: {args.output}")
        require(args.output.read_bytes() == data,
                "Amiga asset pack is not reproducible")
        print(f"Amiga {args.profile} pack verified: {len(data)} bytes sha256 {digest}")
    else:
        atomic_write(args.output, data)
        print(f"Amiga {args.profile} pack written: {len(data)} bytes sha256 {digest}")
    if args.profile == "game":
        mask = build_exit_mask(data)
        mask_path = args.output.with_suffix(".exit-mask")
        provenance_path = args.output.with_suffix(".exit-mask.json")
        provenance = json.dumps({
            "schemaVersion": 1, "generator": "generate-amiga-hud.py/exit-mask-v1",
            "extractor": manifest["extractor"], "sourceSha256": SOURCE_SHA256,
            "packSha256": digest, "sourceSpriteId": 128, "sourceFrame": 0,
            "derivedStripId": 65005, "width": 384, "height": 648,
            "playfieldX": 110, "playfieldWidth": 160, "bytesPerRow": 48,
            "maskSha256": hashlib.sha256(mask).hexdigest(),
        }, sort_keys=True, indent=2).encode("utf-8") + b"\n"
        if args.check:
            require(mask_path.read_bytes() == mask, "exit mask is not reproducible")
            require(provenance_path.read_bytes() == provenance, "exit mask provenance differs")
        else:
            atomic_write(mask_path, mask)
            atomic_write(provenance_path, provenance)
        # Same recovered cover, in the narrow style/upgrade playfield layout.
        # Keep the gameplay export unchanged; never shift/repack it on 68000.
        selection_mask = build_exit_mask(data, stride=22, left=0)
        selection_path = args.output.with_suffix(".selection-mask")
        selection_provenance = json.dumps({
            "schemaVersion": 1, "generator": "generate-amiga-hud.py/selection-mask-v1",
            "extractor": manifest["extractor"], "sourceSha256": SOURCE_SHA256,
            "packSha256": digest, "sourceSpriteId": 128, "sourceFrame": 0,
            "derivedStripId": 65005, "width": 176, "height": 648,
            "playfieldX": 0, "playfieldWidth": 160, "bytesPerRow": 22,
            "maskSha256": hashlib.sha256(selection_mask).hexdigest(),
        }, sort_keys=True, indent=2).encode("utf-8") + b"\n"
        selection_manifest = args.output.with_suffix(".selection-mask.json")
        if args.check:
            require(selection_path.read_bytes() == selection_mask, "selection mask is not reproducible")
            require(selection_manifest.read_bytes() == selection_provenance, "selection mask provenance differs")
        else:
            atomic_write(selection_path, selection_mask)
            atomic_write(selection_manifest, selection_provenance)
    if args.profile == "hud":
        provenance = json.dumps({
            "schemaVersion": 1, "generator": "generate-amiga-hud.py/hud-v2",
            "extractor": manifest["extractor"], "sourceSha256": SOURCE_SHA256,
            "packSha256": digest, "derivedAmmoDigitId": 65010,
        }, sort_keys=True, indent=2).encode("utf-8") + b"\n"
        provenance_path = args.output.with_suffix(".manifest.json")
        if args.check:
            require(provenance_path.read_bytes() == provenance, "HUD provenance differs")
        else:
            atomic_write(provenance_path, provenance)
    if args.profile in ("shop", "clear"):
        labels = shop_texts(args.language) if args.profile == "shop" else clear_texts(args.language)
        provenance = json.dumps({
            "schemaVersion": 1, "generator": f"generate-amiga-hud.py/{args.profile}-v{2 if args.profile == 'clear' else 1}",
            "extractor": manifest["extractor"], "sourceSha256": SOURCE_SHA256,
            "languageSha256": hashlib.sha256(args.language.read_bytes()).hexdigest(),
            "packSha256": digest, "language": "english",
            "textAssetIds": sorted(labels),
            **({"outlinedTextFrame": 1} if args.profile == "clear" else {}),
        }, sort_keys=True, indent=2).encode("utf-8") + b"\n"
        provenance_path = args.output.with_suffix(".manifest.json")
        if args.check:
            require(provenance_path.read_bytes() == provenance, "shop provenance differs")
        else:
            atomic_write(provenance_path, provenance)
        # Host renderer consumes the same extracted labels without duplicating
        # localized strings in hand-authored source. Keep this ignored too.
        label_function = "shopMenuLabel" if args.profile == "shop" else "levelClearLabel"
        header = (f"// Generated by {args.profile}-v1; provenance: " + provenance_path.name + "\n"
                  "#pragma once\nnamespace downwell {\n"
                  f"inline const char* {label_function}(unsigned id) {{ switch (id) {{\n")
        for ident, label in sorted(labels.items()):
            header += f"case {ident}: return {json.dumps(label, ensure_ascii=True)};\n"
        header += "default: return \"\"; } }\n}\n"
        header = header.encode("utf-8")
        header_path = args.output.with_suffix(".text.h")
        if args.check:
            require(header_path.read_bytes() == header, "shop host labels differ")
        else:
            atomic_write(header_path, header)


if __name__ == "__main__":
    main()
