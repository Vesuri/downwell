#!/usr/bin/env python3
"""Decode a captured A500 Downwell vertical well-entry frame."""

import argparse
import struct
from pathlib import Path

from PIL import Image


DMA_WIDTH = 176
DISPLAY_WIDTH = 160
DISPLAY_HEIGHT = 284
BYTES_PER_ROW = DMA_WIDTH // 8
PLANE_BYTES = BYTES_PER_ROW * DISPLAY_HEIGHT
SPRITE_BYTES = 136
DISPLAY_HARDWARE_X = 0x61
DISPLAY_HARDWARE_Y = 0x12
PLAYFIELD_X = 110
PALETTE = ((0, 0, 0), (255, 255, 255), (255, 0, 0), (0, 0, 0))


def draw_sprite(pixels, sprite):
    start_y = sprite[0] | ((sprite[3] & 4) << 6)
    start_x = (sprite[1] << 1) | (sprite[3] & 1)
    stop_y = sprite[2] | ((sprite[3] & 2) << 7)
    logical_x = start_x - DISPLAY_HARDWARE_X - PLAYFIELD_X
    logical_y = start_y - DISPLAY_HARDWARE_Y
    for y in range(max(0, stop_y - start_y)):
        plane0, plane1 = struct.unpack_from(">HH", sprite, 4 + y * 4)
        for x in range(16):
            mask = 0x8000 >> x
            pen = (1 if plane0 & mask else 0) | (2 if plane1 & mask else 0)
            destination_x = logical_x + x
            destination_y = logical_y + y
            if pen and 0 <= destination_x < DISPLAY_WIDTH and \
                    0 <= destination_y < DISPLAY_HEIGHT:
                pixels[destination_y * DISPLAY_WIDTH + destination_x] = pen


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    payload = args.input.read_bytes()
    if len(payload) < 2:
        raise SystemExit("truncated menu-entry capture")
    title_parts = struct.unpack_from(">H", payload)[0]
    expected_bytes = 2 + PLANE_BYTES + title_parts * SPRITE_BYTES
    if len(payload) != expected_bytes:
        raise SystemExit(
            f"expected {expected_bytes} bytes for {title_parts} title "
            f"sprites, got {len(payload)}")

    plane = payload[2:2 + PLANE_BYTES]
    pixels = [0] * (DISPLAY_WIDTH * DISPLAY_HEIGHT)
    for y in range(DISPLAY_HEIGHT):
        row = y * BYTES_PER_ROW
        for x in range(DISPLAY_WIDTH):
            pixels[y * DISPLAY_WIDTH + x] = \
                (plane[row + x // 8] >> (7 - x % 8)) & 1
    sprite_data = payload[2 + PLANE_BYTES:]
    for part in range(title_parts):
        start = part * SPRITE_BYTES
        draw_sprite(pixels, sprite_data[start:start + SPRITE_BYTES])

    image = Image.new("P", (DISPLAY_WIDTH, DISPLAY_HEIGHT))
    image.putpalette([component for color in PALETTE for component in color] +
                     [0] * (768 - len(PALETTE) * 3))
    image.putdata(pixels)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    image.save(args.output)


if __name__ == "__main__":
    main()
