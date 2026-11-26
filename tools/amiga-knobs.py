#!/usr/bin/env python3
"""WR.2 knob sets for the native Amiga build.

Every `NAME ?= value` knob in amiga/Makefile belongs to exactly one set.
Shipping knobs select what ships and their defaults are the shipped choice,
except STRIP (shipping builds and the size gate use STRIP=1); gate knobs are the size-gate ceilings; the development, verify, profile and
reference sets stay off in a shipping build. With --check the tool fails on
an unclassified knob, a knob in two sets, or a listed name that is not a
knob; without it, it prints each set with the Makefile defaults.
"""
import argparse
import fnmatch
import os
import re
import sys

# Explicit names or shell-style patterns, in priority order of listing.
SETS = {
    "shipping": ["STRIP", "AMIGA_CPU", "AMIGA_OPTIMIZATION", "AMIGA_HOT_OPTIMIZATION", "AMIGA_HOT_OBJECTS",
                 "TABLE_CONTACT_ATAN", "RETAIN_HEALTH_HUD", "AMMO_DAMAGE_RANGE",
                 "PREPARED_AMMO_DIVIDERS", "LOCAL_DRAW_CAPTURE", "AMMO_FACTORED",
                 "PREPARED_FIELDS", "PREPARED_FIELDS_BACKING", "SCENERY_LAYERS", "NATIVE_BOXES",
                 "NATIVE_FIELDS", "TERRAIN_BATCH", "TERRAIN_DIRECT"],
    "gate": ["SIZE_MAX_*"],
    "development": ["SKIP_INTRO", "SKIP_TO_GAME", "SKIP_LEVEL", "COLLISION_OWNER"],
    "verify": ["*_VERIFY", "*_AUDIT", "REPLAY_FRAME_TRACE", "FRAME_DUMP"],
    "profile": ["PROBES", "FPSCOUNT", "PROFILE_*", "GENERATION_*", "PHASE_SAMPLING",
                "SAMPLE_FRAME", "TRACE_FRAMES", "CENSUS", "CENSUS_UPDATES",
                "EFFECT_BLIT_CENSUS", "BLIT_CENSUS", "CENSUS_PARTS"],
    "reference": ["REFERENCE_*"],
}
KNOB = re.compile(r"^([A-Z][A-Z0-9_]*)\s*\?=\s*(.*?)\s*\\?$")


def knobs(makefile):
    found = {}
    with open(makefile) as handle:
        for line in handle:
            match = KNOB.match(line.rstrip("\n"))
            if match and match.group(1) not in found:
                found[match.group(1)] = match.group(2)
    return found


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--makefile", default=os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "..", "amiga", "Makefile"))
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    found = knobs(args.makefile)
    members = {name: [] for name in found}
    problems = []
    for set_name, patterns in SETS.items():
        for pattern in patterns:
            matched = [name for name in found if fnmatch.fnmatchcase(name, pattern)]
            if not matched:
                problems.append(f"{set_name} entry {pattern} matches no knob")
            for name in matched:
                if set_name not in members[name]:
                    members[name].append(set_name)
    for name, sets in members.items():
        if len(sets) != 1:
            problems.append(f"knob {name} is in {len(sets)} sets {sets}")
    if args.check:
        for problem in problems:
            print("*** " + problem, file=sys.stderr)
        if problems:
            return 1
        print(f"Knob sets: {len(found)} knobs, each in one set")
        return 0
    for set_name in SETS:
        names = [name for name, sets in members.items() if sets == [set_name]]
        print(f"{set_name} ({len(names)}): " + " ".join(f"{n}={found[n]}" for n in names))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
