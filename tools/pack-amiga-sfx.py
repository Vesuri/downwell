#!/usr/bin/env python3
"""W2.4: pack the W2.2 Paula sound bank into one file the Amiga mixer reads.

Input: extracted/amiga-sfx/bank/ (tools/sfx-bank.py --bank: signed 8-bit
samples and their manifest) and extracted/audio/manifest.json (each sound's
asset volume). Output, all git-ignored derived data:

  generated/amiga-sfx.bin   the DWSX pack (big-endian):
      0  'DWSX'
      4  u16 version (2)
      6  u16 count (at most 255)
      8  u32 total bytes
     12  u32 data offset
     16  u16 index size (highest id + 1)
     18  14 bytes of zero
     32  count x 16-byte entries, ascending id:
           u16 id, u16 period, u8 Paula volume (0-64, asset volume and
           peak normalisation included), u8 flags (bit 0: the whole sample
           loops), u16 asset gain (Q15, the asset volume GameMaker reports),
           u32 sample offset from the data start, u32 sample bytes
     index one byte per id below the index size: 0 for no sound, else the
           entry number + 1 (padded to a whole word)
     data  the samples, each a whole number of words
  Entries sit on 16-byte boundaries so the 68000 reads their words directly.
  generated/amiga-sfx-<bank>.bin  the same format, one pack per scene bank
      (W2.5), each sound in exactly one, except that a front-end sound the run
      also plays is in both run and front (W2.6; the two are never loaded
      together):
        run      every sound of gameplay, shops or pause (kept for the run,
                 also through the clear screen and results)
        front    the front end's (title, garden, style select, menus)
        results  the results screen's own, loaded over the run bank
        clear    the clear screen's own, loaded over the run bank
  generated/amiga-sfx.h     the byte and sound counts of every pack
  generated/amiga-sfx.json  tool version, input hashes and a per-sound table

Every one-shot must play for more than one PAL field at its highest pitch:
the mixer silences a one-shot by rewriting the channel's reload registers in
the VBlank after its start.
"""
import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

VERSION = "1.3.0"
ROOT = Path(__file__).resolve().parents[1]
BANK = ROOT / "extracted/amiga-sfx/bank"
AUDIO = ROOT / "extracted/audio/manifest.json"
OUT_BIN = ROOT / "generated/amiga-sfx.bin"
OUT_H = ROOT / "generated/amiga-sfx.h"
OUT_JSON = ROOT / "generated/amiga-sfx.json"
SCENE_BANKS = ("run", "front", "results", "clear")
RUN_SCENES = {"gameplay", "shop", "pause"}
PAULA_CLOCK = 3546895
FIELD_CLOCKS = PAULA_CLOCK / 50
MIN_PERIOD = 124


def sha(data):
    return hashlib.sha256(data).hexdigest()


