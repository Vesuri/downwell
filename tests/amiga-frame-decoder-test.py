#!/usr/bin/env python3
"""Check fixed sprite-channel priority and the foreground HUD on decoded dumps."""
import importlib.util
from pathlib import Path
import struct

spec = importlib.util.spec_from_file_location("decoder", Path(__file__).resolve().parents[1] / "tools/decode-amiga-gameplay-frame.py")
d = importlib.util.module_from_spec(spec)
spec.loader.exec_module(d)
sprite_start = 2 + 2*d.RING_PLANE_BYTES + 2*d.HUD_PLANE_BYTES
payload = bytearray(sprite_start + 8*d.SPRITE_BYTES)
x, y = 120, 8
sx, sy = d.DISPLAY_HARDWARE_X + d.DISPLAY_X + x, d.DISPLAY_HARDWARE_Y + y
for channel, low, high in ((0, 0x8000, 0), (7, 0, 0x8000)):
    start = sprite_start + channel*d.SPRITE_BYTES
    payload[start:start+4] = bytes((sy, sx >> 1, sy+1, sx & 1))
    struct.pack_into(">HH", payload, start+4, low, high)
assert d.decode(payload).getpixel((x,y)) == 1  # Channel zero covers channel seven.
struct.pack_into(">HH", payload, sprite_start+4, 0, 0)
assert d.decode(payload).getpixel((x,y)) == 2  # Transparent front pixel reveals red.
for plane in range(2):
    offset = 2 + 2*d.RING_PLANE_BYTES + plane*d.HUD_PLANE_BYTES + y*d.ROW_BYTES + (x+d.DISPLAY_X)//8
    payload[offset] |= 0x80 >> ((x+d.DISPLAY_X)%8)
assert d.decode(payload).getpixel((x,y)) == 3  # Opaque-black PF2 is above all sprites.
print("Amiga frame decoder: sprite priority, transparency and PF2 priority passed")
