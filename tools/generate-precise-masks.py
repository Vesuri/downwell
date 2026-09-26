#!/usr/bin/env python3
"""Preserve original precise collision bytes in an endian-explicit mask pack."""
import argparse
import base64
import hashlib
import json
from pathlib import Path
import runpy
import struct

ROOT = Path(__file__).resolve().parents[1]
common = runpy.run_path(str(ROOT / "tools/generate-sprite-pack.py"))
require, atomic_write = common["require"], common["atomic_write"]
sha = lambda data: hashlib.sha256(data).hexdigest()

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    source = ROOT / "generated/precise-sprite-masks.json"
    manifest_path = ROOT / "extracted/utmt-0.9.2.0/manifest.json"
    record = json.loads(source.read_text())
    manifest = json.loads(manifest_path.read_text())
    require(record["sourceSha256"] == manifest["source"]["sha256"] == common["SOURCE_SHA256"], "source mismatch")
    require((record["extractor"], record["version"], record["commit"]) ==
            ("UndertaleModTool", "0.9.2.0", "03c7048438347799e0717b6e57a9358d000472e0"), "extractor mismatch")
    expected = [s for s in manifest["resources"]["sprites"] if s["separateMasks"] == "Precise"]
    require([s["id"] for s in record["sprites"]] == [s["id"] for s in expected], "precise sprite coverage mismatch")
    masks = []
    for exported, sprite in zip(record["sprites"], expected):
        require(exported["name"] == sprite["name"] and len(exported["masks"]) == sprite["collisionMaskCount"], "mask count mismatch")
        for frame, mask in enumerate(exported["masks"]):
            width, height = mask["width"], mask["height"]
            raw = base64.b64decode(mask["data"], validate=True)
            require(mask["frame"] == frame and width > 0 and height > 0, "invalid mask dimensions/frame")
            require((width, height) == (sprite["width"], sprite["height"]), "mask canvas mismatch")
            require(len(raw) == ((width + 7) // 8) * height, "invalid mask stride/size")
            masks.append((sprite["id"], frame, width, height, raw))
    header = struct.pack("<4sHH8sII", b"DWSM", 1, 24, bytes.fromhex(common["SOURCE_SHA256"])[:8], len(masks), 3)
    offset = len(header) + 20 * len(masks)
    descriptors, data = [], []
    for sprite, frame, width, height, raw in masks:
        descriptors.append(struct.pack("<IIIII", sprite, frame, width, height, offset))
        data.append(raw); offset += len(raw)
    payload = header + b"".join(descriptors + data)
    output = ROOT / "generated/downwell-sprite-masks.dwsm"
    provenance = (json.dumps({
        "formatVersion": 1, "source": manifest["source"], "extractor": manifest["extractor"],
        "exportSha256": sha(source.read_bytes()), "manifestSha256": sha(manifest_path.read_bytes()),
        "generatorSha256": sha(Path(__file__).read_bytes()),
        "exportScriptSha256": sha((ROOT / "tools/export-precise-masks.csx").read_bytes()),
        "packSha256": sha(payload), "masks": [dict(spriteId=s, frame=f, sha256=sha(raw)) for s,f,w,h,raw in masks]
    }, sort_keys=True, indent=2) + "\n").encode()
    for path, content in ((output, payload), (output.with_suffix(".manifest.json"), provenance)):
        if args.check:
            require(path.is_file() and path.read_bytes() == content, f"generated data mismatch: {path}")
        else:
            atomic_write(path, content)
    print(f"Precise masks: {len(masks)} masks, {len(payload)} bytes, sha256 {sha(payload)}")

if __name__ == "__main__":
    main()
