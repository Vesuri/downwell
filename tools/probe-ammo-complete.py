#!/usr/bin/env python3
"""Budget complete, independently selected gauge segments before native use.

Unlike the interior probe, each final tile includes the background, red rim,
caps and precomposed single-digit label. There is no runtime painter implied
by this experiment. Multi-digit labels and nonuniform capacity changes retain
the original renderer; the segment-state invariant has the same scope as the
existing segment probe.
"""
import hashlib
import json
import math
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sha = lambda data: hashlib.sha256(data).hexdigest()


def verify(path):
    bank_path = ROOT / "generated/amiga-ammo-complete-probe.json"
    manifest = json.loads((ROOT / "generated/amiga-ammo-complete-probe.manifest.json").read_text())
    assert sha(bank_path.read_bytes()) == manifest["outputSha256"]
    assert sha((ROOT / "generated/amiga-hud.dwap").read_bytes()) == manifest["inputPackSha256"]
    bank = json.loads(bank_path.read_text())
    checks = 0
    for line in Path(path).read_text().splitlines():
        cap, ammo, meter, offset, states, expected = line.split()
        cap, ammo, meter, offset = map(int, (cap, ammo, meter, offset))
        assert len(states) == cap
        bounds = bank["bounds"][cap-1]
        image = [None]*216
        for segment in range(cap):
            configuration = bank["starts"][cap-1]+((ammo*4+meter)*3+offset)*cap+segment
            page = bank["pages"][bank["configurations"][configuration]]
            tile = page[int(states[segment])*4+(int(states[segment-1]) if segment else 0)]
            start, count = bank["tiles"][tile]
            y = 40 if segment == cap-1 else 229+4*meter-bounds[segment+1]+8
            for length, row in bank["runs"][start:start+count]:
                for _ in range(length):
                    assert image[y-40] is None
                    image[y-40] = bank["rows"][row]
                    y += 1
        value = 14695981039346656037
        for row in image:
            assert row is not None
            for word in row:
                for byte in (word >> 8, word & 255):
                    value = ((value ^ byte)*1099511628211) & 0xffffffffffffffff
        assert value == int(expected, 16), (checks, cap, ammo, meter, offset, states, hex(value), expected)
        checks += 1
    assert checks > 10000
    print(f"{checks} complete-gauge pixel digests match the original ordered renderer")


