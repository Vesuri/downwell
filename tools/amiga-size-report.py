#!/usr/bin/env python3
"""Size report and gate for the native Amiga executable (WR).

Reads the linked ELF with m68k-amiga-elf-objdump: section totals, the
largest symbols of a section, and code grouped by the class (or namespace
scope) that defines each function. With --max-* limits it is the shipping
size gate: it fails when a section or the stripped hunk file grows past
its ceiling.
"""
import argparse
import collections
import os
import re
import subprocess
import sys

OBJDUMP = os.environ.get("OBJDUMP", "m68k-amiga-elf-objdump")


def run(*args):
    return subprocess.run([OBJDUMP, *args], check=True, capture_output=True,
                          text=True).stdout


def sections(elf):
    sizes = collections.OrderedDict()
    for line in run("-h", elf).splitlines():
        parts = line.split()
        if len(parts) > 6 and parts[1].startswith("."):
            sizes[parts[1]] = int(parts[2], 16)
    return sizes


def symbols(elf):
    """Sized symbols, each address once: GCC aliases the complete and base
    constructors (and other identical entry points) to one body."""
    pattern = re.compile(r"^([0-9a-f]+) (.{7}) (\S+)\t([0-9a-f]+) (.*)$")
    seen = set()
    for line in run("-t", "-C", elf).splitlines():
        match = pattern.match(line)
        if not match:
            continue
        address, flags, section, size, name = match.groups()
        size = int(size, 16)
        if size and section.startswith(".") and (section, address) not in seen:
            seen.add((section, address))
            yield section, size, name


def section_group(section):
    for prefix in (".text", ".rodata", ".data", ".bss"):
        if section == prefix or section.startswith(prefix + "."):
            return prefix
    return section


def owner(name):
    """The defining class of a demangled function, else its leading scope."""
    name = re.sub(r"\[clone [^\]]*\]", "", name)
    name = name.split("(", 1)[0]
    depth = 0
    parts, current = [], ""
    i = 0
    while i < len(name):
        c = name[i]
        if c == "<":
            depth += 1
        elif c == ">":
            depth -= 1
        if depth == 0 and name.startswith("::", i):
            parts.append(current)
            current = ""
            i += 2
            continue
        current += c
        i += 1
    parts.append(current)
    scopes = [re.sub(r"<.*", "", p).strip() for p in parts[:-1]]
    scopes = [s for s in scopes if s and s != "downwell" and s != "(anonymous namespace)"]
    if scopes:
        return scopes[-1] if len(scopes) == 1 else scopes[0]
    return "(free functions)"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("elf")
    parser.add_argument("--exe", help="hunk executable whose file size is reported")
    parser.add_argument("--top", type=int, default=0, help="largest symbols per section")
    parser.add_argument("--classes", type=int, default=0, help="largest defining classes of .text")
    parser.add_argument("--max-exe", type=int)
    parser.add_argument("--max-text", type=int)
    parser.add_argument("--max-data", type=int, help=".rodata + .data")
    parser.add_argument("--max-bss", type=int)
    args = parser.parse_args()

    totals = collections.Counter()
    for section, size in sections(args.elf).items():
        totals[section_group(section)] += size
    exe = os.path.getsize(args.exe) if args.exe else None
    print("text %d rodata %d data %d bss %d%s" % (
        totals[".text"], totals[".rodata"], totals[".data"], totals[".bss"],
        " exe %d" % exe if exe is not None else ""))

    if args.top or args.classes:
        by_section = collections.defaultdict(list)
        classes = collections.Counter()
        for section, size, name in symbols(args.elf):
            group = section_group(section)
            by_section[group].append((size, name))
            if group == ".text":
                classes[owner(name)] += size
        for group in (".text", ".rodata", ".data", ".bss"):
            if args.top and by_section[group]:
                print("\n%s, largest %d:" % (group, args.top))
                for size, name in sorted(by_section[group], reverse=True)[:args.top]:
                    print("%9d  %s" % (size, name[:150]))
        if args.classes:
            print("\n.text by defining class, largest %d:" % args.classes)
            for name, size in classes.most_common(args.classes):
                print("%9d  %s" % (size, name[:150]))

    failures = []
    for label, value, limit in (("exe", exe, args.max_exe),
                                ("text", totals[".text"], args.max_text),
                                ("rodata+data", totals[".rodata"] + totals[".data"], args.max_data),
                                ("bss", totals[".bss"], args.max_bss)):
        if limit is not None and value is not None and value > limit:
            failures.append("%s %d exceeds %d" % (label, value, limit))
    for failure in failures:
        print("*** shipping size gate: " + failure)
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
