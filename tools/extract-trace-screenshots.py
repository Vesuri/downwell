#!/usr/bin/env python3
"""Extract the per-field screenshots embedded in an FS-UAE instruction trace.

    python3 tools/extract-trace-screenshots.py tmp/trace/run/trace-699.bin \
        tmp/shots [FIELD,FIELD,...]

Each field of a `profile.sh trace` capture carries a JPEG of what the beam
displayed. Unlike `profile.sh frames` memory dumps, these show DMA-level
faults (sprites missing their stop line, a wrong fetch window).
"""
import pathlib
import struct
import sys

path, out = sys.argv[1], pathlib.Path(sys.argv[2])
out.mkdir(parents=True, exist_ok=True)
data = memoryview(open(path, 'rb').read())
offset = 0


def word():
    global offset
    value = struct.unpack_from('<I', data, offset)[0]
    offset += 4
    return value


fields = word()
sections = word()
offset += 4 * sections + 16
for _ in range(3):
    n = word(); offset += n
word(); word()
want = {int(x) for x in sys.argv[3].split(',')} if len(sys.argv) > 3 else None
for field in range(fields):
    n = word(); offset += n
    n = word(); offset += n
    for _ in range(2):
        size, count = word(), word(); offset += size * count
    _cycles, _instructions, count = word(), word(), word(); offset += 4 * count
    size, kind = word(), word()
    if want is None or field in want:
        blob = bytes(data[offset:offset + size])
        ext = 'jpg' if blob[:2] == b'\xff\xd8' else f'k{kind}.bin'
        (out / f'field-{field:03d}.{ext}').write_bytes(blob)
    offset += size
print(fields, 'fields written to', out)
