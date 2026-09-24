"""Check recovered level-clear art/text and the future selected-offer bank bound."""
import importlib.util
import json
from pathlib import Path

root = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("pack", root / "tools/generate-amiga-hud.py")
pack = importlib.util.module_from_spec(spec)
spec.loader.exec_module(pack)
extracted = root / "extracted/utmt-0.9.2.0"
manifest = json.loads((extracted / "manifest.json").read_text())
pages = [pack.Image.open(extracted / "TexturePages" / f"{i}.png").convert("RGBA")
         for i in range(manifest["counts"]["embeddedTextures"])]
texts = pack.clear_texts(root / "data/language.ini")
data = (root / "generated/amiga-clear.dwap").read_bytes()
records = {}
for i in range(pack.HEADER.unpack_from(data)[3]):
    record = pack.RECORD.unpack_from(data, pack.HEADER.size + i * pack.RECORD.size)
    ident, frame, ox, oy, width, height, stride, flags, offset, length = record
    if ident in (51, 74, 72, 70, 55):
        expected_geometry = (32, 32, 16, 16) if ident == 70 else (16, 16, 8, 8)
        assert (width, height, ox, oy) == expected_geometry, "circular border cache geometry contract"
    assert flags == 3 and (ident, frame) not in records
    records[ident, frame] = record
    if ident in texts:
        assert (ox, oy) == (width // 2, 0), "Result text is center/top anchored"
        lines = [pack.render_text(manifest, pages, line) for line in texts[ident].split("\n")]
        expected = pack.Image.new("RGBA", (width, height))
        for row, line in enumerate(lines):
            expected.alpha_composite(line, ((width - line.width) // 2, row * 12))
    else:
        sprite = manifest["resources"]["sprites"][ident]
        expected = pack.frame_canvas(sprite, sprite["frames"][frame],
            manifest["resources"]["texturePageItems"], pages)
        assert (ox, oy) == (sprite["originX"], sprite["originY"])
    assert pack.decode_planar(data[offset:offset + length], width, height, stride) == [
        pack.pixel_to_pen(pixel) for pixel in expected.get_flattened_data()]
assert len(records) == 96 and {(ident, 0) for ident in texts} <= records.keys()
for area in range(1, 5):
    for level in range(1, 4):
        assert texts[64500 + (area - 1) * 3 + level - 1] == f"{area}-{level}\n{texts[64000]}"
sprites = [524, 528, 512, 520, 518, 521, 522, 506, 507, 508, 509, 510,
           511, 516, 523, 525, 529, 530, 531, 532, 533, 534, 535]
bundles = [records[sprite, 0][-1] + records[64100 + i, 0][-1] + records[64300 + i, 0][-1]
           for i, sprite in [*enumerate(sprites), (100, 152)]]
# Conservative bound: ignore deduplication and reserve four distinct bundles.
area_sprites = [51, 74, 72, 70, 55]
borders = max(sum(records[sprite, frame][-1] for sprite in area_sprites[area:area + 2]
                  for frame in (7, 13)) for area in range(4))
titles = max(records[ident, 0][-1] for ident in range(64500, 64512))
bound = (sum(sorted(bundles, reverse=True)[:4]) + titles + records[64001, 0][-1]
         + borders + pack.HEADER.size + 18 * pack.RECORD.size)
assert bound <= 32140, "four offered upgrade bundles must fit the existing retired-art bank"
print(f"Clear pack: {len(records)} source-verified records; selected-offer bank bound {bound} bytes")
