"""Compare actual SDL samples with the source-derived invariant raster."""
import importlib.util
import json
from pathlib import Path
import sys
from PIL import Image, ImageChops

root = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("planar", root / "tools/generate-amiga-hud.py")
planar = importlib.util.module_from_spec(spec)
spec.loader.exec_module(planar)
export = root / "extracted/utmt-0.9.2.0"
manifest = json.loads((export / "manifest.json").read_text())
assert manifest["source"]["sha256"] == planar.SOURCE_SHA256
resources = manifest["resources"]
pages = [Image.open(export / "TexturePages" / f"{i}.png").convert("RGBA")
         for i in range(manifest["counts"]["embeddedTextures"])]
expected = Image.new("RGBA", (380, 284), (0, 0, 0, 255))
for row in range(12):
    sprite = resources["sprites"][458 + row // 6]
    canvas = planar.frame_canvas(sprite, sprite["frames"][row % 6], resources["texturePageItems"], pages)
    planar.verify_projectile_spread_pixels(canvas, sprite["originX"], sprite["originY"])
    rotated = canvas.transpose(Image.Transpose.ROTATE_270)
    for column in range(15):
        expected.alpha_composite(rotated, (12 + column * 24 - sprite["originX"],
                                          6 + row * 23 - sprite["originY"]))
files = list(Path(sys.argv[1]).glob("*.png"))
assert len(files) == 1, "expected exactly one raster capture"
actual = Image.open(files[0]).convert("RGB")
assert actual.size == expected.size
assert ImageChops.difference(actual, expected.convert("RGB")).getbbox() is None, "SDL spread raster differs from source samples"
print("180 actual projectile transforms match the source raster across all twelve frames")
