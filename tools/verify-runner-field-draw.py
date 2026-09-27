#!/usr/bin/env python3
"""Pin dynamic instance insertion and legacy Draw traversal; not execution."""
from hashlib import sha256
from pathlib import Path
import runpy
audit=runpy.run_path(str(Path(__file__).with_name("verify-runner-choose.py")))
region=audit["region"]
for a,b,digest in (
    (0x52cd31,0x52cd44,"e7ef86a07a0fdc8be943bd2fa06a9b7b2836e617a090784a9c8143ceab1e6f9e"),
    (0x529840,0x5298d5,"8a192e16e0d701ce5d1764116bff134efda7d62db174111ca742e69132179d92"),
    (0x4fdcf0,0x4fdd9e,"0900e53961c9b92bbb3d0b7fd7a3201651efe731aeff731ae2ebfd217fd3b17e"),
    (0x4878f0,0x487a0e,"bc6d0419daf3bfa9df2f8ee3a442711fd75ca1bfb2919f42ddb8eff9266d5c1c"),
    (0x487610,0x4876e4,"83b426af059ee4b353f6dd49cdfa841656ef75fd6fcf343235cf3fe5871224d8"),
    (0x48d69c,0x48d760,"6c981cde10765822d44e70ca244835b1c066e006f13a23576b46b0d57466cd4a"),
    (0x48e9a0,0x48eb6e,"ab87dd56d927e53f2339661f59fde578f37a2a5472ed31e335b31d2d0eedfe70"),
    (0x490f11,0x490f95,"2fb3b3d31cffd9d6f23fc1be222a529f80eb462c267d89d626d90d74e5132efd"),
):
    assert sha256(region(a,b-a)).hexdigest()==digest
assert region(0x80cda0,22)==b"instance_create_depth\0"
print("Pinned instance_create_depth insertion, reactivation and equal-depth Draw-list traversal")
