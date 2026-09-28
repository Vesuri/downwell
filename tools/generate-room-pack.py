#!/usr/bin/env python3
"""Generate the deterministic native room pack from the audited UTMT manifest."""

import argparse
import hashlib
import json
import os
import struct
import tempfile
from pathlib import Path


MAGIC = b"DWRM"
VERSION = 4
MANIFEST_SCHEMA = 3
SOURCE_SHA256 = "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"
EXPECTED_ROOMS = 25
EXPECTED_INSTANCES = 5186
EXPECTED_LAYERS = 132

HEADER = struct.Struct("<4sHHIIIIIQI")
ROOM = struct.Struct("<iiiiiIIIII")
VIEW = struct.Struct("<14i")
VIEW_FIELDS = ("enabled", "viewX", "viewY", "viewWidth", "viewHeight",
               "portX", "portY", "portWidth", "portHeight", "borderX", "borderY",
               "speedX", "speedY", "objectId")
INSTANCE = struct.Struct("<Iiii5fiiI")
LAYER = struct.Struct("<IiIIIIiIIffI")
MEMBERSHIP = struct.Struct("<I")

ROOM_PERSISTENT = 1 << 0
INSTANCE_PERSISTENT = 1 << 0
LAYER_VISIBLE = 1 << 0


def require(condition, message):
    if not condition:
        raise ValueError(message)


