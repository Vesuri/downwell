"""Check results planar payloads against recovered source pixels and font masks."""
import importlib.util
import configparser
import json
from pathlib import Path

root = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("pack", root / "tools/generate-amiga-hud.py")
pack = importlib.util.module_from_spec(spec)
spec.loader.exec_module(pack)
extracted = root / "extracted/utmt-0.9.2.0"
manifest = json.loads((extracted / "manifest.json").read_text())
language = configparser.ConfigParser(interpolation=None, strict=False)
language.read(root / "data/language.ini", encoding="utf-8-sig")
for ident, key in enumerate(("menuGameOver", "menuRetry", "menuStyleSelect",
                             "menuSurface", "menuMaxed", "menuStyleUnlock",
                             "menuPaletteUnlock"), 65300):
    assert pack.TEXT_SELECTIONS["results"][ident] == language["english"][key].strip()
resources = manifest["resources"]
for glyph in resources["fonts"][2]["glyphs"]:
    if 47 <= glyph["character"] <= 57:
        assert glyph["shift"] == 8, "progress label layout assumes source eight-pixel advances"
        assert not glyph["kernings"], "progress glyph composition requires source digits without kerning"
pages = [pack.Image.open(extracted / "TexturePages" / f"{i}.png").convert("RGBA")
         for i in range(manifest["counts"]["embeddedTextures"])]
data = (root / "generated/amiga-results.dwap").read_bytes()
assert len(data) <= 32140, "results assets exceed the audited retired-garden gap"
magic, version, header_size, count, source_hash = pack.HEADER.unpack_from(data)
assert magic == pack.MAGIC and version == pack.VERSION and header_size == pack.HEADER.size
assert source_hash == bytes.fromhex(pack.SOURCE_SHA256)[:8]
seen = set()
pixels = 0
for index in range(count):
    record = pack.RECORD.unpack_from(data, pack.HEADER.size + index * pack.RECORD.size)
    ident, frame, ox, oy, width, height, dma_width, flags, offset, length = record
    assert flags == 3, "results art must be planar+mask, not runtime hardware sprites"
    assert (ident, frame) not in seen
    seen.add((ident, frame))
    if ident in pack.SELECTIONS["results"]:
        sprite = resources["sprites"][ident]
        source = pack.frame_canvas(sprite, sprite["frames"][frame], resources["texturePageItems"], pages)
        assert (ox, oy) == (sprite["originX"], sprite["originY"])
    else:
        source = pack.render_text(manifest, pages, pack.TEXT_SELECTIONS["results"][ident])
        assert frame == 0 and (ox, oy) == (source.width // 2, 0)
    assert (width, height) == source.size
    decoded = pack.decode_planar(data[offset:offset + length], width, height, dma_width)
    expected = [pack.pixel_to_pen(pixel) for pixel in source.get_flattened_data()]
    assert decoded == expected, f"source pixel mismatch: {ident}:{frame}"
    pixels += len(expected)
expected_records = {(ident, frame) for ident in pack.SELECTIONS["results"]
                    for frame in range(len(resources["sprites"][ident]["frames"]))}
expected_records |= {(ident, 0) for ident in pack.TEXT_SELECTIONS["results"]}
assert seen == expected_records
print(f"Results pack: {len(seen)} records, {pixels} source pixels and opacity masks verified")
