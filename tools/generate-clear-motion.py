#!/usr/bin/env python3
"""Emit exact canonical Result-room easing states, with source provenance."""
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
INPUTS = ["data/data.win", "data/Downwell.exe", "src/core/LevelClearMotion.h",
          "src/core/HudReal.h", "tools/generate-clear-motion.cpp",
          "tools/generate-clear-motion.py", "extracted/utmt-0.9.2.0/manifest.json",
          "extracted/utmt-0.9.2.0/Assembly/gml_Object_Result_Create_0.asm",
          "extracted/utmt-0.9.2.0/Assembly/gml_Object_Result_Alarm_4.asm",
          "extracted/utmt-0.9.2.0/Assembly/gml_GlobalScript_scrResult.asm"]

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    extracted = json.loads((ROOT / "extracted/utmt-0.9.2.0/manifest.json").read_text())
    if extracted["source"]["sha256"] != digest(ROOT / "data/data.win"):
        raise ValueError("extraction manifest does not match the untouched game data")
    payload = subprocess.check_output([str(ROOT / "build/generate-clear-motion")], cwd=ROOT)
    target = ROOT / "generated/clear-motion.inc"
    target.parent.mkdir(exist_ok=True)
    target.write_bytes(payload)
    manifest = {"format": "clear-motion-exact-v1", "generator_version": 1,
                "extractor": extracted["extractor"],
                "inputs_sha256": {name: digest(ROOT / name) for name in INPUTS},
                "output_sha256": digest(target)}
    target.with_suffix(".json").write_text(json.dumps(manifest, indent=2) + "\n")

if __name__ == "__main__":
    main()