def build():
    manifest_bytes = (BANK / "manifest.json").read_bytes()
    audio_bytes = AUDIO.read_bytes()
    manifest = json.loads(manifest_bytes)
    volumes = {s["id"]: s["volume"] for s in json.loads(audio_bytes)["sounds"]}
    rows = sorted(manifest["sounds"], key=lambda r: r["id"])
    samples = {}
    for r in rows:
        raw = (BANK / f"{r['id']:03d}_{r['name']}.raw").read_bytes()
        if sha(raw) != r["sha256"]:
            sys.exit(f"{r['id']} {r['name']}: sample differs from the bank manifest")
        if len(raw) < 2 or len(raw) & 1 or len(raw) > 131070:
            sys.exit(f"{r['id']} {r['name']}: {len(raw)} bytes is not 1..65535 words")
        if not 0 <= r["paulaVolume"] <= 64:
            sys.exit(f"{r['id']}: Paula volume {r['paulaVolume']}")
        if not 0 < round(volumes[r["id"]] * 32768) <= 65535:
            sys.exit(f"{r['id']}: asset volume {volumes[r['id']]} outside (0, 2)")
        fields = len(raw) * max(MIN_PERIOD, r["period"] / r["maxPitch"]) / FIELD_CLOCKS
        if not r["loop"] and fields <= 1.2:
            sys.exit(f"{r['id']} {r['name']}: plays {fields:.2f} fields at pitch {r['maxPitch']}")
        samples[r["id"]] = raw

    def banks_of(r):
        scenes = set(r["scenes"])
        if scenes & RUN_SCENES:
            return ("run", "front") if "front" in scenes else ("run",)
        if "front" in scenes:
            if scenes != {"front"}:
                sys.exit(f"{r['id']} {r['name']}: front-end sound also in {sorted(scenes - {'front'})}")
            return ("front",)
        if scenes == {"results"}:
            return ("results",)
        if scenes == {"clear"}:
            return ("clear",)
        sys.exit(f"{r['id']} {r['name']}: no scene bank for {sorted(scenes)}")

    def pack(chosen):
        entries, data, table = [], bytearray(), []
        for r in chosen:
            raw = samples[r["id"]]
            gain = round(volumes[r["id"]] * 32768)
            entries.append(struct.pack(">HHBBHII", r["id"], r["period"], r["paulaVolume"],
                                       1 if r["loop"] else 0, gain, len(data), len(raw)))
            table.append({"id": r["id"], "name": r["name"], "period": r["period"],
                          "volume": r["paulaVolume"], "loop": r["loop"], "assetGainQ15": gain,
                          "offset": len(data), "bytes": len(raw), "scenes": r["scenes"]})
            data += raw
        if not entries or len(entries) > 255:
            sys.exit(f"{len(entries)} sounds: a pack holds 1..255")
        index_size = chosen[-1]["id"] + 1
        index = bytearray(index_size + (index_size & 1))
        for n, r in enumerate(chosen):
            index[r["id"]] = n + 1
        header_bytes = 32 + 16 * len(entries) + len(index)
        total = header_bytes + len(data)
        blob = (struct.pack(">4sHHIIH14x", b"DWSX", 2, len(entries), total, header_bytes, index_size)
                + b"".join(entries) + bytes(index) + data)
        assert len(blob) == total
        return blob, table

    blob, table = pack(rows)
    for t in table:
        t["banks"] = list(banks_of(t))
    banks = {}
    for name in SCENE_BANKS:
        banks[name], _ = pack([r for r in rows if name in banks_of(r)])
    total = len(blob)
    meta = {"schemaVersion": 1, "tool": {"name": "pack-amiga-sfx.py", "version": VERSION},
            "inputs": {"bank": {"path": "extracted/amiga-sfx/bank/manifest.json", "sha256": sha(manifest_bytes)},
                       "audio": {"path": "extracted/audio/manifest.json", "sha256": sha(audio_bytes)}},
            "output": {"path": "generated/amiga-sfx.bin", "bytes": total, "sha256": sha(blob)},
            "banks": {name: {"path": f"generated/amiga-sfx-{name}.bin", "bytes": len(b), "sha256": sha(b),
                             "sounds": sum(1 for t in table if name in t["banks"])}
                      for name, b in banks.items()},
            "sounds": table}
    header = (f"// Generated by tools/pack-amiga-sfx.py {VERSION}; pack sha256 {sha(blob)[:16]}.\n"
              "#ifndef DOWNWELL_GENERATED_AMIGA_SFX_H\n#define DOWNWELL_GENERATED_AMIGA_SFX_H\n"
              f"#define DOWNWELL_SFX_BANK_BYTES {total}u\n#define DOWNWELL_SFX_BANK_SOUNDS {len(table)}u\n"
              + "".join(f"#define DOWNWELL_SFX_{name.upper()}_BYTES {len(b)}u\n" for name, b in banks.items())
              + "#endif\n")
    return blob, banks, header, json.dumps(meta, indent=1) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    blob, banks, header, meta = build()
    outputs = ([(OUT_BIN, blob)] + [(ROOT / f"generated/amiga-sfx-{n}.bin", b) for n, b in banks.items()]
               + [(OUT_H, header.encode()), (OUT_JSON, meta.encode())])
    if args.check:
        stale = [p for p, b in outputs if not p.exists() or p.read_bytes() != b]
        if stale:
            sys.exit("stale: " + ", ".join(str(p.relative_to(ROOT)) for p in stale))
        print(f"{OUT_BIN.relative_to(ROOT)} verified")
        return
    OUT_BIN.parent.mkdir(exist_ok=True)
    for p, b in outputs:
        if not p.exists() or p.read_bytes() != b:
            p.write_bytes(b)
    print(f"{OUT_BIN.relative_to(ROOT)}: {len(blob):,} bytes; "
          + ", ".join(f"{n} {len(b):,}" for n, b in banks.items()))


if __name__ == "__main__":
    main()
