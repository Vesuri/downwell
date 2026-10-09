#!/usr/bin/env python3
"""Pin reviewed string_width/font lookup paths in the untouched Windows runner."""
from hashlib import sha256
from pathlib import Path
import runpy
import struct

runner = runpy.run_path(str(Path(__file__).with_name("verify-runner-choose.py")))
region = runner["region"]
assert region(0x811D04, 13) == b"string_width\0"
assert struct.unpack("<I", region(0x537D33, 4))[0] == 0x535D50
assert struct.unpack("<I", region(0x537D38, 4))[0] == 0x811D04
for start, end, digest in (
    (0x535D50, 0x535D92, "c31c8bab74878c8893b65a0a30ea377087c131b8fc0e5745f851f322b772e6c2"),
    (0x45A980, 0x45A9B4, "da0f94a4dd88068c26217761a9a8552242351d76b604c908c6f08d3c77a4d5d5"),
    (0x45AA40, 0x45AB97, "dc9bccd9e67e85534448222b9cf4f28d10e507da8e934ec0cd434c572bce353d"),
    (0x44A2A0, 0x44A316, "dda7bed6b0480edc1e97d4f0a21f2efafc4fb87cf92acbbc46f34337710acfbf"),
    (0x449830, 0x449903, "b017c9b0bcb5a50dd582c81861621fafb571066378b9597f90127125376cba83"),
    (0x45ADC0, 0x45B0A0, "bfd0506c9cdffe3d7f59b61d0f6d8a87f817ee442a6e36074873a6e17fada7dc"),
):
    assert sha256(region(start, end-start)).hexdigest() == digest
assert region(0x44A2D4,4) == bytes.fromhex("0f bf 48 0a") # signed glyph advance
assert region(0x44A2E6,8) == bytes.fromhex("f3 0f 59 8b 8c 00 00 00") # binary32 font scale
assert region(0x44A2F1,8) == bytes.fromhex("f3 0f 58 c8 f3 0f 2c f9") # add then truncate
assert region(0x449894,6) == bytes.fromhex("81 fa af 25 00 00") # fallback termination
assembly = Path(__file__).resolve().parents[1]/"extracted/utmt-0.9.2.0/Assembly"
initialize = (assembly/"gml_GlobalScript_scrInitialize.asm").read_text()
assert ":[54]\npushi.e 2\nconv.i.v\ncall.i draw_set_font(argc=1)\npopz.v" in initialize
assert "draw_set_font" not in (assembly/"gml_Object_levelNumber_Create_0.asm").read_text()
print("Pinned runner string_width: selected font, fallback glyph, scaled per-glyph truncation; no kerning")
