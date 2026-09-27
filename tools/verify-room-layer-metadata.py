#!/usr/bin/env python3
"""Cross-check exported layer scalars against the pinned raw ROOM records."""
from collections import Counter
from hashlib import sha256
import json
from pathlib import Path
import struct

root = Path(__file__).resolve().parents[1]
data = (root / "data/data.win").read_bytes()
metadata = json.loads((root / "generated/room-layer-metadata.json").read_text())
assert sha256(data).hexdigest() == metadata["sourceSha256"] == "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"
assert metadata["schemaVersion"] == 1 and metadata["extractor"] == "UndertaleModTool"
assert metadata["version"] == "0.9.2.0" and metadata["commit"] == "03c7048438347799e0717b6e57a9358d000472e0"

def u32(offset):
    return struct.unpack_from("<I", data, offset)[0]

def i32(offset):
    return struct.unpack_from("<i", data, offset)[0]

def f32(offset):
    return struct.unpack_from("<f", data, offset)[0]

offset = 8
while data[offset:offset + 4] != b"ROOM":
    offset += 8 + u32(offset + 4)
assert u32(offset + 8) == len(metadata["rooms"]) == 25
types, backgrounds = Counter(), Counter()
for room_id, room in enumerate(metadata["rooms"]):
    assert room["id"] == room_id
    raw_room = u32(offset + 12 + 4 * room_id)
    layers = u32(raw_room + 0x58)
    assert u32(layers) == len(room["layers"])
    for index, layer in enumerate(room["layers"]):
        raw = u32(layers + 4 + index * 4)
        values = layer["values"]
        assert layer["id"] == values["LayerId"] == u32(raw + 4)
        kind = u32(raw + 8)
        assert values["LayerType"] == kind
        assert values["LayerDepth"] == i32(raw + 12)
        for field, at in (("XOffset", 16), ("YOffset", 20), ("HSpeed", 24), ("VSpeed", 28)):
            assert values[field] == f32(raw + at) == 0
        assert values["IsVisible"] == bool(u32(raw + 32))
        assert values["EffectEnabled"] == bool(u32(raw + 36))
        # This build has no effect name or properties in any serialized layer.
        assert u32(raw + 40) == u32(raw + 44) == 0
        types[kind] += 1
        if kind == 2:
            assert layer["type"] == "Instances" and not layer["background"]
            count = u32(raw + 48)
            assert layer["instanceIds"] == list(struct.unpack_from(f"<{count}I", data, raw + 52))
        else:
            assert kind == 1 and layer["type"] == "Background" and not layer["instanceIds"]
            bg = layer["background"]
            for field, at in (("Visible", 48), ("Foreground", 52), ("TiledHorizontally", 60),
                              ("TiledVertically", 64), ("Stretch", 68)):
                assert bg[field] == bool(u32(raw + at))
            sprite = i32(raw + 56)
            assert layer["backgroundSpriteId"] == sprite
            assert bg["Color"] == u32(raw + 72)
            assert bg["FirstFrame"] == f32(raw + 76)
            assert bg["AnimationSpeed"] == f32(raw + 80)
            assert bg["AnimationSpeedType"] == u32(raw + 84)
            backgrounds[sprite] += 1
assert types == {1: 24, 2: 108}
assert backgrounds == {-1: 18, 740: 6}
print("Raw ROOM metadata verified: 108 instance layers, 24 backgrounds (6 sprite 740, 18 sprite-less); zero layer motion/effect properties")
