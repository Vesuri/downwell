#!/usr/bin/env python3
"""Export the reviewed straight-line SSE smoke trig paths, never the source file."""
from pathlib import Path
import hashlib
import json
import runpy
import struct
import capstone
import argparse

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--check", action="store_true", help="verify generated output without writing")
args = parser.parse_args()

root = Path(__file__).resolve().parents[1]
audit = runpy.run_path(str(root / "tools/verify-runner-choose.py"))
region = audit["region"]
decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
decoder.detail = True
# These exponent dispatches admit absolute top words 0x3030 through 0x40f5,
# not just the currency wrapper's +/-720-degree float radians. Keep the signed input
# unchanged for the recovered reduction; do not substitute modulo degrees.
for start, fallback in ((0x74b621, "0x74b76e"), (0x74b7e1, "0x74b92e")):
    dispatch = [(i.mnemonic, i.op_str) for i in decoder.disasm(region(start, 23), start)]
    assert dispatch == [("pextrw", "eax, xmm0, 3"), ("and", "ax, 0x7fff"),
                        ("sub", "ax, 0x3030"), ("cmp", "ax, 0x10c5"), ("ja", fallback)]
# Large arguments leave this exported program for actual x87 transcendental
# instructions. Do not extend the polynomial or wrap degrees to hide that path.
for call, target, operation, opcode in ((0x74b797, "0x770b1f", 0x770b3d, "fcos"),
                                      (0x74b964, "0x770c0f", 0x770c2d, "fsin")):
    assert [(i.mnemonic, i.op_str) for i in decoder.disasm(region(call, 5), call)] == [("call", target)]
    assert [(i.mnemonic, i.op_str) for i in decoder.disasm(region(operation, 2), operation)] == [(opcode, "")]
lines = ["// Generated from the pinned original runner; do not commit this payload.",
         "template<class Real> bool smokeTrigProgram(Real input, bool cosine, Real& output) {",
         "Real r[8][2]{}; r[0][0]=input; int index=0; bool ok=true;"]
tables = []
for row in range(64):
    values = []
    for col in range(4):
        low, high = struct.unpack("<II", region(0x877630+row*32+col*8, 8))
        values.append(f"Real::constant(0x{high:08x}u,0x{low:08x}u)")
    tables.append("{"+",".join(values)+"}")
lines.append("static const Real table[64][4]={"+",\n".join(tables)+"};")
records = []
for cosine, start, end in ((True,0x74b638,0x74b76e),(False,0x74b7f8,0x74b92e)):
    lines.append("if(cosine) {" if cosine else "else {")
    data = region(start,end-start)
    records.append(dict(start=hex(start),end=hex(end),sha256=hashlib.sha256(data).hexdigest()))
    instructions = list(decoder.disasm(data,start))
    assert instructions and instructions[0].address == start
    assert sum(ins.size for ins in instructions) == len(data)
    assert instructions[-1].mnemonic == "ret"
    for ins in instructions:
        m = ins.mnemonic
        lines.append(f"// {ins.address:08x} {m} {ins.op_str}")
        if m == "ret": continue
        if m == "lea":
            assert ins.op_str == "eax, [0x877630]"; continue
        if m == "shl":
            assert ins.op_str == "edx, 5"; continue
        if m == "add" and ins.op_str == "eax, edx": continue
        if m in ("add","and"):
            assert ins.reg_name(ins.operands[0].reg)=="edx"
            op = "+=" if m=="add" else "&="
            lines.append(f"index {op} {ins.operands[1].imm};"); continue
        if m=="cvtsd2si":
            assert ins.reg_name(ins.operands[0].reg)=="edx"
            r=int(ins.reg_name(ins.operands[1].reg)[3:])
            lines.append(f"if(!r[{r}][0].integer(index))return false;"); continue
        dst=int(ins.reg_name(ins.operands[0].reg)[3:])
        def source(lane):
            operand=ins.operands[1]
            if operand.type==capstone.x86.X86_OP_REG:
                return f"r[{int(ins.reg_name(operand.reg)[3:])}][{lane}]"
            assert operand.type==capstone.x86.X86_OP_MEM
            mem=operand.mem
            assert not mem.index
            if mem.base:
                assert ins.reg_name(mem.base)=="eax" and 0<=mem.disp<32
                return f"table[index][{mem.disp//8+lane}]"
            low,high=struct.unpack("<II",region(mem.disp+lane*8,8))
            return f"Real::constant(0x{high:08x}u,0x{low:08x}u)"
        if m in ("movapd","movlpd","movsd"):
            for lane in range(2 if m=="movapd" else 1):lines.append(f"r[{dst}][{lane}]={source(lane)};")
        elif m in ("unpcklpd","unpckhpd"):
            lane=0 if m=="unpcklpd" else 1
            lines.append(f"{{auto v={source(lane)};r[{dst}][0]=r[{dst}][{lane}];r[{dst}][1]=v;}}")
        elif m in ("addsd","subsd","mulsd","addpd","subpd","mulpd"):
            op={"add":"add","sub":"subtract","mul":"multiply"}[m[:3]]
            for lane in range(2 if m.endswith("pd") else 1):
                lines.append(f"ok=r[{dst}][{lane}].{op}({source(lane)}) && ok;")
        else: raise AssertionError((m,ins.op_str))
    lines.append("}")
lines += ["if(ok)output=r[0][0];return ok;", "}"]
header = "\n".join(lines)+"\n"
manifest = json.dumps(dict(
    sourceSha256=hashlib.sha256(audit["binary"]).hexdigest(),
    exporter="export-smoke-trig.py",formatVersion=1,capstoneVersion=capstone.__version__,
    exporterSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
    headerSha256=hashlib.sha256(header.encode()).hexdigest(),
    paths=records,tableAddress="0x877630",tableBytes=2048,
    tableSha256=hashlib.sha256(region(0x877630,2048)).hexdigest()),indent=2)+"\n"
for name, content in (("smoke-trig.inc", header), ("smoke-trig.json", manifest)):
    path = root / "generated" / name
    if args.check:
        assert path.read_text() == content, f"Stale generated output: {path}"
    else:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content)
print("Exported both straight-line trig paths and 64 table rows with source hashes")
