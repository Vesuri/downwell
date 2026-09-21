#!/usr/bin/env python3
"""Generate deterministic native font/glyph/kerning metadata."""

import argparse
import hashlib
import json
import os
import struct
import tempfile
from pathlib import Path

MAGIC = b"DWFN"
VERSION = 1
MANIFEST_SCHEMA = 3
SOURCE_SHA256 = "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"
EXPECTED_FONTS = 8
EXPECTED_GLYPHS = 1525
EXPECTED_KERNINGS = 323

HEADER = struct.Struct("<4sHHIIIIQII")
FONT = struct.Struct("<if iIIIIII".replace(" ", ""))
GLYPH = struct.Struct("<IiiiiiiII")
KERNING = struct.Struct("<Ii")


def require(condition, message):
    if not condition:
        raise ValueError(message)


def build_pack(manifest):
    require(manifest.get("schemaVersion") == MANIFEST_SCHEMA,
            f"expected manifest schema {MANIFEST_SCHEMA}")
    require(manifest.get("source", {}).get("sha256") == SOURCE_SHA256,
            "manifest source hash does not match the pinned Steam build")

    font_records = []
    glyph_records = []
    kerning_records = []
    fonts = manifest["resources"]["fonts"]
    page_items = manifest["resources"]["texturePageItems"]
    for font_id, font in enumerate(fonts):
        require(font["id"] == font_id, "font IDs are not contiguous")
        texture_item = font["texturePageItemId"]
        require(0 <= texture_item < len(page_items),
                f"font {font_id} references an unknown texture item")
        first_glyph = len(glyph_records)
        previous_character = -1
        for glyph in font["glyphs"]:
            character = glyph["character"]
            require(character > previous_character,
                    f"font {font_id} glyphs are not strictly ordered")
            previous_character = character
            first_kerning = len(kerning_records)
            for kerning in glyph["kernings"]:
                kerning_records.append(KERNING.pack(
                    kerning["character"], kerning["shiftModifier"]))
            glyph_records.append(GLYPH.pack(
                character, glyph["sourceX"], glyph["sourceY"],
                glyph["sourceWidth"], glyph["sourceHeight"],
                glyph["shift"], glyph["offset"],
                first_kerning, len(glyph["kernings"])))
        require(len(font["glyphs"]) == font["glyphCount"],
                f"font {font_id} glyph count mismatch")
        flags = (1 if font["bold"] else 0) | (2 if font["italic"] else 0)
        font_records.append(FONT.pack(
            font_id, float(font["emSize"]), texture_item, flags,
            first_glyph, len(font["glyphs"]),
            font["rangeStart"], font["rangeEnd"], 0))

    require(len(font_records) == EXPECTED_FONTS == manifest["counts"]["fonts"],
            "font count mismatch")
    require(len(glyph_records) == EXPECTED_GLYPHS, "glyph count mismatch")
    require(len(kerning_records) == EXPECTED_KERNINGS, "kerning count mismatch")
    source_hash_prefix = int.from_bytes(bytes.fromhex(SOURCE_SHA256)[:8], "little")
    header = HEADER.pack(MAGIC, VERSION, HEADER.size, MANIFEST_SCHEMA,
                         len(font_records), len(glyph_records),
                         len(kerning_records), source_hash_prefix, 0, 0)
    return b"".join((header, b"".join(font_records),
                     b"".join(glyph_records), b"".join(kerning_records)))


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
                        default=root / "generated/downwell-fonts.dwfn")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    payload = build_pack(json.loads(args.manifest.read_text(encoding="utf-8")))
    digest = hashlib.sha256(payload).hexdigest()
    if args.check:
        require(args.output.is_file(), f"missing font pack: {args.output}")
        require(args.output.read_bytes() == payload,
                "font pack differs from deterministic generator output")
        print(f"font pack verified: {len(payload)} bytes sha256 {digest}")
        return
    atomic_write(args.output, payload)
    print(f"font pack written: {len(payload)} bytes sha256 {digest}")


if __name__ == "__main__":
    main()
