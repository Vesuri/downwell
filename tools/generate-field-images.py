#!/usr/bin/env python3
"""Generate native field images and pin the portable geometry inputs."""
import hashlib
import json
import struct
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
output = ROOT / "generated/amiga-field-images.bin"
output.parent.mkdir(parents=True, exist_ok=True)
subprocess.run([str(ROOT / "build/generate-field-images"), str(output)], check=True)
payload = output.read_bytes()
magic, version, count, width, height, ox, oy = struct.unpack_from(">4s6H", payload)
assert (magic, version, width, height, ox, oy) == (b"DWFI", 1, 64, 56, 32, 28)
assert len(payload) == 1040 + count*64*56*3//8
inputs = [ROOT / "tools/generate-field-images.cpp", ROOT / "generated/downwell-field-spans.h"]
# Hash all core header inputs, including the portable arithmetic used by the
# raster. These are mathematical/hand-authored inputs, not extracted payloads.
inputs += sorted((ROOT / "src/core").glob("*.h"))
sha = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
manifest = dict(schemaVersion=1, generator="generate-field-images/v1",
    generatorSha256=sha(Path(__file__)), inputs={str(p.relative_to(ROOT)): sha(p) for p in inputs},
    outputSha256=sha(output), bytes=len(payload), images=count, radii=512,
    width=width, height=height, origin=[ox,oy],
    layout="16-byte BE header, 512 uint16 image IDs, frame-major low/high/mask planes",
    oracle="GoalFieldOutline portable raster; not a Windows GPU pixel oracle")
output.with_suffix(".json").write_text(json.dumps(manifest, indent=2)+"\n")
