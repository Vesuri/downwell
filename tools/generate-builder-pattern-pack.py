#!/usr/bin/env python3
"""Generate deterministic native builder-pattern data from recovered GML."""

import argparse
import hashlib
import json
import os
import re
import struct
import tempfile
from pathlib import Path


MAGIC = b"DWBP"
VERSION = 3
MANIFEST_SCHEMA = 3
SOURCE_SHA256 = "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"
AREA_ONE_START = 1
AREA_ONE_MAIN = 2
AREA_ONE_RIGHT = 3
AREA_ONE_SHOP = 4
EXPECTED_START_PATTERNS = 5
EXPECTED_MAIN_PATTERNS = 41
EXPECTED_RIGHT_PATTERNS = 10
EXPECTED_SHOP_PATTERNS = 2
EXPECTED_PATTERNS = 58
EXPECTED_ROWS = 691
EXPECTED_CELLS = 8021

HEADER = struct.Struct("<4sHHIIIQI")
PATTERN = struct.Struct("<HHHHIII")
ROW = struct.Struct("<II")


def require(condition, message):
    if not condition:
        raise ValueError(message)


def decode_gml_string(value):
    escapes = {"r": "\r", "n": "\n", "t": "\t", "\\": "\\", '"': '"'}
    output = []
    index = 0
    while index < len(value):
        if value[index] != "\\":
            output.append(value[index])
            index += 1
            continue
        require(index + 1 < len(value), "unterminated GML escape")
        escaped = value[index + 1]
        require(escaped in escapes, f"unsupported GML escape: \\{escaped}")
        output.append(escapes[escaped])
        index += 2
    return "".join(output)


def parse_patterns(source, array_name, expected_count, expected_width,
                   allow_irregular=False):
    assignment = re.compile(
        rf'{re.escape(array_name)}\[i\]\s*=\s*"((?:\\.|[^"\\])*)";')
    values = assignment.findall(source)
    require(len(values) == expected_count,
            f"expected {expected_count} patterns in {array_name}")
    require(re.search(rf"{re.escape(array_name)}\[100\]\s*=\s*i\s*;",
                      source) is not None,
            f"missing recovered maximum-index assignment for {array_name}")

    patterns = []
    for source_index, encoded in enumerate(values):
        decoded = decode_gml_string(encoded)
        rows = decoded.splitlines()
        while rows and rows[0].strip() == "":
            rows.pop(0)
        while rows and rows[-1].strip() == "":
            rows.pop()
        rows = [row[1:] if row.startswith("\t") else row for row in rows]
        require(rows, f"pattern {source_index} is empty")
        width = max(len(row) for row in rows)
        if allow_irregular:
            require(min(len(row) for row in rows) == expected_width,
                    f"pattern {source_index} has a short recovered row")
        else:
            require(width == expected_width,
                    f"pattern {source_index} width is not {expected_width} cells")
            require(all(len(row) == width for row in rows),
                    f"pattern {source_index} has inconsistent row widths")
        require(all(0x20 <= ord(cell) <= 0x7e for row in rows for cell in row),
                f"pattern {source_index} contains a non-printable cell")
        patterns.append((source_index, width, rows))
    return patterns


def build_pack(manifest, start_source, main_source, right_source, shop_source):
    require(manifest.get("schemaVersion") == MANIFEST_SCHEMA,
            f"expected manifest schema {MANIFEST_SCHEMA}")
    require(manifest.get("source", {}).get("sha256") == SOURCE_SHA256,
            "manifest source hash does not match the pinned Steam build")

    records = []
    row_records = []
    cells = bytearray()
    families = (
        (AREA_ONE_START, parse_patterns(
            start_source, "partStart", EXPECTED_START_PATTERNS, 9)),
        (AREA_ONE_MAIN, parse_patterns(
            main_source, "partMain", EXPECTED_MAIN_PATTERNS, 9)),
        (AREA_ONE_RIGHT, parse_patterns(
            right_source, "partRight", EXPECTED_RIGHT_PATTERNS, 19)),
        (AREA_ONE_SHOP, parse_patterns(
            shop_source, "partShopR", EXPECTED_SHOP_PATTERNS, 19, True)),
    )
    row_count = 0
    for family, patterns in families:
        for source_index, width, rows in patterns:
            first_row = len(row_records)
            first_cell = len(cells)
            for row in rows:
                encoded = row.encode("ascii")
                row_records.append(ROW.pack(len(cells), len(encoded)))
                cells.extend(encoded)
                row_count += 1
            records.append(PATTERN.pack(
                family, source_index, width, len(rows),
                first_row, first_cell, len(cells) - first_cell))

    require(len(records) == EXPECTED_PATTERNS, "unexpected recovered pattern count")
    require(row_count == EXPECTED_ROWS, "unexpected recovered row count")
    require(len(cells) == EXPECTED_CELLS, "unexpected recovered cell count")
    source_hash_prefix = int.from_bytes(bytes.fromhex(SOURCE_SHA256)[:8], "little")
    header = HEADER.pack(MAGIC, VERSION, HEADER.size, MANIFEST_SCHEMA,
                         len(records), len(cells), source_hash_prefix,
                         len(row_records))
    return b"".join((header, b"".join(records), b"".join(row_records), cells))


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
    parser.add_argument("--start-source", type=Path, default=root /
                        "extracted/utmt-0.9.2.0/CodeEntries/"
                        "gml_GlobalScript_scrStartPat1.gml")
    parser.add_argument("--main-source", type=Path, default=root /
                        "extracted/utmt-0.9.2.0/CodeEntries/"
                        "gml_GlobalScript_scrLevelPat1.gml")
    parser.add_argument("--right-source", type=Path, default=root /
                        "extracted/utmt-0.9.2.0/CodeEntries/"
                        "gml_GlobalScript_scrRightPat1.gml")
    parser.add_argument("--shop-source", type=Path, default=root /
                        "extracted/utmt-0.9.2.0/CodeEntries/"
                        "gml_GlobalScript_scrShopPat1.gml")
    parser.add_argument("--output", type=Path,
                        default=root / "generated/downwell-builder-patterns.dwbp")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    payload = build_pack(
        json.loads(args.manifest.read_text(encoding="utf-8")),
        args.start_source.read_text(encoding="utf-8"),
        args.main_source.read_text(encoding="utf-8"),
        args.right_source.read_text(encoding="utf-8"),
        args.shop_source.read_text(encoding="utf-8"))
    digest = hashlib.sha256(payload).hexdigest()
    if args.check:
        require(args.output.is_file(), f"missing builder-pattern pack: {args.output}")
        require(args.output.read_bytes() == payload,
                "builder-pattern pack differs from deterministic generator output")
        print(f"builder-pattern pack verified: {len(payload)} bytes sha256 {digest}")
        return
    atomic_write(args.output, payload)
    print(f"builder-pattern pack written: {len(payload)} bytes sha256 {digest}")


if __name__ == "__main__":
    main()