def main():
    pack = (ROOT / "generated/amiga-hud.dwap").read_bytes()
    provenance = json.loads((ROOT / "generated/amiga-hud.packing.json").read_text())
    assert sha(pack) == provenance["packSha256"]
    magic, version, directory, count = struct.unpack_from(">4sHHI", pack)
    assert (magic, version) == (b"DWAP", 2)
    assets = {}
    for i in range(count):
        sprite, frame, ox, oy, width, height, stride, flags, offset, size = struct.unpack_from(
            ">HHhhHHHHII", pack, directory + 24*i)
        if sprite not in (416, 418, 419, 421, 65010):
            continue
        plane = stride//8*height
        assert size >= plane*2 and offset+size <= len(pack)
        pixels = []
        for y in range(height):
            for x in range(width):
                at, bit = offset+y*(stride//8)+x//8, 7-x%8
                pen = ((pack[at] >> bit) & 1) | (((pack[at+plane] >> bit) & 1) << 1)
                if pen:
                    pixels.append((x-ox, y-oy, pen))
        assets[sprite, frame] = pixels

    def stamp(sprite, frame, x, y):
        # Plane words cover x=272..319. Outside the original single-digit
        # paint clip x=282..319 is deliberately zero and must be preserved.
        words = [[0]*6 for _ in range(284)]
        for dx, dy, pen in assets[sprite, frame]:
            px, py = x+dx, y+dy
            if 282 <= px < 320 and 0 <= py < 284:
                word, bit = (px-272)//16, 1 << (15-px%16)
                if pen & 1:
                    words[py][word] |= bit
                if pen & 2:
                    words[py][word+3] |= bit
        return tuple(tuple(row) for row in words)

    def overlay(rows, art, offset=0):
        for y in range(max(0, -offset), min(len(art), len(rows)-offset)):
            row = art[y]
            target = y+offset
            if 0 <= target < len(rows):
                for word in range(3):
                    low, high = row[word], row[word+3]
                    mask = low | high
                    rows[target][word] = (rows[target][word] & ~mask) | low
                    rows[target][word+3] = (rows[target][word+3] & ~mask) | high

    row_art = {(s, f): stamp(s, f, 288, 0)[:8]
               for s in (418, 419, 421) for f in range(2 if s == 421 else 4)}
    bases, rims = [], []
    for meter in range(4):
        base = [[0]*6 for _ in range(284)]
        for y in range(40, 256):
            base[y] = [0x003f, 0xffff, 0xc000]*2  # original x=282..305 clear
        overlay(base, stamp(416, 0, 284, 43+meter))
        bases.append(base)
        rims.append(stamp(416, 1, 284, 43+meter))
    labels = {(ammo, offset): stamp(65010, ammo, 290+offset, 239+offset)
              for ammo in range(10) for offset in range(3)}
    tiles, tile_ids, pages, page_ids, configurations, starts, bounds_all = [], {}, [], {}, [], [], []
    for cap in range(1, 17):
        starts.append(len(configurations))
        bounds = [math.ceil(float(a)/cap*184.0) for a in range(cap+1)]
        bounds_all.append(bounds)
        for ammo in range(min(cap, 9)+1):
            fill = float(ammo)/cap*184.0
            fill_top, fill_height = ((232, math.floor(5-fill)) if 0 < fill < 5 else
                                    (math.floor(237-fill), math.floor(fill-5)) if ammo else (232, 0))
            for meter in range(4):
                base_y = 229+4*meter
                base = [list(row) for row in bases[meter]]
                for y in range(fill_top, fill_top+fill_height):
                    base[y][1] |= 0x7fe0
                    base[y][4] &= ~0x7fe0
                divider = [[0]*6 for _ in range(284)]
                levels, level, spacing = [], 184.0, (1.0/cap)*184.0
                while level > 0:
                    levels.append(level)
                    level -= spacing
                for level in levels:
                    if level < fill:
                        overlay(divider, row_art[421, 1], math.floor(base_y-level))
                for level in levels:
                    y = base_y-level
                    if level < fill:
                        for frame, dy in ((0, 0), (1, 1), (1, -1)):
                            overlay(divider, row_art[421, frame], math.floor(y+dy))
                    elif level == fill or level == 1.0:
                        overlay(divider, row_art[421, 1], math.floor(y+1))
                for offset in range(3):
                    for segment, (lo, hi) in enumerate(zip(bounds, bounds[1:])):
                        first = 40 if segment == cap-1 else base_y-hi+8
                        last = 256 if not segment else base_y-lo+8
                        page = []
                        for own in range(4):
                            for previous in range(4):
                                column = [list(row) for row in base[first:last]]
                                for row in range(bounds[segment-1] if segment else 0, hi):
                                    y = base_y-row
                                    if y >= 232:
                                        continue
                                    frame = own if row >= lo else previous
                                    sprite = (419 if row else 421) if row < bounds[ammo] else 418
                                    overlay(column, row_art[sprite, frame % 2 if sprite == 421 else frame], y-first)
                                overlay(column, divider, -first)
                                overlay(column, rims[meter], -first)
                                overlay(column, labels[ammo, offset], -first)
                                tile = tuple(tuple(row) for row in column)
                                if tile not in tile_ids:
                                    tile_ids[tile] = len(tiles)
                                    tiles.append(tile)
                                page.append(tile_ids[tile])
                        page = tuple(page)
                        if page not in page_ids:
                            page_ids[page] = len(pages)
                            pages.append(page)
                        configurations.append(page_ids[page])
        print(f"capacity {cap}: {len(tiles)} tiles, {len(pages)} selector pages", flush=True)

    pairs, pair_ids, runs, directory = [], {}, [], []
    for tile in tiles:
        encoded = []
        for row in tile:
            if row not in pair_ids:
                pair_ids[row] = len(pairs)
                pairs.append(row)
            index = pair_ids[row]
            if encoded and encoded[-1][1] == index and encoded[-1][0] < 255:
                encoded[-1] = (encoded[-1][0]+1, index)
            else:
                encoded.append((1, index))
        directory.append((len(runs), len(encoded)))
        runs.extend(encoded)
    assert max(len(tiles), len(pages), len(pairs)) < 65536
    # Packed records, no implied C struct padding. Decode cost is NOT free.
    costs = dict(runs=len(runs)*3, directory=len(tiles)*6, rows=len(pairs)*12,
                 pages=len(pages)*32, configurations=len(configurations)*2,
                 starts=16*4, bounds=16*17)
    blocks, block_indices = set(), 0
    for tile in tiles:
        ids = [pair_ids[row] for row in tile]
        for first in range(0, len(ids), 8):
            blocks.add(tuple(ids[first:first+8]))
            block_indices += 1
    blocked_costs = dict(costs)
    blocked_costs["runs"] = sum(map(len, blocks))*2+block_indices*2+len(blocks)*6
    payload = dict(starts=starts, bounds=bounds_all, configurations=configurations,
                   pages=pages, tiles=directory, runs=runs, rows=pairs)
    output = json.dumps(payload, separators=(",", ":")).encode()
    (ROOT / "generated/amiga-ammo-complete-probe.json").write_bytes(output)
    manifest = dict(schemaVersion=1, generator="probe-ammo-complete.py/v1",
                    generatorSha256=sha(Path(__file__).read_bytes()),
                    sourceSha256=provenance["sourceSha256"], extractor=provenance["extractor"],
                    inputPackSha256=sha(pack), outputSha256=sha(output),
                    tiles=len(tiles), rows=len(pairs), costs=costs,
                    totalBytes=sum(costs.values()), blockedCosts=blocked_costs,
                    blockedTotalBytes=sum(blocked_costs.values()), runtimeEnabled=False)
    (ROOT / "generated/amiga-ammo-complete-probe.manifest.json").write_text(json.dumps(manifest, indent=2)+"\n")
    print(json.dumps(manifest, indent=2))


if __name__ == "__main__":
    if len(sys.argv) == 3 and sys.argv[1] == "--verify":
        verify(sys.argv[2])
    else:
        assert len(sys.argv) == 1
        main()
