#!/usr/bin/env python3
"""Guard the reviewed move_snap wrapper and finite-coordinate implementation."""
from pathlib import Path
from hashlib import sha256
import runpy

region = runpy.run_path(str(Path(__file__).with_name("verify-runner-choose.py")))["region"]
assert region(0x80c978,10) == b"move_snap\0"
for start,end,digest in (
    (0x52a600,0x52a62f,"f9078ba6581a3ca1c97ae0e7dd2d6d5615e3b694b4ea322deaa68054130bbb22"),
    (0x4ff4ce,0x4ff556,"ec85a091ae6c1c1630e2011db9d6ceec334e4dfeda47ec8f09ec3561d657ef7f"),
):
    assert sha256(region(start,end-start)).hexdigest()==digest
print("Pinned Windows move_snap wrapper, float division, fistp and multiplication verified")
