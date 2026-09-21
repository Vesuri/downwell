#!/usr/bin/env python3
"""Generate the deterministic native object/event pack from the UTMT manifest."""

import argparse
import hashlib
import json
import os
import struct
import tempfile
from pathlib import Path


MAGIC = b"DWOB"
VERSION = 1
MANIFEST_SCHEMA = 3
SOURCE_SHA256 = "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"
EXPECTED_OBJECTS = 386
EXPECTED_BINDINGS = 2075

HEADER = struct.Struct("<4sHHIIIQIQ")
OBJECT = struct.Struct("<iiiiiIIII")
BINDING = struct.Struct("<iiiI")

OBJECT_VISIBLE = 1 << 0
OBJECT_SOLID = 1 << 1
OBJECT_PERSISTENT = 1 << 2
OBJECT_USES_PHYSICS = 1 << 3


def require(condition, message):
    if not condition:
        raise ValueError(message)


def build_pack(manifest):
    require(manifest.get("schemaVersion") == MANIFEST_SCHEMA,
            f"expected manifest schema {MANIFEST_SCHEMA}")
    require(manifest.get("source", {}).get("sha256") == SOURCE_SHA256,
            "manifest source hash does not match the pinned Steam build")

    object_records = []
    binding_records = []
    objects = manifest["resources"]["objects"]
    for object_index, obj in enumerate(objects):
        require(obj["id"] == object_index,
                "object IDs are not contiguous resource-table indices")
        first_binding = len(binding_records)
        for event in obj["events"]:
            require(len(event["actions"]) == 1,
                    f"object {obj['id']} event does not have exactly one action")
            binding_records.append(BINDING.pack(
                event["eventType"],
                event["eventSubtype"],
                event["actions"][0]["codeId"],
                0,
            ))

        flags = 0
        if obj["Visible"]:
            flags |= OBJECT_VISIBLE
        if obj["Solid"]:
            flags |= OBJECT_SOLID
        if obj["Persistent"]:
            flags |= OBJECT_PERSISTENT
        if obj["UsesPhysics"]:
            flags |= OBJECT_USES_PHYSICS
        object_records.append(OBJECT.pack(
            obj["id"],
            obj["spriteId"],
            obj["parentId"],
            obj["textureMaskId"],
            obj["Depth"],
            flags,
            first_binding,
            len(obj["events"]),
            0,
        ))

    require(len(object_records) == EXPECTED_OBJECTS == manifest["counts"]["objects"],
            "object count mismatch")
    require(len(binding_records) == EXPECTED_BINDINGS,
            "event-binding count mismatch")

    source_hash_prefix = int.from_bytes(bytes.fromhex(SOURCE_SHA256)[:8], "little")
    header = HEADER.pack(
        MAGIC,
        VERSION,
        HEADER.size,
        MANIFEST_SCHEMA,
        len(object_records),
        len(binding_records),
        source_hash_prefix,
        0,
        0,
    )
    return b"".join((header, b"".join(object_records), b"".join(binding_records)))


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
                        default=root / "generated/downwell-objects.dwob")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()

    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    payload = build_pack(manifest)
    digest = hashlib.sha256(payload).hexdigest()

    if args.check:
        require(args.output.is_file(), f"missing object pack: {args.output}")
        require(args.output.read_bytes() == payload,
                "object pack differs from deterministic generator output")
        print(f"object pack verified: {len(payload)} bytes sha256 {digest}")
        return

    atomic_write(args.output, payload)
    print(f"object pack written: {len(payload)} bytes sha256 {digest}")


if __name__ == "__main__":
    main()
