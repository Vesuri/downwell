#!/usr/bin/env python3
"""Scene-owned original masks for the admitted dither/wind collision pairs."""
import argparse
import hashlib
import json
from pathlib import Path
import runpy
import struct

ROOT = Path(__file__).resolve().parents[1]
COMMON = runpy.run_path(str(ROOT / "tools/generate-sprite-pack.py"))
# Object defaults are pinned below: live changes outside this admission must
# supply a reviewed dependency, never silently load the complete catalogue.
OBJECTS = {35: (31, 31), 60: (51, 585), 61: (128, -1),
           255: (616, -1), 371: (616, -1)}
MASKS = (31, 128, 585, 616)
BYTE_LIMIT = 34256  # 24 header + 4*20 descriptors + 72+32000+32+2048 masks.


def select_pack(source):
    if len(source) < 24 or source[:8] != struct.pack("<4sHH", b"DWSM", 1, 24):
        raise ValueError("invalid source mask header")
    if source[8:16] != bytes.fromhex(COMMON["SOURCE_SHA256"])[:8]:
        raise ValueError("wrong Steam source")
    count, flags = struct.unpack_from("<II", source, 16)
    if flags != 3 or count > (len(source)-24)//20:
        raise ValueError("invalid mask descriptor count")
    offset = 24+20*count
    previous = (-1, -1)
    selected = []
    for i in range(count):
        sprite, frame, width, height, start = struct.unpack_from("<IIIII", source, 24+i*20)
        size = ((width+7)//8)*height
        if (sprite, frame) <= previous or not width or not height or start != offset or size > len(source)-offset:
            raise ValueError("invalid mask descriptor/payload")
        if sprite in MASKS:
            selected.append((sprite, frame, width, height, source[offset:offset+size]))
        offset += size
        previous = (sprite, frame)
    if offset != len(source) or [(s, f) for s, f, _, _, _ in selected] != [(s, 0) for s in MASKS]:
        raise ValueError("unexpected admitted mask coverage")
    header = source[:16]+struct.pack("<II", len(selected), flags)
    offset = 24+20*len(selected)
    descriptors, payloads = [], []
    for sprite, frame, width, height, raw in selected:
        descriptors.append(struct.pack("<IIIII", sprite, frame, width, height, offset))
        payloads.append(raw)
        offset += len(raw)
    result = header+b"".join(descriptors+payloads)
    if len(result) != BYTE_LIMIT:
        raise ValueError("admitted mask size changed; review scene budget")
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    path = ROOT / "generated/downwell-sprite-masks.dwsm"
    source = path.read_bytes()
    manifest = json.loads(path.with_suffix(".manifest.json").read_text())
    metadata = json.loads((ROOT / "extracted/utmt-0.9.2.0/manifest.json").read_text())
    digest = lambda data: hashlib.sha256(data).hexdigest()
    if digest(source) != manifest["packSha256"] or manifest["source"]["sha256"] != COMMON["SOURCE_SHA256"]:
        raise ValueError("unverified input pack")
    actual = {obj["id"]: (obj["spriteId"], obj["textureMaskId"])
              for obj in metadata["resources"]["objects"] if obj["id"] in OBJECTS}
    if actual != OBJECTS or metadata["source"] != manifest["source"]:
        raise ValueError("object admission changed")
    payload = select_pack(source)
    record = dict(formatVersion=1, source=manifest["source"], extractor=manifest["extractor"],
                  inputSha256=digest(source), inputManifestSha256=digest(path.with_suffix(".manifest.json").read_bytes()),
                  generatorSha256=digest(Path(__file__).read_bytes()), sprites=list(MASKS),
                  objectDefaults=OBJECTS, bytes=len(payload), byteLimit=BYTE_LIMIT, packSha256=digest(payload))
    output = ROOT / "generated/downwell-encounter-masks.dwsm"
    header = ("// Generated scene admission; no copyrighted payload.\n"
              f"#define DOWNWELL_ENCOUNTER_MASK_BYTES {len(payload)}u\n").encode()
    for dest, content in ((output, payload), (output.with_suffix(".manifest.json"),
            (json.dumps(record, indent=2, sort_keys=True)+"\n").encode()),
            (ROOT / "generated/amiga-encounter-masks.h", header)):
        if args.check:
            if not dest.is_file() or dest.read_bytes() != content:
                raise ValueError(f"stale scene masks: {dest}")
        else:
            COMMON["atomic_write"](dest, content)
    print(f"Encounter masks: {len(MASKS)} masks, {len(payload)} bytes (Fast RAM scene lease)")


if __name__ == "__main__":
    main()
