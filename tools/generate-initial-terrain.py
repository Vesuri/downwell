#!/usr/bin/env python3
"""Generate source-hashed initial terrain artwork with the scalar pixel oracle."""
import hashlib
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
INPUTS = ["generated/amiga-game.dwap", "generated/amiga-terrain-phase6.bin",
          "generated/downwell-rooms.dwrm"]
OUTPUT = "generated/amiga-initial-terrain.bin"


def main():
    subprocess.run(["build/prepare-initial-terrain", *INPUTS, OUTPUT], cwd=ROOT, check=True)
    sha = lambda path: hashlib.sha256((ROOT / path).read_bytes()).hexdigest()
    provenance = json.loads((ROOT / "generated/amiga-game.packing.json").read_text())
    assert sha(INPUTS[0]) == provenance["packSha256"], "stale game-pack provenance"
    manifest = dict(schemaVersion=1, generator="generate-initial-terrain.py/v1",
                    sourceSha256=provenance["sourceSha256"], extractor=provenance["extractor"],
                    inputs={name: sha(name) for name in INPUTS},
                    tools={name: sha(name) for name in ["tools/generate-initial-terrain.py",
                           "tools/prepare-initial-terrain.cpp", "src/platform/amiga/AmigaTerrainPainter.cpp",
                           "src/platform/amiga/AmigaTerrain.cpp", "src/platform/amiga/AmigaRenderer.c",
                           "src/core/RoomPack.cpp", "src/core/RoomPack.h", "src/platform/amiga/AmigaTerrain.h",
                           "src/platform/amiga/AmigaRenderer.h", "src/platform/amiga/AmigaGameplayRows.h",
                           "src/platform/amiga/AmigaTerrainRunSurface.h", "src/platform/amiga/AmigaInitialTerrain.h"]},
                    roomId=11, spriteId=51, outputSha256=sha(OUTPUT),
                    bytes=(ROOT / OUTPUT).stat().st_size,
                    format="DWIT v1: big-endian rows, 3 column-major row references, 22-word row dictionary")
    (ROOT / OUTPUT).with_suffix(".json").write_text(json.dumps(manifest, indent=2)+"\n")


if __name__ == "__main__":
    main()
