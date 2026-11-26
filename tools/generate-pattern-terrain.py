#!/usr/bin/env python3
"""Offline cavern wall rows, keyed by the exact live autotile neighbourhood.

This is artwork, not generated gameplay: uncertain cells and unlisted seams
always retain the live painter. Platforms, boxes and decorations overlay it.
"""
import hashlib
import itertools
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def neighbourhoods(patterns):
    count = struct.unpack_from('<I', patterns, 12)[0]
    rows = struct.unpack_from('<I', patterns, 28)[0]
    row_base = 32 + count * 20
    cell_base = row_base + rows * 8
    sequences = []
    corridor = (1 << 1) | (1 << 11)
    for i in range(count):
        family, _, _, height, first, _, _ = struct.unpack_from('<HHHHIII', patterns, 32+i*20)
        if family > 5:
            continue
        for reverse in (False, True):
            sequence = []
            for y in range(height):
                offset, width = struct.unpack_from('<II', patterns, row_base+(first+y)*8)
                cells = patterns[cell_base+offset:cell_base+offset+width]
                fixed = corridor
                optional = []
                for x, symbol in enumerate(cells):
                    bit = 11-(x+1) if reverse else x+2
                    if not 0 <= bit < 13:
                        continue
                    if symbol in b'1|':
                        fixed |= 1 << bit
                    elif symbol == ord('2'):
                        optional.append(1 << bit)
                # The live fallback covers patterns with a larger random domain.
                values = {fixed}
                if len(optional) <= 6:
                    for bit in optional:
                        values |= {value | bit for value in list(values)}
                sequence.append(values)
            sequences.append(sequence)
    firsts = {corridor, 0}
    lasts = {corridor, 0}
    for sequence in sequences:
        if sequence:
            firsts.update(sequence[0])
            lasts.update(sequence[-1])
    keys = set()
    for sequence in sequences + [[{corridor}]]:
        for i, middle in enumerate(sequence):
            above = sequence[i-1] if i else lasts
            below = sequence[i+1] if i+1 < len(sequence) else firsts
            for a, m, b in itertools.product(above, middle, below):
                # Diagonals and neighbours of absent cells cannot affect pixels.
                keys.add((m, a & m & 0xffe, b & m & 0xffe))
    return sorted(keys)


def wall_frames(pack):
    magic, version, directory, count = struct.unpack_from('>4sHHI', pack)
    assert magic == b'DWAP' and version == 2
    frames = {}
    for i in range(count):
        sprite, frame, ox, oy, w, h, stride, flags, offset, size = struct.unpack_from('>HHhhHHHHII', pack, directory+24*i)
        if sprite == 51 and frame < 26:
            assert (ox, oy, w, h, stride, flags) == (8, 8, 16, 16, 16, 3)
            frames[frame] = struct.unpack_from('>32H', pack, offset)
    assert len(frames) == 26
    return frames


