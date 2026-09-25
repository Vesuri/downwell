#!/usr/bin/env python3
"""Guard reviewed image field binary32 storage in the pinned Windows runner."""
from hashlib import sha256
from pathlib import Path
import runpy
review = runpy.run_path(str(Path(__file__).with_name("verify-runner-choose.py")))
region = review["region"]
assert region(0x7FE3CC, 12) == b"image_index\0"
assert region(0x7FE47C, 12) == b"image_speed\0"
for start, end, digest in (
    (0x430D0C, 0x430D22, "dfd0255b5282e2fc5ca46c4826e10ff4ff839c2f5d5bab168e923331a441b187"),
    (0x430E0E, 0x430E24, "88a8f4957a1aef678da7b40e03bfa06b2796bbd211a2bc7deb0b77e270ac5c70"),
    (0x432170, 0x4321AF, "3f6cd2270d083f86b2c714d83cdde1a214caff6308d7d3892394a05a8f2e622f"),
    (0x432250, 0x43228F, "b8c74954a310548a83b0331b5b1729671e9f8b8d0d62c5fb49e57d83906d95e1"),
    (0x42DD90, 0x42DDB1, "3b477aa2f2db6874eb6234c247eafe4a22b7af19e95a40da6d305d84c09f2fdf"),
):
    assert sha256(region(start, end - start)).hexdigest() == digest
print("Pinned Windows image field registration, narrowing setters and speed getter verified")
