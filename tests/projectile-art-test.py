#!/usr/bin/env python3
"""Exercise the conservative offline spread check with synthetic artwork."""
import importlib.util
from pathlib import Path
from PIL import Image

root = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("planar", root / "tools/generate-amiga-hud.py")
planar = importlib.util.module_from_spec(spec)
spec.loader.exec_module(planar)

central = Image.new("RGBA", (24, 24))
central.putpixel((12, 12), (255, 255, 255, 255))
planar.verify_projectile_spread_pixels(central, 12, 12)

for corner in ((0, 0), (23, 0), (0, 23), (23, 23)):
    moving = Image.new("RGBA", (24, 24))
    moving.putpixel(corner, (255, 255, 255, 255))
    try:
        planar.verify_projectile_spread_pixels(moving, 12, 12)
    except ValueError:
        pass
    else:
        raise AssertionError(f"spread checker accepted a moving corner pixel: {corner}")

try:
    planar.verify_projectile_spread_pixels(central, 11, 12)
except ValueError:
    pass
else:
    raise AssertionError("spread checker accepted an unsupported anchor")

print("Projectile spread check accepts invariant art and rejects moving pixels/unsupported anchors")
