#!/usr/bin/env python3
"""Pin original text coordinate narrowing and persistent alignment setters."""
from hashlib import sha256
from pathlib import Path
import runpy
import struct

root = Path(__file__).resolve().parents[1]
runner = runpy.run_path(str(Path(__file__).with_name('verify-runner-choose.py')))
region = runner['region']  # Also verifies the complete untouched executable hash.
for name, address, registration, wrapper in (
    ('draw_set_halign',0x811CD4,0x537CFB,0x531480),
    ('draw_set_valign',0x811CF4,0x537D24,0x5314E0),
    ('draw_text',0x811D4C,0x537D86,0x5323A0),
):
    assert region(address,len(name)+1)==name.encode()+b'\0'
    assert region(registration-5,10)==b'\x68'+struct.pack('<I',wrapper)+b'\x68'+struct.pack('<I',address)
for start,end,digest in (
    (0x531480,0x531495,'f44cafee81df2453019f53ab81155ae6ba73ace44cd6de9467ba0d1017527835'),
    (0x5314E0,0x5314F5,'6966d19832cab27b7dcd4f7170da3b09d90cad4ecad40556713e4b55a5de9b8c'),
    (0x5323A0,0x5323FC,'4f2020ca91611df17779f6f6e820ffafc734d3e65bc75f7b39b48da20058211a'),
    (0x45A9C0,0x45A9F4,'75342af80d5598665e7a59e05e0da0851fd26d1255db96d3652c3d8aa9ec398f'),
    (0x45AA00,0x45AA34,'2b2630c125db33b6193cc82ba173c74eb40a9d1cfb9512ccc81fcf2a8a2fdb12'),
    (0x459D9C,0x459E38,'e6e4fe5bc2244e224b55206b0c835b92467122e86d3ced8a5b43a090b3955763'),
    (0x45A940,0x45A973,'ff711292f49058bdea687049bdb8bc26276a1bc2c7bc990955f34bc1e729c31d'),
):
    assert sha256(region(start,end-start)).hexdigest()==digest,hex(start)
# draw_text narrows both numeric arguments using FSTP m32 before 0x459C50.
assert region(0x5323D7,3)==bytes.fromhex('d9 1c 24')
assert region(0x5323E5,3)==bytes.fromhex('d9 1c 24')
# Centered alignment: signed truncation of width/2, negate, convert to f32.
assert region(0x459DBA,11)==bytes.fromhex('99 2b c2 d1 f8 f7 d8 66 0f 6e c0')
assert region(0x459E29,5)==bytes.fromhex('f3 0f 58 45 08')
assembly=root/'extracted/utmt-0.9.2.0/Assembly'
player=(assembly/'gml_GlobalScript_playerDraw.asm').read_text()
assert player.count('call.i draw_set_halign(argc=1)')==1
assert ':[5]\npushi.e 1\nconv.i.v\ncall.i draw_set_halign(argc=1)' in player
title=(assembly/'gml_Object_levelNumber_Draw_0.asm').read_text()
for name in ('draw_set_font','draw_set_halign','draw_set_valign','draw_set_alpha'):
    assert name not in title
print('Pinned text Draw: persistent alignment, binary32 input narrowing, signed half-width centering')
