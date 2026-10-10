#!/usr/bin/env python3
"""Prove the wall's exact original tile patch can use an opaque black fill."""
from pathlib import Path
import hashlib
import json
from PIL import Image

root = Path(__file__).resolve().parents[1]
export = root / "extracted/utmt-0.9.2.0"
manifest = json.loads((export / "manifest.json").read_text())
digest = "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"
assert manifest["source"]["sha256"] == digest
assert hashlib.sha256((root / "data/data.win").read_bytes()).hexdigest() == digest
assert manifest["extractor"]["version"] == "0.9.2.0"
assert hashlib.sha256((export / "Assembly/gml_GlobalScript_tile_add.asm").read_bytes()).hexdigest() == \
    "68bba6cb8861dafcabe5fd469d67d8002c0508dcca330ebf7f9bc182ada8eb8c"
assert hashlib.sha256((export / "Assembly/gml_GlobalScript_tile_layer_find.asm").read_bytes()).hexdigest() == \
    "f0510e9d42df02317e319f030a7e8a64642b65053a3898ec8d9bd2df42cb7087"
resources = manifest["resources"]
sprite = resources["sprites"][739]
assert sprite["id"] == 739 and sprite["name"] == "tileCavern"
assert len(sprite["frames"]) == 1
item = resources["texturePageItems"][sprite["frames"][0]["texturePageItemId"]]
assert item["texturePageId"] == 1
assert hashlib.sha256((export / "TexturePages/1.png").read_bytes()).hexdigest() == \
    "c3dcc697cffacb53b1c373a9a079ea190e306f0e1d5e2070377614f546245287"
assert item["sourceWidth"] == item["targetWidth"] and item["sourceHeight"] == item["targetHeight"]
assert item["targetX"] <= 48 and item["targetY"] <= 48
assert item["targetX"] + item["targetWidth"] >= 64 and item["targetY"] + item["targetHeight"] >= 64
with Image.open(export / "TexturePages" / f'{item["texturePageId"]}.png') as image:
    rgba = image.convert("RGBA")
    for y in range(48, 64):
        for x in range(48, 64):
            assert rgba.getpixel((item["sourceX"] + x - item["targetX"],
                                  item["sourceY"] + y - item["targetY"])) == (0, 0, 0, 255)
print("Original tile 739 (48,48,16,16): all 256 pixels are opaque black")
