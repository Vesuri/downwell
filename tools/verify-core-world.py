#!/usr/bin/env python3
"""Keep the extracted gameplay world and portable replay free of backend headers."""
from pathlib import Path
import shlex
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
sources = ["src/core/GameplayWorld.cpp", "tools/port-replay-reference.cpp"]
compiler = shlex.split(sys.argv[1] if len(sys.argv) > 1 else "clang++")
for flags in ([], ["-DDOWNWELL_PROBES", "-DDOWNWELL_FREESTANDING"]):
    for source in sources:
        result = subprocess.run(compiler + ["-Isrc", "-std=c++17", "-MM", "-MT", "world"]
                                + flags + [source], cwd=root, text=True,
                                capture_output=True, check=True)
        dependencies = shlex.split(result.stdout.replace("\\\n", " ").split(":", 1)[1])
        assert source in dependencies, (source, "missing source dependency")
        for dependency in dependencies:
            path = (root / dependency).resolve()
            assert path == root / source or root / "src/core" in path.parents, (source, dependency)
print("GameplayWorld and portable replay depend only on core in ordinary and probe builds")
