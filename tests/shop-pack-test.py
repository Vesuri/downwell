"""Verify the shop bank's text, origins and planar masks against source glyphs."""
import importlib.util
import json
from pathlib import Path

root = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("pack", root / "tools/generate-amiga-hud.py")
pack = importlib.util.module_from_spec(spec)
spec.loader.exec_module(pack)
extracted = root / "extracted/utmt-0.9.2.0"
manifest = json.loads((extracted / "manifest.json").read_text())
resources = manifest["resources"]
texts = pack.shop_texts(root / "data/language.ini")
glyphs = {g["character"]: g for g in resources["fonts"][2]["glyphs"]}
for character in "0123456789 G":
    assert glyphs[ord(character)]["shift"] == 8, "shared price compositor requires original eight-pixel advances"
for text in texts.values():
    for character in text.replace("\n", ""):
        assert not glyphs[ord(character)]["kernings"], "shop rasterizer requires unkerned source font"
assert "\n" in texts[65434], "energy drink description must retain its two lines"
pages = [pack.Image.open(extracted / "TexturePages" / f"{i}.png").convert("RGBA")
         for i in range(manifest["counts"]["embeddedTextures"])]
data = (root / "generated/amiga-shop.dwap").read_bytes()
_, _, _, count, _ = pack.HEADER.unpack_from(data)
seen = set()
for i in range(count):
    ident, frame, ox, oy, width, height, stride, flags, offset, length = pack.RECORD.unpack_from(
        data, pack.HEADER.size + i * pack.RECORD.size)
    assert flags == 3 and (ident, frame) not in seen
    seen.add((ident, frame))
    if ident not in texts:
        continue  # artwork round trips also checked during pack generation
    assert frame == 0 and (ox, oy) == (width // 2, height // 2)
    # Build each line independently, then center it into the multiline canvas.
    lines = [pack.render_text(manifest, pages, line) for line in texts[ident].split("\n")]
    expected = pack.Image.new("RGBA", (width, height))
    for row, line in enumerate(lines):
        expected.alpha_composite(line, ((width - line.width) // 2, row * 12))
    decoded = pack.decode_planar(data[offset:offset + length], width, height, stride)
    assert decoded == [pack.pixel_to_pen(p) for p in expected.get_flattened_data()]
assert {(ident, 0) for ident in texts} <= seen
assert count == 18 + len(texts)
print(f"Shop bank: {count} records; source text masks and middle/center anchors verified")
