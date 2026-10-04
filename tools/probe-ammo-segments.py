#!/usr/bin/env python3
"""Measure an offline final-column segment bank, without changing the runtime.

At fixed capacity, rows between adjacent charge thresholds share every future
target. Their initial states are equal, so their animation states remain equal.
An eight-row sprite overlaps at most the preceding segment for capacities <=16.
Store final words for that state pair; deduplicate tiles and 16-entry pages.
"""
import hashlib
import json
import math
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sha = lambda b: hashlib.sha256(b).hexdigest()


def main():
    pack = (ROOT / "generated/amiga-hud.dwap").read_bytes()
    provenance = json.loads((ROOT / "generated/amiga-hud.packing.json").read_text())
    assert sha(pack) == provenance["packSha256"]
    magic, version, directory, count = struct.unpack_from(">4sHHI", pack)
    assert (magic, version) == (b"DWAP", 2)
    stamps = {}
    for i in range(count):
        sprite, frame, ox, oy, width, height, stride, flags, offset, size = struct.unpack_from(
            ">HHhhHHHHII", pack, directory + 24*i)
        if sprite not in (418, 419, 421):
            continue
        assert (ox, oy, width, height, stride) == (0, 0, 12, 8, 16)
        assert size >= 32 and offset + size <= len(pack)
        stamps[sprite, frame] = tuple((struct.unpack_from(">H", pack, offset+2*y)[0],
                                      struct.unpack_from(">H", pack, offset+16+2*y)[0]) for y in range(8))

    def overlay(column, words, y, first):
        for i, (low, high) in enumerate(words):
            target = y+i-first
            if 0 <= target < len(column):
                mask = (low | high) & 0xfff0
                a, b = column[target]
                column[target] = ((a & ~mask) | (low & mask), (b & ~mask) | (high & mask))

    def dividers(ammo, cap):
        out = [(0, 0)] * 284
        spacing, fill = (1.0/cap)*184.0, (float(ammo)/cap)*184.0
        levels, level = [], 184.0
        while level > 0:
            levels.append(level)
            level -= spacing
        def draw(frame, y):
            overlay(out, stamps[421, frame], math.floor(y), 0)
        for level in levels:
            if level < fill:
                draw(1, 229.0-level)
        for level in levels:
            y = 229.0-level
            if level < fill:
                draw(0, y)
                draw(1, y+1.0)
                draw(1, y-1.0)
            elif level == fill or level == 1.0:
                draw(1, y+1.0)
        return out

    tiles, tile_ids, pages, page_ids = [], {}, [], {}
    configurations, capacities = [], []
    for cap in range(1, 17):
        bounds = [math.ceil((float(a)/cap)*184.0) for a in range(cap+1)]
        assert all(b-a >= 8 for a, b in zip(bounds, bounds[1:]))
        first_config = len(configurations)
        for ammo in range(cap+1):
            fill = (float(ammo)/cap)*184.0
            fill_top, fill_height = ((232, math.floor(5-fill)) if 0 < fill < 5 else
                                    (math.floor(237-fill), math.floor(fill-5)) if ammo else (232, 0))
            divider = dividers(ammo, cap)
            for meter in range(4):
                base = 229+4*meter
                for segment, (lo, hi) in enumerate(zip(bounds, bounds[1:])):
                    first = base-hi+(1 if segment == cap-1 else 8)
                    last = base-lo+8
                    page = []
                    for own in range(4):
                        for previous in range(4):
                            column = [(0xfff0, 0x8010 if fill_top <= y < fill_top+fill_height else 0xfff0)
                                      for y in range(first, last)]
                            for row in range(bounds[segment-1] if segment else 0, hi):
                                y = base-row
                                if y >= 232:
                                    continue
                                frame = own if row >= lo else previous
                                sprite = (419 if row else 421) if row < bounds[ammo] else 418
                                overlay(column, stamps[sprite, frame % 2 if sprite == 421 else frame], y, first)
                            overlay(column, divider, 4*meter, first)
                            tile = tuple(column)
                            if tile not in tile_ids:
                                tile_ids[tile] = len(tiles)
                                tiles.append(tile)
                            page.append(tile_ids[tile])
                    page = tuple(page)
                    if page not in page_ids:
                        page_ids[page] = len(pages)
                        pages.append(page)
                    configurations.append(page_ids[page])
        capacities.append(dict(capacity=cap, boundaries=bounds,
                               firstConfiguration=first_config,
                               configurationCount=len(configurations)-first_config))
    assert len(tiles) < 65536 and len(pages) < 65536
    tile_words = sum(len(tile)*2 for tile in tiles)
    # Tile offset/length directory uses 32-bit byte offsets + 16-bit row counts.
    costs = dict(tileBytes=tile_words*2, tileDirectoryBytes=len(tiles)*6,
                 pageBytes=len(pages)*32, configurationBytes=len(configurations)*2,
                 capacityDirectoryBytes=16*8,
                 boundaryBytes=sum(len(c["boundaries"]) for c in capacities))
    pairs, pair_ids, runs = [], {}, []
    run_count = 0
    for tile in tiles:
        encoded = []
        for pair in tile:
            if pair not in pair_ids:
                pair_ids[pair] = len(pairs)
                pairs.append(pair)
            index = pair_ids[pair]
            if encoded and encoded[-1][1] == index and encoded[-1][0] < 255:
                encoded[-1] = (encoded[-1][0]+1, index)
            else:
                encoded.append((1, index))
        runs.append(encoded)
        run_count += len(encoded)
    assert len(pairs) <= 256
    run_costs = dict(costs)
    run_costs["tileBytes"] = run_count*2
    run_costs["pairDictionaryBytes"] = len(pairs)*4
    # Run offsets/counts fit 16 bits; unlike raw tile byte offsets, these need
    # only four bytes per directory entry. No decompression buffer is needed.
    assert run_count < 65536
    run_costs["tileDirectoryBytes"] = len(tiles)*4
    run_costs["capacityDirectoryBytes"] = 16*2
    run_costs["boundaryBytes"] = 16*17
    lines = ["// Generated copyrighted asset data; do not commit.",
             "namespace downwell_ammo_segments {",
             "static const unsigned short sourceStamps[10][8][2] = {"]
    for sprite, frame in [(s,f) for s in (418,419,421) for f in range(2 if s==421 else 4)]:
        lines.append("{" + ",".join("{%d,%d}" % pair for pair in stamps[sprite,frame]) + "},")
    lines += ["};",
             "static const unsigned short configStart[16] = {" +
             ",".join(str(c["firstConfiguration"]) for c in capacities) + "};",
             "static const unsigned char boundaries[16][17] = {"]
    lines += ["{" + ",".join(map(str, c["boundaries"]+[0]*(17-len(c["boundaries"])))) + "},"
              for c in capacities]
    lines += ["};", "static const unsigned short configurations[] = {",
              ",".join(map(str, configurations)), "};",
              "static const unsigned short pages[][16] = {"]
    lines += ["{" + ",".join(map(str, page)) + "}," for page in pages]
    lines += ["};", "static const unsigned short tiles[][2] = {"]
    offset = 0
    for encoded in runs:
        lines.append("{%d,%d}," % (offset, len(encoded)))
        offset += len(encoded)
    lines += ["};", "static const unsigned short pairs[][2] = {"]
    lines += ["{%d,%d}," % pair for pair in pairs]
    lines += ["};", "static const unsigned char runs[][2] = {"]
    lines += ["{%d,%d}," % run for encoded in runs for run in encoded]
    lines += ["};", "}", ""]
    header = "\n".join(lines).encode()
    (ROOT / "generated/amiga-ammo-segments-probe.h").write_bytes(header)
    run_costs["sourceValidationBytes"] = 320
    manifest = dict(schemaVersion=1, generator="probe-ammo-segments.py/v1",
                    generatorSha256=sha(Path(__file__).read_bytes()),
                    sourceSha256=provenance["sourceSha256"], extractor=provenance["extractor"],
                    inputPackSha256=sha(pack), capacities=capacities,
                    tiles=len(tiles), pages=len(pages), costs=costs,
                    totalBytes=sum(costs.values()), runPairs=len(pairs), runs=run_count,
                    runCosts=run_costs, runTotalBytes=sum(run_costs.values()),
                    outputSha256=sha(header), runtimeEnabled=False)
    output = ROOT / "generated/amiga-ammo-segments-probe.json"
    output.write_text(json.dumps(manifest, indent=2)+"\n")
    print(json.dumps({k: manifest[k] for k in ("tiles", "pages", "costs", "totalBytes",
                                             "runPairs", "runs", "runCosts", "runTotalBytes")}, indent=2))


if __name__ == "__main__":
    main()
