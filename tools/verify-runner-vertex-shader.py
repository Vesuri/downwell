#!/usr/bin/env python3
"""Decode the embedded Windows vertex program, not a GPU execution oracle."""
import base64
from hashlib import sha256
from pathlib import Path
import runpy
import struct

root = Path(__file__).resolve().parents[1]
# Reuse the input/tool identity and palette-source checks.
metadata = runpy.run_path(str(root / "tools/verify-default-shader.py"))["metadata"]
blob = base64.b64decode(metadata["shader"]["HLSL11_VertexData"]["Data"], validate=True)
assert blob.index(b"DXBC") == 420
container = blob[420:]
size, count = struct.unpack_from("<II", container, 24)
assert size == 1564 and count == 6
container = container[:size]
assert sha256(container).hexdigest() == "3d61c6d3110829dcfbe11559547acb9dce43a372d0dde04da1cd6b526e4ae3de"
chunks = {}
for offset in struct.unpack_from(f"<{count}I", container, 32):
    tag = container[offset:offset + 4]
    length = struct.unpack_from("<I", container, offset + 4)[0]
    assert offset >= 32 + 4 * count and offset + 8 + length <= size
    assert tag not in chunks
    chunks[tag] = container[offset + 8:offset + 8 + length]


def signatures(data):
    count, offset = struct.unpack_from("<II", data)
    result = []
    for index in range(count):
        name, semantic, system, kind, register, masks = struct.unpack_from(
            "<6I", data, offset + 24 * index)
        result.append((data[name:].split(b"\0", 1)[0].decode(), semantic,
                       system, kind, register, masks & 255))
    return result


assert signatures(chunks[b"ISGN"]) == [
    ("POSITION", 0, 0, 3, 0, 7), ("COLOR", 0, 0, 3, 1, 15),
    ("TEXCOORD", 0, 0, 3, 2, 3)]
assert signatures(chunks[b"OSGN"]) == [
    ("TEXCOORD", 0, 0, 3, 0, 3), ("SV_Position", 0, 1, 3, 1, 15)]

# Check the reflected matrix array rather than assuming constant-buffer slots.
reflection = chunks[b"RDEF"]
buffers, offset = struct.unpack_from("<II", reflection)
assert buffers == 1
_, variables, offset, buffer_size, _, _ = struct.unpack_from("<6I", reflection, offset)
assert buffer_size == 784
fields = {}
for index in range(variables):
    name, start, length, flags, typ, default = struct.unpack_from(
        "<6I", reflection, offset + index * 24)
    fields[reflection[name:].split(b"\0", 1)[0]] = (start, length, typ)
start, length, typ = fields[b"_gm_Matrices"]
assert (start, length) == (432, 320)
assert struct.unpack_from("<6H", reflection, typ) == (3, 3, 4, 4, 5, 0)
assert (start + 4 * 64) // 16 == 43  # Fifth 4x4 matrix, not an anchor offset.

# Narrow decoder for the forms actually present. Definitions: Microsoft's
# DirectXShaderCompiler/include/dxc/Support/d3d12TokenizedProgramFormat.hpp.
# Unsupported modes fail rather than being silently skipped.
def operand(words, cursor):
    token = words[cursor]
    cursor += 1
    assert not token & 0x80000000  # No extended modifier.
    components, mode = token & 3, (token >> 2) & 3
    kind, dimensions = (token >> 12) & 255, (token >> 20) & 3
    assert token >> 22 == 0  # Only immediate32 indices.
    if kind == 4:
        assert components == 1 and dimensions == 0
        return ("literal", words[cursor]), cursor + 1
    assert components == 2 and kind in (0, 1, 2, 8)
    assert dimensions == (2 if kind == 8 else 1)
    indices = words[cursor:cursor + dimensions]
    cursor += dimensions
    if mode == 0:
        selection = "".join(c for i, c in enumerate("xyzw") if token & (16 << i))
    else:
        assert mode == 1
        selection = "".join("xyzw"[(token >> (4 + 2 * i)) & 3] for i in range(4))
    return (("r", "v", "o")[kind] if kind < 3 else "cb", indices, selection), cursor


words = struct.unpack(f"<{len(chunks[b'SHDR']) // 4}I", chunks[b"SHDR"])
assert words[0] == 0x10040 and words[1] == len(words)  # vs_4_0
cursor = 2
declarations, program = [], []
while cursor < len(words):
    token = words[cursor]
    opcode, length = token & 2047, (token >> 24) & 127
    assert not token & 0x80fff800 and length and cursor + length <= len(words)
    instruction = words[cursor:cursor + length]
    if opcode in (89, 95, 101, 103, 104):
        assert not program
        declarations.append((opcode, length))
    else:
        assert opcode in (54, 17, 62)  # mov, dp4, ret; no rounding/conversion.
        args, at = [], 1
        for _ in range({54: 2, 17: 3, 62: 0}[opcode]):
            arg, at = operand(instruction, at)
            args.append(arg)
        assert at == length
        program.append((opcode, args))
    cursor += length
assert declarations == [(89, 4), (95, 3), (95, 3), (101, 3), (103, 4), (104, 2)]
assert program == [
    (54, [("o", (0,), "xy"), ("v", (2,), "xyxx")]),
    (54, [("r", (0,), "xyz"), ("v", (0,), "xyzx")]),
    (54, [("r", (0,), "w"), ("literal", 0x3f800000)]),
] + [(17, [("o", (1,), component), ("cb", (0, 43 + index), "xyzw"),
           ("r", (0,), "xyzw")]) for index, component in enumerate("xyzw")] + [(62, [])]

audit = runpy.run_path(str(root / "tools/verify-runner-choose.py"))
region = audit["region"]
for start, end, digest in (
    (0x453a70, 0x453ba7, "eeacb72d9586ac7f88f0149fada525eb8a11f7a0324cfcd5578b16a50da5d2b3"),
    (0x6f75c0, 0x6f76a7, "b29a69663b47ccd022e9453d3adc1235c787c5eb3bdb0307300ba563e8bc3e0c"),
):
    assert sha256(region(start, end - start)).hexdigest() == digest
assert struct.unpack("<f", region(0x800308, 4))[0] == 2.0
assert struct.unpack("<f", region(0x795ab8, 4))[0] == 1.0
print("Embedded vs_4_0: texture-coordinate forwarding and four matrix dot products verified")
print("Pinned orthographic matrix builder; feature-level selection, upload and raster sampling remain unproven")
