#!/usr/bin/env python3
"""Generate deterministic native sprite/frame/atlas metadata."""

import argparse
import hashlib
import json
import os
import struct
import tempfile
from pathlib import Path


MAGIC = b"DWSP"
VERSION = 2
MANIFEST_SCHEMA = 3
SOURCE_SHA256 = "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"
EXPECTED_SPRITES = 741
EXPECTED_FRAMES = 3152
EXPECTED_PAGE_ITEMS = 3161

HEADER = struct.Struct("<4sHHIIIIQII")
SPRITE = struct.Struct("<iiiiiIIIiiiiII")
FRAME = struct.Struct("<I")
PAGE_ITEM = struct.Struct("<Iiiiiiiiiiii")

SPRITE_TRANSPARENT = 1 << 0
SPRITE_SMOOTH = 1 << 1
SPRITE_PRELOAD = 1 << 2


def require(condition, message):
    if not condition:
        raise ValueError(message)


def build_pack(manifest):
    require(manifest.get("schemaVersion") == MANIFEST_SCHEMA,
            f"expected manifest schema {MANIFEST_SCHEMA}")
    require(manifest.get("source", {}).get("sha256") == SOURCE_SHA256,
            "manifest source hash does not match the pinned Steam build")

    resources = manifest["resources"]
    sprite_records = []
    frame_records = []
    for sprite_index, sprite in enumerate(resources["sprites"]):
        require(sprite["id"] == sprite_index, "sprite IDs are not contiguous")
        first_frame = len(frame_records)
        for frame_index, frame in enumerate(sprite["frames"]):
            require(frame["id"] == frame_index,
                    f"sprite {sprite_index} frame IDs are not contiguous")
            frame_records.append(FRAME.pack(frame["texturePageItemId"]))
        flags = 0
        if sprite["transparent"]:
            flags |= SPRITE_TRANSPARENT
        if sprite["smooth"]:
            flags |= SPRITE_SMOOTH
        if sprite["preload"]:
            flags |= SPRITE_PRELOAD
        modes = {"AxisAlignedRect": 0, "Precise": 1}
        require(sprite["separateMasks"] in modes, "unsupported collision mode")
        sprite_records.append(SPRITE.pack(
            sprite["id"], sprite["width"], sprite["height"],
            sprite["originX"], sprite["originY"], flags,
            first_frame, len(sprite["frames"]),
            sprite["marginLeft"], sprite["marginTop"], sprite["marginRight"], sprite["marginBottom"],
            modes[sprite["separateMasks"]], sprite["collisionMaskCount"],
        ))

    page_item_records = []
    for item_index, item in enumerate(resources["texturePageItems"]):
        require(item["id"] == item_index,
                "texture-page-item IDs are not contiguous")
        require(0 <= item["texturePageId"] < manifest["counts"]["embeddedTextures"],
                "texture page item references an unknown atlas")
        page_item_records.append(PAGE_ITEM.pack(
            item["id"], item["texturePageId"],
            item["sourceX"], item["sourceY"],
            item["sourceWidth"], item["sourceHeight"],
            item["targetX"], item["targetY"],
            item["targetWidth"], item["targetHeight"],
            item["boundingWidth"], item["boundingHeight"],
        ))

    require(len(sprite_records) == EXPECTED_SPRITES == manifest["counts"]["sprites"],
            "sprite count mismatch")
    require(len(frame_records) == EXPECTED_FRAMES, "sprite frame count mismatch")
    require(len(page_item_records) == EXPECTED_PAGE_ITEMS ==
            manifest["counts"]["texturePageItems"], "page-item count mismatch")

    source_hash_prefix = int.from_bytes(bytes.fromhex(SOURCE_SHA256)[:8], "little")
    header = HEADER.pack(
        MAGIC, VERSION, HEADER.size, MANIFEST_SCHEMA,
        len(sprite_records), len(frame_records), len(page_item_records),
        source_hash_prefix, manifest["counts"]["embeddedTextures"], 0,
    )
    return b"".join((header, b"".join(sprite_records),
                     b"".join(frame_records), b"".join(page_item_records)))


def atomic_write(path, payload):
    path.parent.mkdir(parents=True, exist_ok=True)
    descriptor, temporary_name = tempfile.mkstemp(prefix=path.name + ".", dir=path.parent)
    try:
        with os.fdopen(descriptor, "wb") as output:
            output.write(payload)
        os.replace(temporary_name, path)
        os.chmod(path, 0o644)
    except BaseException:
        os.unlink(temporary_name)
        raise


def main():
    root = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", type=Path,
                        default=root / "extracted/utmt-0.9.2.0/manifest.json")
    parser.add_argument("--output", type=Path,
                        default=root / "generated/downwell-sprites.dwsp")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    payload = build_pack(manifest)
    digest = hashlib.sha256(payload).hexdigest()
    provenance = (json.dumps({
        "formatVersion": VERSION, "source": manifest["source"],
        "extractor": manifest["extractor"],
        "manifestSha256": hashlib.sha256(args.manifest.read_bytes()).hexdigest(),
        "generatorSha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "packSha256": digest,
    }, sort_keys=True, indent=2) + "\n").encode()
    provenance_path = args.output.with_suffix(".manifest.json")
    if args.check:
        require(args.output.is_file(), f"missing sprite pack: {args.output}")
        require(args.output.read_bytes() == payload,
                "sprite pack differs from deterministic generator output")
        require(provenance_path.is_file() and provenance_path.read_bytes() == provenance,
                "sprite provenance differs from deterministic generator output")
        print(f"sprite pack verified: {len(payload)} bytes sha256 {digest}")
        return
    atomic_write(args.output, payload)
    atomic_write(provenance_path, provenance)
    print(f"sprite pack written: {len(payload)} bytes sha256 {digest}")


if __name__ == "__main__":
    main()
