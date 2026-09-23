#!/usr/bin/env python3
"""Decode the first grounded A500 gameplay frame from Chip RAM."""

import argparse
import struct
from pathlib import Path

from PIL import Image


DMA_WIDTH = 384
DISPLAY_X = 16
DISPLAY_WIDTH = 352
DISPLAY_HEIGHT = 284
RING_HEIGHT = 320
ROW_BYTES = DMA_WIDTH // 8
RING_PLANE_BYTES = ROW_BYTES * RING_HEIGHT
HUD_PLANE_BYTES = ROW_BYTES * DISPLAY_HEIGHT
SPRITE_BYTES = 136
ACTOR_SPRITES = 6
DISPLAY_HARDWARE_X = 0x61
DISPLAY_HARDWARE_Y = 0x12
PALETTE = ((0, 0, 0), (255, 255, 255), (255, 0, 0), (0, 0, 0))


def plane_pixel(plane, x, y):
    offset = y * ROW_BYTES + x // 8
    return (plane[offset] >> (7 - x % 8)) & 1


def draw_sprite(pixels, sprite):
    start_y = sprite[0] | ((sprite[3] & 4) << 6)
    start_x = (sprite[1] << 1) | (sprite[3] & 1)
    stop_y = sprite[2] | ((sprite[3] & 2) << 7)
    logical_x = start_x - DISPLAY_HARDWARE_X - DISPLAY_X
    logical_y = start_y - DISPLAY_HARDWARE_Y
    for y in range(max(0, stop_y - start_y)):
        plane0, plane1 = struct.unpack_from(">HH", sprite, 4 + y * 4)
        for x in range(16):
            mask = 0x8000 >> x
            pen = (1 if plane0 & mask else 0) | \
                (2 if plane1 & mask else 0)
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
    expected_bytes = 2 + RING_PLANE_BYTES * 2 + HUD_PLANE_BYTES * 2 + \
        SPRITE_BYTES * (1 + ACTOR_SPRITES + 1)
    if len(payload) != expected_bytes:
        raise SystemExit(f"expected {expected_bytes} bytes, got {len(payload)}")

    ring_origin = struct.unpack_from(">H", payload)[0]
    if ring_origin >= RING_HEIGHT:
        raise SystemExit(f"invalid ring origin {ring_origin}")
    offset = 2
    game0 = payload[offset:offset + RING_PLANE_BYTES]
    offset += RING_PLANE_BYTES
    game1 = payload[offset:offset + RING_PLANE_BYTES]
    offset += RING_PLANE_BYTES
    hud0 = payload[offset:offset + HUD_PLANE_BYTES]
    offset += HUD_PLANE_BYTES
    hud1 = payload[offset:offset + HUD_PLANE_BYTES]
    offset += HUD_PLANE_BYTES

    pixels = [0] * (DISPLAY_WIDTH * DISPLAY_HEIGHT)
    for y in range(DISPLAY_HEIGHT):
        game_y = (ring_origin + y) % RING_HEIGHT
        for x in range(DISPLAY_WIDTH):
            dma_x = DISPLAY_X + x
            game_pen = plane_pixel(game0, dma_x, game_y) | \
                (plane_pixel(game1, dma_x, game_y) << 1)
            hud_pen = plane_pixel(hud0, dma_x, y) | \
                (plane_pixel(hud1, dma_x, y) << 1)
            pixels[y * DISPLAY_WIDTH + x] = hud_pen or game_pen

    for sprite_index in range(1 + ACTOR_SPRITES + 1):
        start = offset + sprite_index * SPRITE_BYTES
        draw_sprite(pixels, payload[start:start + SPRITE_BYTES])

    image = Image.new("P", (DISPLAY_WIDTH, DISPLAY_HEIGHT))
    image.putpalette([component for color in PALETTE for component in color] +
                     [0] * (768 - len(PALETTE) * 3))
    image.putdata(pixels)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    image.save(args.output)


if __name__ == "__main__":
    main()
