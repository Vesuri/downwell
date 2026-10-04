#!/usr/bin/env python3
"""Pin inputs and output for native large-field column commands."""
import hashlib
import json
import subprocess
from pathlib import Path

root = Path(__file__).resolve().parents[1]
output = root / "generated/downwell-field-columns.h"
output.parent.mkdir(parents=True, exist_ok=True)
subprocess.run([str(root / "build/generate-field-columns"), str(output)], check=True)
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
inputs = [root / "tools/generate-field-columns.cpp", *sorted((root / "src/core").glob("*.h"))]
output.with_suffix(".json").write_text(json.dumps(dict(
    schemaVersion=1, generator="generate-field-columns/v1",
    generatorSha256=sha(Path(__file__)),
    inputs={str(p.relative_to(root)): sha(p) for p in inputs},
    outputSha256=sha(output), radii=512, nativeWordBytes=8,
    layout="deduplicated columns of signed row/word offsets and two planar colour words",
    oracle="GoalFieldOutline portable raster; not a Windows GPU pixel oracle"), indent=2)+"\n")
