#!/usr/bin/env python3
"""Guard the reviewed sound and music-duck VM scripts, not audio backend fidelity."""
from hashlib import sha256
import json
from pathlib import Path

root = Path(__file__).resolve().parents[1]
export = root / "extracted/utmt-0.9.2.0"
manifest = json.loads((export / "manifest.json").read_text())
assert manifest["source"]["sha256"] == "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"
assert manifest["extractor"]["version"] == "0.9.2.0"
assert manifest["extractor"]["commit"] == "03c7048438347799e0717b6e57a9358d000472e0"
for name, digest in (
    ("soundPlay", "9aefb5fdbe118719d35e19389d76f2ea22990b67357d777850395c2d87edd33e"),
    ("soundPlayOL", "71426bbaa0d41da4ab49b5179a6f28b0f6dfe6e003785afaee60969182944553"),
    ("soundInitialize", "d551673a4b80ecd7ecf26edbb67188fa437077a910cf2cd5913e2fc7806d0850"),
    ("bgmDuck", "eed2f0b147ffcceeb87ec162d0ce522d7b3069cba83bcf11f6ac0ed676c0e2c2"),
):
    assert sha256((export / f"Assembly/gml_GlobalScript_{name}.asm").read_bytes()).hexdigest() == digest
print("Pinned sound scripts: caller writes, ten channels, cut/visibility/water/music gates and voice returns")