def tile_groups(patterns, frames):
    """Four adjacent tile glyphs, including every deletion/box-colour choice.

    The wall layer remains separate. Runtime admits this OR composition only
    where the live tile cells do not overlap walls or another tile instance.
    """
    count = struct.unpack_from('<I', patterns, 12)[0]
    rows = struct.unpack_from('<I', patterns, 28)[0]
    row_base = 32 + count * 20
    cell_base = row_base + rows * 8
    codes = {0}
    for i in range(count):
        family, _, _, height, first, _, _ = struct.unpack_from('<HHHHIII', patterns, 32+i*20)
        if family > 5:
            continue
        for reverse in (False, True):
            for y in range(height):
                offset, width = struct.unpack_from('<II', patterns, row_base+(first+y)*8)
                cells = patterns[cell_base+offset:cell_base+offset+width]
                choices = [(0,)] * 12
                for x, symbol in enumerate(cells):
                    cell = 9-x if reverse else x+1
                    if 0 <= cell < 11:
                        choices[cell] = ((0,1,2,3) if symbol in b'B3' else
                                         (0,4) if symbol == ord('-') else (0,))
                for start in (0,4,8):
                    for digits in itertools.product(*choices[start:start+4]):
                        codes.add(sum(digit * 5**j for j, digit in enumerate(digits)))
    directory = [65535] * 625
    payload = bytearray()
    for index, code in enumerate(sorted(codes)):
        directory[code] = index
        pixels = [[0]*10 for _ in range(16)]
        for cell in range(4):
            digit = (code // 5**cell) % 5
            if not digit:
                continue
            frame = (16,17,22,23)[digit-1]
            for y in range(16):
                for plane in range(2):
                    value = frames[frame][plane*16+y]
                    for x in range(16):
                        if value & (0x8000 >> x):
                            px = 6+cell*16+x
                            pixels[y][plane*5+px//16] |= 0x8000 >> (px & 15)
        for row in pixels:
            payload += struct.pack('>10H', *row)
    return len(codes), struct.pack('>625H', *directory) + payload


def render(key, frames):
    middle, above, below = key
    result = [[0] * 22 for _ in range(16)]
    for cell in range(11):
        bit = 2 << cell
        if not middle & bit:
            continue
        frame = (bool(above & bit) | (bool(middle & (bit << 1)) << 1) |
                 (bool(below & bit) << 2) | (bool(middle & (bit >> 1)) << 3))
        for y in range(16):
            for plane in range(2):
                value = frames[frame][y+plane*16]
                for pixel in range(16):
                    x = 102+cell*16+pixel
                    if 107 <= x < 272 and value & (0x8000 >> pixel):
                        result[y][plane*11+(x-96)//16] |= 0x8000 >> ((x-96) & 15)
    return [tuple(row) for row in result]


def overlays(pack):
    _, _, directory, count = struct.unpack_from('>4sHHI', pack)
    records, payload = [], bytearray()
    for i in range(count):
        sprite, frame, ox, oy, w, h, stride, flags, offset, size = struct.unpack_from('>HHhhHHHHII', pack, directory+24*i)
        if sprite not in (139, 550, 551, 568, 569, 570):
            continue
        assert flags == 3 and w <= 32 and h <= 32
        for flip in (0, 1):
            origin = w-ox if flip else ox
            phase = (110-origin) & 15
            words = (phase+w+15)//16
            records.append((sprite, frame, flip, origin, oy, words, h, phase, len(payload)))
            for y in range(h):
                planes = [[0]*words for _ in range(2)]
                for plane in range(2):
                    for x in range(w):
                        source_x = w-1-x if flip else x
                        if pack[offset+(plane*h+y)*(stride//8)+source_x//8] & (128 >> (source_x & 7)):
                            planes[plane][(phase+x)//16] |= 0x8000 >> ((phase+x)&15)
                for low, high in zip(*planes):
                    payload += struct.pack('>HHH', low | high, low, high)
    return records, payload


def main():
    inputs = ['generated/amiga-game.dwap', 'generated/downwell-builder-patterns.dwbp']
    pack, patterns = [(ROOT / path).read_bytes() for path in inputs]
    frames = wall_frames(pack)
    keys = neighbourhoods(patterns)
    pixels = []
    for key in keys:
        pixels.extend(render(key, frames))
    assert len(keys) < 65536 and len(pixels) < 65536
    records, payload = overlays(pack)
    group_count, groups = tile_groups(patterns, frames)
    output = bytearray(struct.pack('>4sHHHHH', b'DWPT', 4, len(keys), len(pixels), len(records), group_count))
    for i, key in enumerate(keys):
        output += struct.pack('>4H', *key, i*16)
    for row in pixels:
        output += struct.pack('>22H', *row)
    for record in records:
        output += struct.pack('>8HI', *record)
    output += payload
    output += groups
    path = ROOT / 'generated/amiga-pattern-terrain.bin'
    path.write_bytes(output)
    sha = lambda data: hashlib.sha256(data).hexdigest()
    provenance = json.loads((ROOT / 'generated/amiga-game.packing.json').read_text())
    assert sha(pack) == provenance['packSha256']
    manifest = dict(generator='generate-pattern-terrain.py/v4', generatorSha256=sha(Path(__file__).read_bytes()),
                    sourceSha256=provenance['sourceSha256'], extractor=provenance['extractor'],
                    inputs={name: sha((ROOT/name).read_bytes()) for name in inputs},
                    outputSha256=sha(output), bytes=len(output), neighbourhoods=len(keys),
                    pixelRows=len(pixels), overlays=len(records), tileGroups=group_count,
                    format='DWPT v4: 14-byte header; sorted 3-word keys and first-row index; contiguous 16-row planar strips; aligned masked overlays; 625-entry tile-group directory and 320-byte strips')
    path.with_suffix('.json').write_text(json.dumps(manifest, indent=2)+'\n')
    print(f'Prepared {len(keys)} contiguous wall strips, {group_count} tile groups: {len(output)} bytes')


if __name__ == '__main__':
    main()