def build_pack(manifest, layer_metadata):
    require(manifest.get("schemaVersion") == MANIFEST_SCHEMA,
            f"expected manifest schema {MANIFEST_SCHEMA}")
    require(manifest.get("source", {}).get("sha256") == SOURCE_SHA256,
            "manifest source hash does not match the pinned Steam build")
    require(layer_metadata.get("schemaVersion") == 1 and
            layer_metadata.get("sourceSha256") == SOURCE_SHA256 and
            layer_metadata.get("extractor") == "UndertaleModTool" and
            layer_metadata.get("version") == "0.9.2.0" and
            layer_metadata.get("commit") == "03c7048438347799e0717b6e57a9358d000472e0",
            "layer metadata provenance mismatch")
    metadata_rooms = {room["id"]: room for room in layer_metadata["rooms"]}

    resources = manifest["resources"]
    rooms_by_id = {room["id"]: room for room in resources["rooms"]}
    objects_by_id = {obj["id"]: obj for obj in resources["objects"]}
    room_order = manifest["gameMaker"]["roomOrder"]
    last_runtime_instance_id = manifest["gameMaker"]["lastObjectInstanceId"]
    require(0 < last_runtime_instance_id < 0xffffffff,
            "runtime object instance counter must admit a next ID")
    # The selected runner increments GEN8.LastObj before construction.
    next_runtime_instance_id = last_runtime_instance_id + 1

    room_records = []
    instance_records = []
    layer_records = []
    memberships = []

    for order_index, order_entry in enumerate(room_order):
        require(order_entry["index"] == order_index, "room order index is not contiguous")
        room = rooms_by_id[order_entry["roomId"]]
        require(room["name"] == order_entry["roomName"], "room order name mismatch")

        instances_by_id = {instance["instanceId"]: instance
                           for instance in room["instances"]}
        first_instance = len(instance_records)
        for creation_index, ordered in enumerate(room["instanceCreationOrder"]):
            require(ordered["index"] == creation_index,
                    f"room {room['id']} creation index is not contiguous")
            instance = instances_by_id[ordered["instanceId"]]
            require(instance["instanceId"] < next_runtime_instance_id,
                    "serialized instance ID crosses runtime allocator boundary")
            require(instance["objectId"] == ordered["objectId"],
                    f"room {room['id']} object mismatch")
            obj = objects_by_id[instance["objectId"]]
            flags = INSTANCE_PERSISTENT if obj["Persistent"] else 0
            instance_records.append(INSTANCE.pack(
                instance["instanceId"],
                instance["objectId"],
                instance["x"],
                instance["y"],
                float(instance["scaleX"]),
                float(instance["scaleY"]),
                float(instance["rotation"]),
                float(instance["imageSpeed"]),
                float(instance["imageIndex"]),
                instance["creationCodeId"],
                instance["preCreateCodeId"],
                flags,
            ))

        first_layer = len(layer_records)
        metadata_layers = metadata_rooms[room["id"]]["layers"]
        require(len(metadata_layers) == len(room["layers"]), "layer metadata count mismatch")
        for layer, metadata in zip(room["layers"], metadata_layers):
            values = metadata["values"]
            require(metadata["id"] == layer["id"] and metadata["type"] == layer["type"] and
                    values["LayerDepth"] == layer["depth"] and values["IsVisible"] == layer["visible"] and
                    metadata["instanceIds"] == layer["instanceIds"], "layer metadata identity mismatch")
            require(all(values[field] == 0 for field in ("XOffset", "YOffset", "HSpeed", "VSpeed")),
                    "nonzero layer motion requires a pack extension")
            kind = {"Background": 1, "Instances": 2}[layer["type"]]
            background = metadata["background"]
            background_flags = sum((1 << bit) for bit, field in enumerate(
                ("Visible", "Foreground", "TiledHorizontally", "TiledVertically", "Stretch"))
                if background.get(field, False))
            first_membership = len(memberships)
            for instance_id in layer["instanceIds"]:
                require(instance_id in instances_by_id,
                        f"room {room['id']} layer references unknown instance {instance_id}")
                memberships.append(MEMBERSHIP.pack(instance_id))
            layer_records.append(LAYER.pack(
                layer["id"],
                layer["depth"],
                LAYER_VISIBLE if layer["visible"] else 0,
                first_membership,
                len(layer["instanceIds"]),
                kind,
                metadata["backgroundSpriteId"],
                background.get("Color", 0),
                background_flags,
                background.get("FirstFrame", 0),
                background.get("AnimationSpeed", 0),
                background.get("AnimationSpeedType", 0),
            ))

        require(len(room["views"]) == 8, "expected eight serialized room views")
        room_records.append(ROOM.pack(
            room["id"],
            room["width"],
            room["height"],
            room["speed"],
            room["creationCodeId"],
            ROOM_PERSISTENT if room["persistent"] else 0,
            first_instance,
            len(room["instanceCreationOrder"]),
            first_layer,
            len(room["layers"]),
        ) + b"".join(VIEW.pack(*(view[field] for field in VIEW_FIELDS))
                   for view in room["views"]))

    require(len(instance_records) == EXPECTED_INSTANCES,
            "unexpected total room instance count")
    require(len(room_records) == EXPECTED_ROOMS == manifest["counts"]["rooms"],
            "room count mismatch")
    require(len(layer_records) == EXPECTED_LAYERS,
            "unexpected total room layer count")
    require(len(memberships) == len(instance_records),
            "each room instance must belong to exactly one layer")

    source_hash_prefix = int.from_bytes(bytes.fromhex(SOURCE_SHA256)[:8], "little")
    header = HEADER.pack(
        MAGIC,
        VERSION,
        HEADER.size,
        MANIFEST_SCHEMA,
        len(room_records),
        len(instance_records),
        len(layer_records),
        len(memberships),
        source_hash_prefix,
        next_runtime_instance_id,
    )
    return b"".join((
        header,
        b"".join(room_records),
        b"".join(instance_records),
        b"".join(layer_records),
        b"".join(memberships),
    ))


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
                        default=root / "generated/downwell-rooms.dwrm")
    parser.add_argument("--layer-metadata", type=Path,
                        default=root / "generated/room-layer-metadata.json")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()

    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    layer_metadata = json.loads(args.layer_metadata.read_text(encoding="utf-8"))
    payload = build_pack(manifest, layer_metadata)
    digest = hashlib.sha256(payload).hexdigest()

    if args.check:
        require(args.output.is_file(), f"missing room pack: {args.output}")
        require(args.output.read_bytes() == payload,
                "room pack differs from deterministic generator output")
        print(f"room pack verified: {len(payload)} bytes sha256 {digest}")
        return

    atomic_write(args.output, payload)
    print(f"room pack written: {len(payload)} bytes sha256 {digest}")


if __name__ == "__main__":
    main()
