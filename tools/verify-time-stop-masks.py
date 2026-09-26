#!/usr/bin/env python3
"""Verify original player/shard masks before specializing field queries."""
import base64
import hashlib
import json
from pathlib import Path
root = Path(__file__).resolve().parents[1]
digest = "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"
assert hashlib.sha256((root / "data/data.win").read_bytes()).hexdigest() == digest
record = json.loads((root / "generated/time-stop-masks.json").read_text())
assert record["sourceSha256"] == digest
assert (record["extractor"],record["version"],record["commit"]) == (
    "UndertaleModTool","0.9.2.0","03c7048438347799e0717b6e57a9358d000472e0")
expected = (
    (31,"sprPlayerIdle",8,16,8,20,"76c9c0411bcc38717855ce9e8699c829324464855edbc72d774bd32088626979"),
    (91,"sprPoisonPotPieces",8,16,10,13,"d3957e5f4643a448c15473e55942c4e56554fbefb99deb99806177e98ba0010e"),
)
assert len(record["sprites"]) == len(expected)
for sprite, (id,name,left,right,top,bottom,mask_hash) in zip(record["sprites"],expected):
    assert (sprite["id"],sprite["name"]) == (id,name)
    assert len(sprite["masks"]) == 1
    mask = sprite["masks"][0]
    assert mask["width"] == mask["height"] == 24
    data = base64.b64decode(mask["data"],validate=True)
    assert len(data) == 72 and hashlib.sha256(data).hexdigest() == mask_hash
    for y in range(24):
        for x in range(24):
            assert bool(data[y*3+x//8] & (128>>(x%8))) == (left<=x<right and top<=y<bottom)
print("All 1152 player/shard mask pixels verified; no transparent holes in either collision rectangle")
