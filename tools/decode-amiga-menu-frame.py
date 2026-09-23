#!/usr/bin/env python3
"""Decode a captured A500 hardware-scrolled Downwell garden frame."""

import argparse
import struct
from pathlib import Path

from PIL import Image


CACHE_WIDTH = 512
CACHE_HEIGHT = 384
BYTES_PER_ROW = CACHE_WIDTH // 8
PLANE_BYTES = BYTES_PER_ROW * CACHE_HEIGHT
DISPLAY_WIDTH = 160
DISPLAY_HEIGHT = 284
SPRITE_BYTES = 136
DISPLAY_HARDWARE_X = 0x61
DISPLAY_HARDWARE_Y = 0x12
PLAYFIELD_X = 110
PALETTE = ((0, 0, 0), (255, 255, 255), (255, 0, 0), (0, 0, 0))


def plane_pixel(plane, x, y):
    return (plane[y * BYTES_PER_ROW + x // 8] >> (7 - x % 8)) & 1


def draw_sprite(pixels, sprite):
    start_y = sprite[0] | ((sprite[3] & 4) << 6)
    start_x = (sprite[1] << 1) | (sprite[3] & 1)
    stop_y = sprite[2] | ((sprite[3] & 2) << 7)
    logical_x = start_x - DISPLAY_HARDWARE_X - PLAYFIELD_X
    logical_y = start_y - DISPLAY_HARDWARE_Y
    height = stop_y - start_y
    for y in range(max(0, height)):
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
    foreground_offset, background_offset, scroll, title_parts = \
        struct.unpack_from(">IIHH", payload)
    expected_bytes = 12 + PLANE_BYTES * 2 + \
        SPRITE_BYTES * (1 + title_parts)
    if len(payload) != expected_bytes:
        raise SystemExit(
            f"expected {expected_bytes} capture bytes for {title_parts} "
            f"title sprites, got {len(payload)}")
    foreground = payload[12:12 + PLANE_BYTES]
    background = payload[12 + PLANE_BYTES:12 + PLANE_BYTES * 2]
    sprite_data = payload[12 + PLANE_BYTES * 2:]
    player = sprite_data[:SPRITE_BYTES]

    foreground_y, foreground_byte = divmod(foreground_offset, BYTES_PER_ROW)
    background_y, background_byte = divmod(background_offset, BYTES_PER_ROW)
    foreground_x = foreground_byte * 8 + (scroll & 15)
    background_x = background_byte * 8 + ((scroll >> 4) & 15)
    if foreground_x + DISPLAY_WIDTH > CACHE_WIDTH or \
            background_x + DISPLAY_WIDTH > CACHE_WIDTH or \
            foreground_y + DISPLAY_HEIGHT > CACHE_HEIGHT or \
            background_y + DISPLAY_HEIGHT > CACHE_HEIGHT:
        raise SystemExit("captured scroll state leaves the static garden cache")

    pixels = [0] * (DISPLAY_WIDTH * DISPLAY_HEIGHT)
    for y in range(DISPLAY_HEIGHT):
        for x in range(DISPLAY_WIDTH):
            foreground_pen = plane_pixel(
                foreground, foreground_x + x, foreground_y + y)
            background_pen = plane_pixel(
                background, background_x + x, background_y + y)
            pixels[y * DISPLAY_WIDTH + x] = \
                1 if foreground_pen or background_pen else 0
    draw_sprite(pixels, player)
    for part in range(title_parts):
        start = SPRITE_BYTES * (part + 1)
        draw_sprite(pixels, sprite_data[start:start + SPRITE_BYTES])

    image = Image.new("P", (DISPLAY_WIDTH, DISPLAY_HEIGHT))
    image.putpalette([component for color in PALETTE for component in color] +
                     [0] * (768 - len(PALETTE) * 3))
    image.putdata(pixels)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    image.save(args.output)


if __name__ == "__main__":
    main()
