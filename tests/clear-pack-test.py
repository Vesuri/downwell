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
        if frame >= 256:
            expected_geometry = (expected_geometry[0], 9*16+expected_geometry[1]-16,
                                 *expected_geometry[2:])
        assert (width, height, ox, oy) == expected_geometry, "circular border cache geometry contract"
    assert flags == 3 and (ident, frame) not in records
    records[ident, frame] = record
    if ident in texts:
        lines = [pack.render_text(manifest, pages, line) for line in texts[ident].split("\n")]
        base_width = max(line.width for line in lines)
        expected = pack.Image.new("RGBA", (base_width, 10+12*(len(lines)-1)))
        for row, line in enumerate(lines):
            expected.alpha_composite(line, ((base_width - line.width) // 2, row * 12))
        base = expected
        if frame == 1:
            assert ident not in (64000, 64001) and not 64500 <= ident < 64512
            expected = pack.Image.new("RGBA", (base.width+2, base.height+2))
            black = pack.Image.new("RGBA", base.size, (0, 0, 0, 255))
            black.putalpha(base.getchannel("A"))
            for xy in ((0, 1), (2, 1), (1, 0), (1, 2)):
                expected.alpha_composite(black, xy)
            expected.alpha_composite(base, (1, 1))
            assert (ox, oy) == (base.width//2+1, 1)
        elif frame:
            assert (ident == 64001 or 64500 <= ident < 64512) and 2 <= frame <= 7
            extent = frame-1
            expected = pack.Image.new("RGBA", (base.width+extent, base.height+extent))
            red = pack.Image.new("RGBA", base.size, (255, 0, 0, 255))
            red.putalpha(base.getchannel("A"))
            for shadow_offset in range(extent, 0, -1):
                expected.alpha_composite(red, (shadow_offset, shadow_offset))
            expected.alpha_composite(base, (0, 0))
            assert (ox, oy) == (base.width//2+extent, extent)
        else:
            assert (ox, oy) == (base.width//2, 0), "Result text is center/top anchored"
    else:
        sprite = manifest["resources"]["sprites"][ident]
        expected = pack.frame_canvas(sprite, sprite["frames"][frame%256],
            manifest["resources"]["texturePageItems"], pages)
        if frame >= 256:
            assert ident in (51, 74, 72, 70, 55) and frame in (263, 269)
            tile = expected
            expected = pack.Image.new("RGBA", (width, height))
            for row in range(9):
                expected.alpha_composite(tile, (0, row*16))
        assert (ox, oy) == (sprite["originX"], sprite["originY"])
    assert expected.size == (width, height)
    assert pack.decode_planar(data[offset:offset + length], width, height, stride) == [
        pack.pixel_to_pen(pixel) for pixel in expected.get_flattened_data()]
expected_keys = {(ident, frame) for ident, frames in pack.SELECTIONS["clear"].items() for frame in frames}
expected_keys |= {(ident, frame) for ident in (51, 74, 72, 70, 55) for frame in (263, 269)}
expected_keys |= {(ident, 0) for ident in texts}
expected_keys |= {(ident, 1) for ident in texts if ident not in (64000, 64001) and not 64500 <= ident < 64512}
expected_keys |= {(ident, frame) for ident in (64001, *range(64500, 64512)) for frame in range(2, 8)}
assert records.keys() == expected_keys
for upgrade in range(23):
    assert texts[64600 + upgrade] == texts[64100 + upgrade] + " 2"
for area in range(1, 5):
    for level in range(1, 4):
        assert texts[64500 + (area - 1) * 3 + level - 1] == f"{area}-{level}\n{texts[64000]}"
sprites = [524, 528, 512, 520, 518, 521, 522, 506, 507, 508, 509, 510,
           511, 516, 523, 525, 529, 530, 531, 532, 533, 534, 535]
bundles = [records[sprite, 0][-1] + max(records[64100 + i, 0][-1],
           records[64600 + i, 0][-1] if i < 23 else 0) + records[64300 + i, 0][-1]
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
