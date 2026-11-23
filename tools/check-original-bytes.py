#!/usr/bin/env python3
"""WI.3: no file we distribute holds a block of the player's files.

The siblings' tools/check_release.py rule: every aligned 64-byte block of
an original file with more than 8 distinct byte values, forward or
reversed, must not occur at any offset of any checked file. An aligned
block lies whole inside any copied run of 127 bytes or more, so sliding
over the checked file by one byte finds every such copy.

The originals are every file the Amiga slice's converter reads
(docs/amiga-input-inventory.md). Usage: check-original-bytes.py FILE...
"""
import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ORIGINALS = ['data.win', 'Downwell.exe', 'language.ini', 'sfxfalling.ogg', 'amb1.ogg',
             'sfxbossfirsthit.ogg'] + [f'audiogroup{n}.dat' for n in range(1, 7)]
SIZE = 64


def original_blocks(data_dir):
    blocks = {}
    for name in ORIGINALS:
        data = (data_dir / name).read_bytes()
        for offset in range(0, len(data) - SIZE + 1, SIZE):
            block = data[offset:offset + SIZE]
            if len(set(block)) > 8:
                blocks.setdefault(block, (name, offset, False))
                blocks.setdefault(block[::-1], (name, offset, True))
    return blocks


def first_copy(data, blocks):
    for i in range(len(data) - SIZE + 1):
        found = blocks.get(data[i:i + SIZE])
        if found:
            return i, found
    return None


def main():
    parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    parser.add_argument('files', nargs='+', type=Path)
    parser.add_argument('--data', type=Path, default=ROOT / 'data')
    args = parser.parse_args()
    blocks = original_blocks(args.data)
    failed = False
    for path in args.files:
        copy = first_copy(path.read_bytes(), blocks)
        if copy:
            offset, (name, source, reversed_) = copy
            print(f'{path}: offset {offset} holds {name} block at {source}'
                  f'{" (reversed)" if reversed_ else ""}', file=sys.stderr)
            failed = True
        else:
            print(f'{path}: no original block')
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
