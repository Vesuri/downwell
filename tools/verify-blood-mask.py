#!/usr/bin/env python3
"""Verify sprite-84 mask specialization against the ignored UTMT export."""
import base64
import hashlib
import json
from pathlib import Path
root = Path(__file__).resolve().parents[1]
source_hash = "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"
assert hashlib.sha256((root / "data/data.win").read_bytes()).hexdigest() == source_hash
record = json.loads((root / "extracted/utmt-0.9.2.0/blood-mask.json").read_text())
assert record["sourceSha256"] == source_hash
assert record["extractor"] == "UndertaleModTool" and record["version"] == "0.9.2.0"
assert record["spriteId"] == 84 and record["width"] == record["height"] == 24
mask = base64.b64decode(record["data"], validate=True)
assert len(mask) == 72
assert hashlib.sha256(mask).hexdigest() == "9039517c6abab87f8c11ed8a0af996b2029d6238890f8c420d60191765136fe0"
for y in range(24):
    for x in range(24):
        assert bool(mask[y*3+x//8] & (128 >> (x%8))) == (10 <= x < 14 and 11 <= y < 14)
print("All 576 source blood mask pixels match the rectangle specialization")
