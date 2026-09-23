#!/usr/bin/env python3
import argparse
from pathlib import Path

from PIL import Image


WIDTH = 176
VISIBLE_WIDTH = 160
HEIGHT = 284
ROW_BYTES = WIDTH // 8
PLANE_BYTES = ROW_BYTES * HEIGHT
FRAME_BYTES = PLANE_BYTES * 2
PALETTE = ((0, 0, 0), (255, 255, 255), (255, 0, 0), (0, 0, 0))


def main():
    parser = argparse.ArgumentParser(
        description="Decode an A500 Downwell style-select planar frame")
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    data = args.input.read_bytes()
    if len(data) != FRAME_BYTES:
        raise SystemExit(
            f"expected {FRAME_BYTES} bytes, found {len(data)}")

    pixels = []
    for y in range(HEIGHT):
        row = y * ROW_BYTES
        for x in range(VISIBLE_WIDTH):
            mask = 0x80 >> (x & 7)
            offset = row + (x >> 3)
            pen = int(bool(data[offset] & mask))
            pen |= int(bool(data[PLANE_BYTES + offset] & mask)) << 1
            pixels.append(PALETTE[pen])

    image = Image.new("RGB", (VISIBLE_WIDTH, HEIGHT))
    image.putdata(pixels)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    image.save(args.output)


if __name__ == "__main__":
    main()
