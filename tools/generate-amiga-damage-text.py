#!/usr/bin/env python3
"""Precompose common moving damage labels; retain runtime fallback for other integers."""
import argparse
import hashlib
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / 'generated/amiga-damage-text.h'
INPUTS = ['data/data.win', 'generated/amiga-game.dwap',
          'generated/amiga-hud.manifest.json', 'generated/amiga-game.packing.json',
          'tools/generate-amiga-damage-text.py']


def generate():
    pack = (ROOT / 'generated/amiga-game.dwap').read_bytes()
    magic, version, _, count, _ = struct.unpack_from('>4sHHI8s', pack)
    assert magic == b'DWAP' and version == 2
    records = [struct.unpack_from('>HHhhHHHHII', pack, 20 + i * 24)
               for i in range(count)]
    glyphs = []
    for sprite in range(65100, 65111):
        matches = [r for r in records if r[:2] == (sprite, 0)]
        assert len(matches) == 1
        r = matches[0]
        assert r[2:7] == (4, 0, 8, 10, 16)
        offset = r[8]
        assert offset + 40 <= len(pack)
        glyphs.append([pack[offset + y * 2] | pack[offset + 20 + y * 2]
                       for y in range(10)])
    lines = ['// Generated offline; source hashes and extractor in companion JSON.',
             'static const AmigaDamageGlyphs preparedDamageGlyphs = {{']
    lines += ['{' + ','.join(map(str, row)) + '},' for row in glyphs]
    lines += ['}};', 'static const AmigaPreparedDamageText preparedDamageText[10] = {']
    for value in range(10):
        # Two glyphs, centred on x: source prefix "-", then the digit.
        ink = [[0] * 48 for _ in range(12)]
        for digit, start in [(10, 16), (value, 24)]:
            for y, byte in enumerate(glyphs[digit], 1):
                for x in range(8):
                    ink[y][start+x] = bool(byte & (128 >> x))
        mask = [[any(0 <= y+dy < 12 and 0 <= x+dx < 48 and ink[y+dy][x+dx]
                     for dx, dy in [(0, 0), (-1, 0), (1, 0), (0, -1), (0, 1)])
                 for x in range(48)] for y in range(12)]
        def words(bitmap):
            return [[sum(int(bitmap[y][w*16+x]) << (15-x) for x in range(16))
                     for w in range(3)] for y in range(12)]
        planes = [words(mask), words(ink)]
        used = [x for x in range(3) if any(row[x] for row in planes[0])]
        first = min(used) if used else 0
        length = max(used)-first+1 if used else 0
        lines.append('{' + f'{first},{length},' + ','.join(
            '{' + ','.join('{' + ','.join(map(str, row)) + '}' for row in plane) + '}'
            for plane in planes) + '},')
    lines.append('};')
    return ('\n'.join(lines) + '\n').encode()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    payload = generate()
    manifest = {
        'format': 'amiga-damage-text-words-v1', 'generator_version': 1,
        'extractor': json.loads((ROOT / INPUTS[2]).read_text())['extractor'],
        'derived_resource_ids': list(range(65100, 65111)), 'values': list(range(10)),
        'inputs_sha256': {p: hashlib.sha256((ROOT / p).read_bytes()).hexdigest() for p in INPUTS},
        'output_sha256': hashlib.sha256(payload).hexdigest()}
    for path, data in [(OUTPUT, payload), (OUTPUT.with_suffix('.json'),
            (json.dumps(manifest, indent=2) + '\n').encode())]:
        if args.check:
            if not path.exists() or path.read_bytes() != data:
                raise SystemExit(f'stale damage bank: {path}')
        else:
            temporary = path.with_suffix(path.suffix + '.tmp')
            temporary.write_bytes(data)
            temporary.replace(path)


if __name__ == '__main__':
    main()
