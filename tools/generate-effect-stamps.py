#!/usr/bin/env python3
"""Prepare immutable three-row (alpha/low/high) effect blit packets offline."""
import argparse
import hashlib
import json
from pathlib import Path
import struct

VERSION = "effect-stamps-v1"
KEYS = {(84, frame) for frame in range(9)} | {
    (sprite, frame) for sprite in (133, 134) for frame in range(4)}


def prepare(pack):
    if len(pack) < 20 or struct.unpack_from(">4sHH", pack) != (b"DWAP", 2, 20):
        raise ValueError("expected DWAP v2")
    count = struct.unpack_from(">I", pack, 8)[0]
    if count > (len(pack) - 20) // 24:
        raise ValueError("truncated DWAP records")
    images = {}
    for i in range(count):
        sprite, frame, ox, oy, width, height, dma, planes, offset, size = struct.unpack_from(">8H2I", pack, 20 + i * 24)
        if (sprite, frame) not in KEYS:
            continue
        stride = dma // 8
        plane = stride * height
        if (planes != 3 or not width or not height or dma % 16 or width > dma or
                offset < 20 + count * 24 or offset % 2 or offset + size > len(pack) or size != plane * 3):
            raise ValueError("invalid planar effect")
        if (sprite, frame) in images:
            raise ValueError("duplicate resource key")
        low, high, mask = (pack[offset + n * plane:offset + (n + 1) * plane] for n in range(3))
        pixels, coverage = bytearray(), bytearray()
        for y in range(height):
            row = slice(y * stride, (y + 1) * stride)
            # Destination rows are alpha, low, high. Alpha is masked replace
            # of its own mask, equivalent to OR, including opaque black.
            pixels += mask[row] + low[row] + high[row]
            coverage += mask[row] * 3
        images[sprite, frame] = width, height, stride, bytes(pixels), bytes(coverage)
    if set(images) != KEYS:
        raise ValueError("missing currency/smoke frame")
    header = struct.pack(">4sHHII32s", b"DWEF", 1, len(images), 20,
                         48 + len(images) * 20, hashlib.sha256(pack).digest())
    records, payload = bytearray(), bytearray()
    for (sprite, frame), (width, height, stride, pixels, mask) in sorted(images.items()):
        offset = len(header) + 20 * len(images) + len(payload)
        records += struct.pack(">HHHHIII", sprite, frame, width, height, stride, offset, offset + len(pixels))
        payload += pixels + mask
    return header + records + payload


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, default=Path("generated/amiga-game.dwap"))
    parser.add_argument("--output", type=Path, default=Path("generated/amiga-effect-stamps.dwef"))
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    source = args.input.read_bytes()
    output = prepare(source)
    provenance_path = args.input.with_suffix(".packing.json")
    provenance_bytes = provenance_path.read_bytes()
    provenance = json.loads(provenance_bytes)
    if provenance["packSha256"] != hashlib.sha256(source).hexdigest():
        raise ValueError("input provenance does not match pack")
    manifest = {
        "schemaVersion": 1, "generator": VERSION,
        "generatorSha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "sourceSha256": provenance["sourceSha256"], "extractor": provenance["extractor"],
        "inputSha256": hashlib.sha256(source).hexdigest(),
        "inputManifestSha256": hashlib.sha256(provenance_bytes).hexdigest(),
        "outputSha256": hashlib.sha256(output).hexdigest(), "bytes": len(output),
        "resources": [list(key) for key in sorted(KEYS)],
    }
    encoded = (json.dumps(manifest, indent=2) + "\n").encode()
    manifest_path = args.output.with_suffix(".json")
    if args.check:
        if args.output.read_bytes() != output or manifest_path.read_bytes() != encoded:
            raise ValueError("prepared effects differ; regenerate")
    else:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_bytes(output)
        manifest_path.write_bytes(encoded)
    print(f"Prepared effect stamps: {len(KEYS)} resources, {len(output)} bytes")


if __name__ == "__main__":
    main()
