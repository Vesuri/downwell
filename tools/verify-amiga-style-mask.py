#!/usr/bin/env python3
"""Check style opacity against Steam art, optionally including DMA captures."""
import argparse
import json
import runpy
import struct
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parent.parent


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--capture", action="store_true")
    parser.add_argument("--selected", action="store_true")
    args = parser.parse_args()
    extracted = ROOT / "extracted/utmt-0.9.2.0"
    manifest = json.loads((extracted / "manifest.json").read_text())
    resources = manifest["resources"]
    pages = [Image.open(extracted / "TexturePages" / f"{i}.png").convert("RGBA")
             for i in range(manifest["counts"]["embeddedTextures"])]
    canvas_for = runpy.run_path(str(ROOT / "tools/generate-amiga-hud.py"))["frame_canvas"]
    pack = (ROOT / "generated/amiga-front.dwap").read_bytes()
    start, count = struct.unpack_from(">HI", pack, 6)
    records = {r[:2]: r for i in range(count)
               for r in [struct.unpack_from(">HHhhHHHHII", pack, start + 24 * i)]}
    strips = []
    for ident, original, sy in ((65004, 129, 0), (65005, 128, 720)):
        s = resources["sprites"][original]
        canvas = canvas_for(s, s["frames"][0], resources["texturePageItems"], pages)
        original_pixels = list(canvas.crop((80, sy, 240, sy + 80)).get_flattened_data())
        record = records[ident, 0]
        stride, offset = record[6] // 8, record[8]
        strip = []
        for y in range(80):
            for x in range(160):
                opacity = bool(pack[offset + y * stride + x // 8] & (0x80 >> (x & 7)))
                assert opacity == (original_pixels[y * 160 + x][3] >= 128)
                strip.append(opacity)
        assert any(strip) and not all(strip)
        strips.append(strip)
    print("Both style strips match source opacity (alpha-1 holes remain transparent)")
    if args.selected:
        mask = (ROOT / "amiga/.run/selected-style-mask.bin").read_bytes()
        assert len(mask) == 22 * 284
        for y in range(284):
            for x in range(160):
                expected = strips[1][y * 160 + x] if y < 80 else False
                assert bool(mask[y * 22 + x // 8] & (0x80 >> (x & 7))) == expected
        print("Selected-style exit DMA mask matches source opacity and position")
    if args.capture:
        run = ROOT / "amiga/.run"
        border = (run / "style-border.bin").read_bytes()
        assert len(border) == 2 * 300 * 22
        sprite = resources["sprites"][51]
        tiles = [canvas_for(sprite, sprite["frames"][frame],
                            resources["texturePageItems"], pages) for frame in (13, 7)]
        for y in range(300):
            for x in range(176):
                expected = 0
                if x < 8 or 152 <= x < 160:
                    rgba = tiles[0 if x < 8 else 1].getpixel(
                        (x + 8 if x < 8 else x - 152, (y + 8) % 16))
                    if rgba[3]:
                        expected = {(255, 255, 255): 1, (255, 0, 0): 2, (0, 0, 0): 3}[rgba[:3]]
                bit = 0x80 >> (x & 7)
                index = y * 22 + x // 8
                actual = int(bool(border[index] & bit)) + 2 * int(bool(border[index + 300 * 22] & bit))
                assert actual == expected, ("border", x, y, actual, expected)
        print("Complete native border matches original tiles, clipped anchors and guard words")
        masks = (run / "style-all-masks.bin").read_bytes()
        assert len(masks) == 2 * 648 * 22
        for exit in range(2):
            for y in range(648):
                for x in range(176):
                    expected = (bool(exit) if y < 284 else not exit if y >= 364
                                else x < 160 and strips[exit][(y - 284) * 160 + x])
                    byte = masks[exit * 648 * 22 + y * 22 + x // 8]
                    assert bool(byte & (0x80 >> (x & 7))) == expected
        print("Both complete native masks match source opacity, tails and guard words")
        mask = (run / "style-reveal-mask.bin").read_bytes()
        assert len(mask) == 22 * 284
        for y in range(284):
            for x in range(160):
                expected = False if y < 128 else True if y >= 208 else strips[0][(y - 128) * 160 + x]
                assert bool(mask[y * 22 + x // 8] & (0x80 >> (x & 7))) == expected
        copper = (run / "style-reveal-copper.bin").read_bytes()
        registers = {}
        for reg, value in struct.iter_unpack(">HH", copper):
            if reg == 0xffff: break
            registers[reg] = value
        assert registers[0x100] == 0x5401  # BPL5 is the temporary mask
        assert not registers[0x104] & 0x40  # PF1 in front
        assert all(registers[0x180 + color * 2] == 0 for color in range(4, 8))
        assert registers[0x108] == registers[0x10a] == 2
        print("Amiga DMA mask and dual-playfield Copper configuration match")


if __name__ == "__main__":
    main()
