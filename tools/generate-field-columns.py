#!/usr/bin/env python3
"""Pin inputs and output for native large-field column commands."""
import hashlib
import json
import subprocess
from pathlib import Path

root = Path(__file__).resolve().parents[1]
output = root / "generated/downwell-field-columns.h"
output.parent.mkdir(parents=True, exist_ok=True)
bank = root / "generated/amiga-field-bands.bin"
subprocess.run([str(root / "build/generate-field-columns"), str(output), str(bank)], check=True)
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
inputs = [root / "tools/generate-field-columns.cpp", *sorted((root / "src/core").glob("*.h"))]
output.with_suffix(".json").write_text(json.dumps(dict(
    schemaVersion=3, generator="generate-field-columns/v3",
    generatorSha256=sha(Path(__file__)),
    inputs={str(p.relative_to(root)): sha(p) for p in inputs},
    outputSha256=sha(output), radii=512, nativeWordBytes=8,
    bankSha256=sha(bank), bankBytes=bank.stat().st_size,
    layout="deduplicated columns of signed row/word offsets and two planar colour words",
    bands="176-pixel top/bottom arcs; white and coverage planes; supported centres 182..198",
    oracle="GoalFieldOutline portable raster; not a Windows GPU pixel oracle"), indent=2)+"\n")
