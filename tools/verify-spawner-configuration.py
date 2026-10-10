#!/usr/bin/env python3
"""Compare shared cavern spawner initialization with pinned VM assignments."""
from hashlib import sha256
from pathlib import Path
import json
import re
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
export = root / "extracted/utmt-0.9.2.0"
manifest = json.loads((export / "manifest.json").read_text())
source_hash = "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"
assert sha256((root / "data/data.win").read_bytes()).hexdigest() == source_hash
assert manifest["source"]["sha256"] == source_hash
assert manifest["extractor"]["version"] == "0.9.2.0"
assert manifest["extractor"]["commit"] == "03c7048438347799e0717b6e57a9358d000472e0"

# Both reviewed scripts have the same three explicit level branches. Decode
# their array stores, not the decompiler's text or copies of expected values.
store = re.compile(r"pushi.e (\d+)\nconv.i.v\npushi.e -1\npush.v self.chan\n"
                   r"conv.v.i\npush.v \[arraypopaf\]self.(spChannel|spEnemy)\n"
                   r"pushi.e (\d+)\npopaf.e")
expected = []
for hard, name, digest in (
    (0, "spawnerSet1", "f2f87b87c3567f5840210c9f417372d1fcf55f1ac76afd0ca599b0ce1ad0d74b"),
    (1, "hardSet1", "f908b2619535f15ab931b1b83fa05754dd5a574462d93b26c074c21d77813768"),
):
    payload = (export / "Assembly" / f"gml_GlobalScript_{name}.asm").read_bytes()
    assert sha256(payload).hexdigest() == digest
    text = payload.decode()
    assert "pushi.e 20\npop.v.i self.enemyMaxCount" in text
    for level, start, end in ((1, 2, 3), (2, 4, 5), (3, 6, 7)):
        block = text.split(f":[{start}]\n", 1)[1].split(f":[{end}]\n", 1)[0]
        channels = list(re.finditer(r"pushi.e (\d+)\npop.v.i self.chan\n", block))
        assert channels
        maximum = int(re.search(r"pushi.e (\d+)\npop.v.i self.chanMax", block)[1])
        assert maximum == len(channels) - 1
        for number, channel in enumerate(channels):
            assert int(channel[1]) == number
            limit = channels[number + 1].start() if number < maximum else len(block)
            part = block[channel.end():limit]
            fields, enemies = {}, {}
            for value, array, index in store.findall(part):
                dest = fields if array == "spChannel" else enemies
                assert int(index) not in dest
                dest[int(index)] = int(value)
            enemy_max = re.search(r"pushi.e (\d+)\nconv.i.v\npushi.e -1\npush.v self.chan\n"
                                  r"conv.v.i\npop.v.v \[array\]self.enemyMax", part)
            assert enemy_max and set(fields) == set(range(6))
            assert set(enemies) == set(range(int(enemy_max[1]) + 1))
            expected.append([hard, level, number, *[fields[i] for i in range(6)],
                             *[enemies[i] for i in range(len(enemies))]])

binary = Path(sys.argv[1] if len(sys.argv) > 1 else root / "build/spawner-step-test").resolve()
actual = [list(map(int, row.split())) for row in
          subprocess.check_output([binary, "--configurations"], text=True).splitlines()]
assert actual == expected, (actual, expected)
print(f"Spawner configuration: {len(expected)} channel rows across six normal/hard cavern levels match pinned VM stores")
