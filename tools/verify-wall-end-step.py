#!/usr/bin/env python3
"""Execute code 986's integer VM control flow against the C++ trace matrix.

External calls are explicit fixtures, not a substitute for runner query/tiling
semantics. In particular shape and point queries receive distinct answers.
"""
import json
from pathlib import Path
import re
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
export = root / "extracted/utmt-0.9.2.0"
manifest = json.loads((export / "manifest.json").read_text())
assert manifest["source"]["sha256"] == "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"
assert manifest["extractor"]["version"] == "0.9.2.0"
assert manifest["resources"]["code"][986]["name"] == "gml_Object_objWall_n_Step_2"
wall = manifest["resources"]["objects"][99]
assert wall["name"] == "Wall" and wall["parentId"] == 60
override = [event for event in wall["events"] if (event["eventType"], event["eventSubtype"]) == (3, 2)]
assert len(override) == 1 and [action["codeId"] for action in override[0]["actions"]] == [1112]
assert manifest["resources"]["code"][1112]["name"] == "gml_Object_Wall_Step_2"
assert not [line for line in (export / "Assembly/gml_Object_Wall_Step_2.asm").read_text().splitlines()
            if line.strip() and not line.strip().startswith((".", ":["))]
child = manifest["resources"]["objects"][379]
assert (child["id"], child["name"], child["spriteId"], child["Depth"]) == (379, "limboShard", 91, 0)
assert [(event["eventType"], event["eventSubtype"],
         [action["codeId"] for action in event["actions"]]) for event in child["events"]] == [
    (0, 0, [2769]), (3, 0, [2770]), (4, 89, [2771]), (8, 0, [2772]), (14, 0, [2773])]
# Birth is synchronous in production. This trace fixture records the call;
# it does not execute the child's RNG-consuming Create or claim to verify it.
assert manifest["resources"]["code"][2769]["name"] == "gml_Object_limboShard_Create_0"
code, labels = [], {}
for line in (export / "Assembly/gml_Object_objWall_n_Step_2.asm").read_text().splitlines():
    line = line.strip()
    if line.startswith(":["):
        labels[line[2:-1]] = len(code)
    elif line and not line.startswith("."):
        code.append(line)

def run(state, area, neighbors, diagonals, choice):
    stack, trace = [], []
    pc = draws = steps = 0
    def external(name, args):
        nonlocal draws
        if name == "gml_Script_scrOutofview":
            assert args == []
            trace.append(1)
        elif name in ("place_meeting", "position_meeting"):
            x, y, obj = args
            assert obj == 60
            point = name == "position_meeting"
            offsets = [(-16,-16),(16,-16),(-16,16),(16,16)] if point else [(0,-16),(16,0),(0,16),(-16,0)]
            bit = offsets.index((x-state["x"],y-state["y"]))
            trace.append(3 if point else 2)
            return ((diagonals if point else neighbors) >> bit) & 1
        elif name == "gml_Script_tile_add":
            assert args == [739,48,48,16,16,state["x"]-8,state["y"]-8,100]
            trace.append(4)
        elif name == "instance_destroy":
            assert args == []
            trace.append(5)
        elif name == "@@This@@":
            assert args == []
            return 123456
        elif name == "instance_deactivate_object":
            assert args == [123456]
            trace.append(6)
        elif name == "choose":
            assert args == [0,0,1] and choice in (0,1,2)
            draws += 1
            return args[choice]
        elif name == "gml_Script_instance_create":
            assert args == [state["x"],state["y"]-16,379]
            trace.append(7)
        else:
            raise AssertionError(name)
        return 0
    while pc < len(code):
        steps += 1
        assert steps < 1000
        instruction = code[pc]
        pc += 1
        op, *tail = instruction.split(" ", 1)
        arg = tail[0] if tail else ""
        if op in ("pushi.e", "push.e"):
            stack.append(int(arg))
        elif op == "pushglb.v":
            assert arg == "global.area"
            stack.append(area)
        elif op == "push.v":
            stack.append(state[arg.removeprefix("self.")])
        elif op.startswith("pop.v."):
            key = arg.removeprefix("self.")
            state[key] = stack.pop()
            if key == "image_index":
                trace.append(100+state[key])
        elif op.startswith("conv."):
            if op.endswith(".b"):
                stack[-1] = int(bool(stack[-1]))
        elif op == "dup.v":
            assert arg == "0"
            stack.append(stack[-1])
        elif op == "popz.v":
            stack.pop()
        elif op == "not.b":
            stack[-1] = int(not stack[-1])
        elif op.startswith(("add.", "sub.", "cmp.")):
            b, a = stack.pop(), stack.pop()
            if op.startswith("add."): result = a+b
            elif op.startswith("sub."): result = a-b
            else:
                assert arg in ("EQ", "NEQ")
                result = int(a == b) if arg == "EQ" else int(a != b)
            stack.append(result)
        elif op in ("b", "bt", "bf"):
            condition = True if op == "b" else bool(stack.pop()) == (op == "bt")
            if condition: pc = labels[arg[1:-1]]
        elif op == "call.i":
            name, count = re.fullmatch(r"(.+)\(argc=(\d+)\)",arg).groups()
            stack.append(external(name,[stack.pop() for _ in range(int(count))]))
        else:
            raise AssertionError(instruction)
    assert not stack
    return draws, trace

binary = Path(sys.argv[1] if len(sys.argv)>1 else root / "build/wall-end-step-test").resolve()
rows = subprocess.check_output([str(binary),"--oracle"],text=True).splitlines()
assert len(rows) == 9216
for line in rows:
    area,pillar,neighbors,diagonals,choice,phase,expected,draws,trace = json.loads(line)
    if phase == 0:
        state = dict(x=1000,y=2000,checked=0,isPillar=pillar,i=0,image_index=0,checkThing=0,
                     chkdist=0,surrounded=0,deactivate=0,lightUpLeft=0,lightUpRight=0,lightDownLeft=0,lightDownRight=0)
        total = 0
    else:
        assert phase == 1
        state["deactivate"] = 1
    used, actual_trace = run(state,area,neighbors,diagonals,choice)
    total += used
    lights = sum(state[name]<<i for i,name in enumerate(("lightUpLeft","lightUpRight","lightDownLeft","lightDownRight")))
    actual = [state["checked"],state["i"],state["image_index"],state["checkThing"],state["chkdist"],state["surrounded"],lights]
    assert (actual,total,actual_trace) == (expected,draws,trace), (line,actual,total,actual_trace)
print("Pinned VM code 986: 9216 End Step state/call/RNG traces match the portable handler")
