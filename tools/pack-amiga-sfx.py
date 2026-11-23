#!/usr/bin/env python3
"""W2.4: the Paula sound banks the Amiga mixer reads, as the converter makes them.

WI.4: the installed banks are built by DownwellInstallData's audio stage
(tools/install-data/audio.c: Tremor for the Ogg effects, our own resampler,
integer cuts and normalisation), whose samples are the reference since
2026-11-23. This tool runs the host converter with only that stage and
writes, all git-ignored derived data:

  generated/amiga-sfx-<bank>.bin  the converter's packs, one per scene bank
      (W2.5), each sound in exactly one, except that a front-end sound the run
      also plays is in both run and front (W2.6; the two are never loaded
      together):
        run      every sound of gameplay, shops or pause (kept for the run,
                 also through the clear screen and results)
        front    the front end's (title, garden, style select, menus)
        results  the results screen's own, loaded over the run bank
        clear    the clear screen's own, loaded over the run bank
  generated/amiga-sfx.bin   every sound in one pack, for the Paula probe
  generated/amiga-sfx.h     the byte and sound counts of every pack
  generated/amiga-sfx.json  tool version, input hashes and a per-sound table

The DWSX pack (big-endian):
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
The converter checks that every one-shot plays for more than one PAL field
at its highest pitch: the mixer silences a one-shot by rewriting the
channel's reload registers in the VBlank after its start.
"""
import argparse
import hashlib
import json
import os
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

VERSION = "2.0.0"
ROOT = Path(__file__).resolve().parents[1]
CONVERTER = ROOT / "build/install-data/DownwellInstallData"
OUT_BIN = ROOT / "generated/amiga-sfx.bin"
OUT_H = ROOT / "generated/amiga-sfx.h"
OUT_JSON = ROOT / "generated/amiga-sfx.json"
SCENE_BANKS = ("run", "front", "results", "clear")


def sha(data):
    return hashlib.sha256(data).hexdigest()


def parse(blob):
    magic, version, count, total, header, index_size = struct.unpack_from(">4sHHIIH", blob)
    if magic != b"DWSX" or version != 2 or total != len(blob):
        sys.exit("the converter wrote a malformed sound bank")
    out = []
    for n in range(count):
        ident, period, volume, flags, gain, offset, size = struct.unpack_from(">HHBBHII", blob, 32 + 16 * n)
        out.append({"id": ident, "period": period, "volume": volume, "loop": bool(flags & 1),
                    "assetGainQ15": gain, "data": blob[header + offset:header + offset + size]})
    return out


def pack(entries):
    data, rows = bytearray(), []
    for e in entries:
        rows.append(struct.pack(">HHBBHII", e["id"], e["period"], e["volume"], 1 if e["loop"] else 0,
                                e["assetGainQ15"], len(data), len(e["data"])))
        data += e["data"]
    index_size = entries[-1]["id"] + 1
    index = bytearray(index_size + (index_size & 1))
    for n, e in enumerate(entries):
        index[e["id"]] = n + 1
    header_bytes = 32 + 16 * len(entries) + len(index)
    return (struct.pack(">4sHHIIH14x", b"DWSX", 2, len(entries), header_bytes + len(data), header_bytes, index_size)
            + b"".join(rows) + bytes(index) + data)


def convert():
    """The converter's audio stage over data/: the four installed banks."""
    with tempfile.TemporaryDirectory(prefix="sfx-", dir=ROOT / "tmp") as tmp:
        env = dict(os.environ, DOWNWELL_INSTALL_STAGES="audio")
        result = subprocess.run([str(CONVERTER), str(ROOT / "data"), tmp], env=env, capture_output=True, text=True)
        if result.returncode:
            sys.exit(result.stdout + result.stderr)
        return {name: (Path(tmp) / f"Downwell.snd{name}").read_bytes() for name in SCENE_BANKS}


def build():
    banks = convert()
    sounds = {}
    for name in SCENE_BANKS:
        for e in parse(banks[name]):
            if e["id"] in sounds and sounds[e["id"]]["data"] != e["data"]:
                sys.exit(f"{e['id']}: two banks hold different samples")
            sounds.setdefault(e["id"], dict(e, banks=[]))["banks"].append(name)
    entries = [sounds[i] for i in sorted(sounds)]
    blob = pack(entries)
    table, offset = [], 0
    for e in entries:
        table.append({"id": e["id"], "period": e["period"], "volume": e["volume"], "loop": e["loop"],
                      "assetGainQ15": e["assetGainQ15"], "offset": offset, "bytes": len(e["data"]),
                      "banks": e["banks"], "sha256": sha(e["data"])})
        offset += len(e["data"])
    meta = {"schemaVersion": 2, "tool": {"name": "pack-amiga-sfx.py", "version": VERSION},
            "converter": "build/install-data/DownwellInstallData (audio stage)",
            "output": {"path": "generated/amiga-sfx.bin", "bytes": len(blob), "sha256": sha(blob)},
            "banks": {name: {"path": f"generated/amiga-sfx-{name}.bin", "bytes": len(b), "sha256": sha(b),
                             "sounds": sum(1 for t in table if name in t["banks"])}
                      for name, b in banks.items()},
            "sounds": table}
    header = (f"// Generated by tools/pack-amiga-sfx.py {VERSION}; pack sha256 {sha(blob)[:16]}.\n"
              "#ifndef DOWNWELL_GENERATED_AMIGA_SFX_H\n#define DOWNWELL_GENERATED_AMIGA_SFX_H\n"
              f"#define DOWNWELL_SFX_BANK_BYTES {len(blob)}u\n#define DOWNWELL_SFX_BANK_SOUNDS {len(table)}u\n"
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
