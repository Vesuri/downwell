#!/usr/bin/env python3
"""Derive immutable HUD word tables and record all pixel-source inputs."""
import argparse
import hashlib
import json
import re
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "generated/amiga-hud-text.h"
INPUTS = ["data/data.win", "generated/amiga-hud.dwap", "generated/amiga-game.dwap",
          "generated/amiga-cavern.dwap",
          "generated/amiga-hud.manifest.json", "generated/amiga-game.packing.json",
          "tools/generate-amiga-hud-text.cpp", "tools/generate-amiga-hud-text.py",
          "src/platform/amiga/AmigaHudText.h", "src/core/HudNumbers.h",
          "src/platform/amiga/AmigaRenderer.c", "src/platform/amiga/AmigaRenderer.h"]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    def run(game):
        output = subprocess.check_output([str(ROOT / "build/generate-amiga-hud-text"),
                                          "generated/amiga-hud.dwap", game], cwd=ROOT)
        body, sizes = output.rstrip(b"\n").rsplit(b"\n", 1)
        match = re.fullmatch(rb"namespace downwellHudPrepared \{ inline constexpr unsigned "
                             rb"hudBytes=(\d+), gameBytes=(\d+), gameChipBytes=(\d+); \}", sizes)
        assert match, sizes
        return body, match.groups()
    # Production gameplay binds the cavern bank (a subset of the full game
    # pack); its HUD glyphs must derive exactly the same tables.
    body, (hud, game, game_chip) = run("generated/amiga-game.dwap")
    cavern_body, (cavern_hud, cavern, cavern_chip) = run("generated/amiga-cavern.dwap")
    if cavern_body != body or cavern_hud != hud:
        raise SystemExit("the cavern pack's HUD glyphs differ from the game pack's")
    payload = body + (b"\nnamespace downwellHudPrepared { inline constexpr unsigned hudBytes=%s, "
                      b"gameBytes=%s, gameChipBytes=%s, cavernBytes=%s, cavernChipBytes=%s; }\n"
                      % (hud, game, game_chip, cavern, cavern_chip))
    manifest = {"format": "amiga-hud-prepared-words-v2", "generator_version": 2,
                "pair_column_bytes": 49368,
                "extractor": json.loads((ROOT / "generated/amiga-hud.manifest.json").read_text())["extractor"],
                "resource_ids": {"currency_digits": 413, "currency_icon": 376,
                                 "streak_icon": 133, "derived_streak_digits": list(range(65100, 65110))},
                "inputs_sha256": {name: hashlib.sha256((ROOT / name).read_bytes()).hexdigest()
                                  for name in INPUTS},
                "output_sha256": hashlib.sha256(payload).hexdigest()}
    outputs = {OUTPUT: payload, OUTPUT.with_suffix(".json"):
               (json.dumps(manifest, indent=2) + "\n").encode()}
    for path, content in outputs.items():
        if args.check:
            if not path.exists() or path.read_bytes() != content:
                raise SystemExit(f"stale generated HUD table: {path}")
        else:
            path.parent.mkdir(exist_ok=True)
            temporary = path.with_suffix(path.suffix + ".tmp")
            temporary.write_bytes(content)
            temporary.replace(path)


if __name__ == "__main__":
    main()
