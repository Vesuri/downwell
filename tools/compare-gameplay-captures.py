#!/usr/bin/env python3
"""Compare actual SDL output to the decoded 352x284 native DMA window."""
import argparse
from pathlib import Path
from PIL import Image


def compare(host, native):
    host, native = host.convert("RGB"), native.convert("RGB")
    scale = host.width // 380
    if scale < 1 or host.size != (380 * scale, 284 * scale):
        raise ValueError("SDL capture must be an integer-scaled 380x284 framebuffer")
    if native.size != (352, 284):
        raise ValueError("native capture must be the 352x284 DMA window")
    different = 0
    for y in range(284):
        for x in range(380):
            pixel = host.getpixel((x * scale, y * scale))
            # Never silently downsample filtering/subpixel differences away.
            for dy in range(scale):
                for dx in range(scale):
                    if host.getpixel((x * scale + dx, y * scale + dy)) != pixel:
                        raise ValueError("SDL capture contains nonuniform scaled pixels")
            # Native storage is 384 wide; its fetch shows logical x=16..367.
            if 16 <= x < 368 and native.getpixel((x - 16, y)) != pixel:
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
    print(f"SDL/native landing raster: {differences} differing pixels of {352 * 284}")
    if differences:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
