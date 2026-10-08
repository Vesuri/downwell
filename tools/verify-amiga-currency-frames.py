#!/usr/bin/env python3
"""Check native published pixels against source-backed effect witnesses.

Generate witnesses with amiga-currency-promotion-test --overflow --pixel-witness
and native dumps with profile.sh frames at the same post-Step ticks. This checks
opaque controller pixels, not the rest of the game's behavior or transparent
shade holes. --reference additionally compares every visible RGB pixel with a
second native capture (sprite assignment and planar storage may differ).
"""
import argparse
import importlib.util
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("frames", type=Path)
    parser.add_argument("witness", type=Path)
    parser.add_argument("--reference", type=Path)
    args = parser.parse_args()
    spec = importlib.util.spec_from_file_location(
        "frame_decoder", Path(__file__).with_name("decode-amiga-gameplay-frame.py"))
    decoder = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(decoder)
    frames, checked, failures = {}, {}, []
    seen = set()
    for line in args.witness.read_text().splitlines():
        if not line.startswith("PIXELS "):
            continue
        _, tick, x, y, alpha, low, high = line.split()
        tick, x, y = int(tick), int(x), int(y)
        alpha, low, high = int(alpha, 16), int(low, 16), int(high, 16)
        if not (0 <= x < 160 and x % 16 == 0 and 0 <= y < 284 and
                0 < alpha <= 65535 and 0 <= low <= 65535 and 0 <= high <= 65535):
            raise ValueError("invalid witness word: " + line)
        if (tick, x, y) in seen:
            raise ValueError("duplicate witness word: " + line)
        seen.add((tick, x, y))
        if tick not in frames:
            frames[tick] = decoder.decode(
                (args.frames / f"frame-{tick}.bin").read_bytes()).convert("RGB")
            checked[tick] = 0
        for bit_index in range(16):
            mask = 32768 >> bit_index
            if not alpha & mask:
                continue
            pen = int(bool(low & mask)) + 2 * int(bool(high & mask))
            position = (x + bit_index + 110 - decoder.DISPLAY_X, y)
            checked[tick] += 1
            if frames[tick].getpixel(position) != decoder.PALETTE[pen]:
                failures.append((tick, position, "source effect"))
    if not frames:
        raise ValueError("no source pixel witnesses")
    if args.reference:
        for tick, actual in frames.items():
            reference = decoder.decode(
                (args.reference / f"frame-{tick}.bin").read_bytes()).convert("RGB")
            for y in range(actual.height):
                for x in range(actual.width):
                    if actual.getpixel((x, y)) != reference.getpixel((x, y)):
                        failures.append((tick, (x, y), "native reference"))
    if failures:
        for item in failures[:20]:
            print("Mismatch:", *item)
        raise SystemExit(f"{len(failures)} pixel comparison failures")
    print(f"Currency pixels: {sum(checked.values())} source-backed opaque pixels "
          f"in {len(frames)} native frames agree")
    if args.reference:
        print("All visible RGB pixels also match the native reference")


if __name__ == "__main__":
    main()
