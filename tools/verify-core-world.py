#!/usr/bin/env python3
"""Guard the core boundary and reject abandoned SDL implementation branches."""
from pathlib import Path
import shlex
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
sources = ["src/core/GameplayWorld.cpp", "tools/port-replay-reference.cpp"]
compiler = shlex.split(sys.argv[1] if len(sys.argv) > 1 else "clang++")
# Source-derived, platform-independent inputs used by shared collision/math.
# Keep this an explicit list: accepting all generated headers would also admit
# target-only planar artwork or backend implementation dependencies.
core_generated = {root / "generated" / name for name in (
    "downwell-goal-mask.h", "downwell-time-field-mask.h", "smoke-trig.inc")}


def allowed(source, dependency):
    path = (root / dependency).resolve()
    return (path == root / source or root / "src/core" in path.parents or
            path in core_generated)


for rejected in ("src/platform/amiga/AmigaRenderer.h",
                 "src/platform/sdl/PlatformSDL.h", "generated/amiga-hud.h",
                 "src/core/../platform/sdl/PlatformSDL.h"):
    assert not allowed(sources[0], rejected), rejected

for flags in ([], ["-DDOWNWELL_PROBES", "-DDOWNWELL_FREESTANDING"]):
    for source in sources:
        result = subprocess.run(compiler + ["-Isrc", "-std=c++17", "-MM", "-MT", "world"]
                                + flags + [source], cwd=root, text=True,
                                capture_output=True, check=True)
        dependencies = shlex.split(result.stdout.replace("\\\n", " ").split(":", 1)[1])
        assert source in dependencies, (source, "missing source dependency")
        for dependency in dependencies:
            assert allowed(source, dependency), (source, dependency)
subprocess.run(compiler + ["-Isrc", "-std=c++17", "-fsyntax-only", "-Wunreachable-code",
                          "-Werror=unreachable-code", "src/core/StartupRuntime.cpp"],
               cwd=root, check=True)
print("GameplayWorld/replay are backend-free; StartupRuntime has no unreachable implementation branches")
