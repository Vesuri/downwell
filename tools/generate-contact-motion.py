#!/usr/bin/env python3
"""Generate and certify the finite contact-motion decoder bank."""
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "generated/contact-motion.inc"
INPUTS = ["data/Downwell.exe", "data/data.win", "generated/smoke-trig.inc",
          "generated/atan-q16.inc", "src/core/RunnerPointDirection.h",
          "src/core/SmokeDirection.h", "src/core/SmokeTrig.h",
          "src/core/ContactMotionCodec.h", "src/core/MotionBinary64.h",
          "src/core/MultiplyBinary64.h", "src/core/RunnerImageValue.h",
          "tools/generate-contact-motion.cpp", "tools/generate-contact-motion.py"]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    payload = subprocess.check_output([str(ROOT / "build/generate-contact-motion")], cwd=ROOT)
    OUTPUT.parent.mkdir(exist_ok=True)
    OUTPUT.write_bytes(payload)
    manifest = {"format": "contact-motion-lossless-v1", "generator_version": 1,
                "resource_ids": {"player_damage_number": 221},
                "certified_inputs": 411776,
                "inputs_sha256": {name: digest(ROOT / name) for name in INPUTS},
                "output_sha256": digest(OUTPUT)}
    OUTPUT.with_suffix(".json").write_text(json.dumps(manifest, indent=2) + "\n")


if __name__ == "__main__":
    main()
