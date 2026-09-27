#!/usr/bin/env python3
"""Pin reviewed ordinary bitmap Draw paths; not a GPU rasterization proof."""
from hashlib import sha256
from pathlib import Path
import runpy
import struct

audit = runpy.run_path(str(Path(__file__).with_name("verify-runner-choose.py")))
region = audit["region"]
for start, end, digest in (
    # draw_self registration and wrapper; the latter passes instance float X/Y.
    (0x537e6a, 0x537e7d, "a422ad160b6b892d09e457d12fdfdd3489baec0016006068db3042e5aceb25a2"),
    (0x531350, 0x5313fc, "b8cf9a8f80aa2033136c0afde7cfd3ac7eafb951ee683ee481372ba229286e91"),
    (0x4af6f0, 0x4afb39, "1ff9fd1c67981466e9140eff8ef2662811e4432e76ae64844acb9618915bdab4"),
    # Default instance Draw, its float field loads and bitmap dispatch.
    (0x490f65, 0x490f8f, "3783cce0945cc6cb59ba7d5c21935af793514678d2d2e9bff7ec752ea2b604c1"),
    (0x4afb94, 0x4afbf8, "9047e6d4c53e34dad8a92b773152611146b1f9e1c96d0adefcaba2f66246454e"),
    (0x4afea8, 0x4aff48, "ebd62d96546cc80693d7c5137fc10ff5845224e1ad1f8991edb74621d5733dbc"),
    # Explicit Draw bitmap dispatch and the two float vertex builders.
    (0x4afa5b, 0x4afaf0, "2468ec06887bd4f8cd868f42aaac109bc23f471277d989bc695b6be1c314f908"),
    (0x45c3f0, 0x45c77d, "ed587d6db3f6079ba3bd47224e592f2c1c237e58d4aced56b443594ade84156c"),
    (0x45c780, 0x45cb64, "6866fc12a532ccd3ea557e7a75c46ec07c53e58da192c50fa035df9b27911524"),
):
    assert sha256(region(start, end-start)).hexdigest() == digest
assert region(0x811e6c, 10) == b"draw_self\0"
assert struct.unpack("<I", region(0x7fe7d0, 4))[0] == 0x40490fdb  # float pi
assert struct.unpack("<f", region(0x7fe7d4, 4))[0] == 180.0
assert struct.unpack("<I", region(0x800300, 4))[0] == 0x3a83126f  # float 0.001
print("Pinned ordinary sprite float-position forwarding and unrounded CPU vertex construction")
print("GPU transform, pixel-centre/tie rules and fractional raster equivalence remain unproven")
