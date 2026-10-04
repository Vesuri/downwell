#!/usr/bin/env python3
"""Compare actual SDL output to the decoded 336x284 native DMA window.

The native window shows logical x=16..351. Its gem counter is drawn 16 pixels
left of the original's top-right corner, so that rectangle is compared with
the SDL counter at its original position.
"""
import argparse
from pathlib import Path
from PIL import Image

WIDTH = 336
CURRENCY_SHIFT = 16
# Counter fill plus the underline that extends left of it (original x=310).
CURRENCY_LEFT, CURRENCY_TOP = 304 - CURRENCY_SHIFT, 4
CURRENCY_RIGHT, CURRENCY_BOTTOM = 380 - CURRENCY_SHIFT, 34


def compare(host, native):
    host, native = host.convert("RGB"), native.convert("RGB")
    scale = host.width // 380
    if scale < 1 or host.size != (380 * scale, 284 * scale):
        raise ValueError("SDL capture must be an integer-scaled 380x284 framebuffer")
    if native.size != (WIDTH, 284):
        raise ValueError(f"native capture must be the {WIDTH}x284 DMA window")
    different = 0
    for y in range(284):
        for x in range(380):
            pixel = host.getpixel((x * scale, y * scale))
            # Never silently downsample filtering/subpixel differences away.
            for dy in range(scale):
                for dx in range(scale):
                    if host.getpixel((x * scale + dx, y * scale + dy)) != pixel:
                        raise ValueError("SDL capture contains nonuniform scaled pixels")
    for y in range(284):
        for nx in range(WIDTH):
            x = nx + 16
            if CURRENCY_LEFT <= x < CURRENCY_RIGHT and CURRENCY_TOP <= y < CURRENCY_BOTTOM:
                x += CURRENCY_SHIFT
            if native.getpixel((nx, y)) != host.getpixel((x * scale, y * scale)):
                different += 1
    return different


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("host_directory", type=Path)
    parser.add_argument("native_png", type=Path)
    args = parser.parse_args()
    files = list(args.host_directory.glob("*.png"))
    if len(files) != 1:
        raise SystemExit("expected exactly one SDL PNG in the capture directory")
    try:
        with Image.open(files[0]) as host, Image.open(args.native_png) as native:
            differences = compare(host, native)
    except ValueError as error:
        raise SystemExit(str(error))
    print(f"SDL/native gameplay raster: {differences} differing pixels of {WIDTH * 284}")
    if differences:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
