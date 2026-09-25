#!/usr/bin/env python3
"""Pin the reviewed D3D11 effect blend path; not a Windows execution test."""
from hashlib import sha256
from pathlib import Path
import runpy
import struct

audit = runpy.run_path(str(Path(__file__).with_name("verify-runner-choose.py")))
region = audit["region"]
for start, end, digest in (
    (0x519d41, 0x519d54, "df810dbe425023f9efd0337e511ad73bee06b0bddcf86fb505bc9d9b4e189721"),
    (0x518ab0, 0x518bbf, "ca3c6dfcd242119a6d0a67b9412f2c4b98506ebe00d5b007df134f024e1f3bc4"),
    (0x4502b0, 0x450356, "9b033c9c29495fd3db5e2b9546ed18dd0e9a83b25c405d32650062927e12db5a"),
    (0x44e370, 0x44e3ce, "a205633c9f93b17dddcd2ca7e92213ca7731723e114b9e3a189145a5b59cecbb"),
    (0x6683b0, 0x668670, "2f282bc7d27f25a8f6c276211e8dbceaea4806f1279ad021e58ee327d1a5bc5f"),
    (0x66c390, 0x66c4c5, "76d1503419e1aa2b7bdaa74e702a19380ac96c193cf951e680f48607f4244a21"),
    (0x66c07e, 0x66c135, "a9976242216dfc0bb9ef0bbabb750491e67f7f54d9307dddbf3a2ae1438acc30"),
):
    assert sha256(region(start, end-start)).hexdigest() == digest

def u32(address):
    return struct.unpack("<I", region(address, 4))[0]

def constant_return(address):
    assert region(address, 1) == b"\xb8" and region(address+5, 1) == b"\xc3"
    return u32(address+1)

def rgb_factor(value):
    return constant_return(u32(0x66c3f0+4*(value-1)))

def alpha_factor(value):
    index = region(0x66c4b4+value-1, 1)[0]
    return constant_return(u32(0x66c490+4*index))

# Mode 3's reviewed wrapper sets source 1/destination 4 for RGB and alpha.
# The alpha translator replaces inverse source COLOR with inverse source ALPHA.
assert (rgb_factor(1), rgb_factor(4), alpha_factor(1), alpha_factor(4)) == (1,4,1,6)
# Normal mode restores SRC_ALPHA / INV_SRC_ALPHA for both channels.
assert (rgb_factor(5), rgb_factor(6), alpha_factor(5), alpha_factor(6)) == (5,6,5,6)
# Internal state slots reach the reviewed RGB/alpha fields unchanged.
assert [u32(0x6685e4+4*(slot-1)) for slot in (6,7,34,35)] == [0x668435,0x668446,0x6685c2,0x6685d3]
print("Pinned effect blend: RGB ZERO/INV_SRC_COLOR, alpha ZERO/INV_SRC_ALPHA, additive operations")
