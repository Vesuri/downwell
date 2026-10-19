"""Check expanded make prerequisites, even when test executables do not exist."""
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[1]
targets = (
    "build/runner-borrowed-state-test",
    "build/stationary-retirement-test",
    "build/slot-family-map-test",
    "build/native-enemy-alarm-roster-test",
    "build/gameplay-scene-collision-contract-test",
    "build/native-collision-source-span-test",
    "build/stored-encounter-session-test",
    "build/gameplay-encounter-session-test",
)
result = subprocess.run(
    ["make", "-qp", *targets], cwd=root, capture_output=True, text=True
)
# Query mode returns 1 for targets requiring rebuilding, which is expected.
if result.returncode not in (0, 1):
    raise SystemExit(result.stderr)
lines = result.stdout.splitlines()
for target in targets:
    matches = [line for line in lines if line.startswith(target + ":")]
    assert len(matches) == 1, (target, matches)
    prerequisites = set(matches[0].split()[1:])
    for header in (
        "src/core/GameplayCollisionOwner.h",
        "src/core/RunnerSpatialIndex.h",
        "src/platform/amiga/AmigaWindAudio.h",
        "generated/clear-motion.inc",
    ):
        assert header in prerequisites, f"{target} omits {header}"
print(f"Host check dependencies: {len(targets)} early/late targets invalidate on shared headers")
