#!/usr/bin/env python3
"""WR.4 world-one link gate for the native Amiga executable.

Reads amiga/world-one-manifest.json and the linked ELF's symbol table (via
m68k-amiga-elf-objdump, or $OBJDUMP) and fails when a sized code or data
symbol belonging to a later-area family links. Use it only on world-one
builds (DOWNWELL_WORLD_ONE_ONLY: no collision owner); owner builds bind those
families on purpose.
"""
import argparse
import json
import os
import re
import subprocess
import sys

SYMBOL = re.compile(r"^([0-9a-f]+) (.{7}) (\S+)\t([0-9a-f]+) (.*)$")
SECTIONS = (".text", ".rodata", ".data", ".bss")


def symbols(elf):
    objdump = os.environ.get("OBJDUMP", "m68k-amiga-elf-objdump")
    listing = subprocess.run([objdump, "-t", "-C", elf], check=True,
                             capture_output=True, text=True).stdout
    for line in listing.splitlines():
        match = SYMBOL.match(line)
        if not match:
            continue
        _, _, section, size, name = match.groups()
        if int(size, 16) and section.startswith(SECTIONS):
            yield section, int(size, 16), name


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("elf")
    parser.add_argument("--manifest", default=os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "..", "amiga", "world-one-manifest.json"))
    args = parser.parse_args()
    with open(args.manifest) as handle:
        manifest = json.load(handle)
    families = [(family["name"], re.compile(family["symbols"]))
                for family in manifest["laterAreaFamilies"]]
    found = []
    for section, size, name in symbols(args.elf):
        for family, pattern in families:
            if pattern.search(name):
                found.append((family, size, section, name))
                break
    if found:
        for family, size, section, name in sorted(found):
            print(f"*** later-area {family}: {size} bytes {section} {name}", file=sys.stderr)
        print(f"*** world-one build links {len(found)} later-area symbols "
              f"({sum(f[1] for f in found)} bytes)", file=sys.stderr)
        return 1
    print("World-one link audit: clean")
    return 0


if __name__ == "__main__":
    sys.exit(main())
