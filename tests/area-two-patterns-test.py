#!/usr/bin/env python3
"""Synthetic parser checks plus comparison of VM exports with recovered GML."""
import importlib.util
import hashlib
import json
from pathlib import Path
import re
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("area_two_patterns", ROOT / "tools/export-area-two-patterns.py")
exporter = importlib.util.module_from_spec(spec)
spec.loader.exec_module(exporter)


def fixture():
    source = "pushi.e 0\npop.v.i self.i\npush.i 123\nsetowner.e\n"
    for index, text in enumerate(("\r\n\tABC\r\n\tD\r\n\t", "\r\n\tEF\r\n\tGH")):
        if index:
            source += exporter.INCREMENT + "\n"
        source += (f"push.s {json.dumps(text)}@{45+index}\nconv.s.v\npushi.e -1\n"
                   "push.v self.i\nconv.v.i\npop.v.v [array]self.partMain\n")
    return source + "push.v self.i\npushi.e -1\npushi.e 100\npop.v.v [array]self.partMain\nexit.i\n"


class PatternTests(unittest.TestCase):
    def test_source_character_geometry(self):
        for script, digest in (
            ("scrBuilder", "0c95048af06ff612499acd64a717766441b820d19ffc6017c0ff3a95425581f5"),
            ("scrSetBuildx", "1c3e1cd4eac113d8491e4183654ac8cb5be47f67d41f706eff853c14160119ee"),
            ("groundRoom", "9708c07856cb976b1606c27023f8aa1b3fa9370fe38114df9b8361dbce39196d"),
        ):
            source = ROOT / f"extracted/utmt-0.9.2.0/Assembly/gml_GlobalScript_{script}.asm"
            self.assertEqual(hashlib.sha256(source.read_bytes()).hexdigest(), digest)
        box = ROOT / "extracted/utmt-0.9.2.0/Assembly/gml_Object_objBox_n_Create_0.asm"
        self.assertEqual(hashlib.sha256(box.read_bytes()).hexdigest(),
                         "fd90ec59a1a3fa6eeaf8f420e4bd7c5b1ecb6e83f18bff62df7c4844cc0320a5")
        result = exporter.build(ROOT)
        expected = []
        patterns = [p for family in result["families"] for p in family["patterns"]]
        for index, pattern in enumerate(patterns):
            self.assertTrue(pattern["text"].startswith("\r\n"))
            lines = pattern["text"][2:].split("\r\n")
            for reverse in (0, 1):
                for wrap in (0, 1):
                    def x(column):
                        return ((336 if wrap else 320) - column*16 if reverse else
                                (144 if wrap else 160) + column*16)
                    for pillars in (0, 1):
                        for extra in (0, 4):
                            expected.append(f"case {index} {reverse} {wrap} {pillars} {extra}")
                            y = 704
                            for row_index, line in enumerate(lines):
                                text = line.replace("\t", "")
                                for col, char in enumerate(text, 1):
                                    expected.extend(f"{kind} {ord(char)} {x(col)} {y}" for kind in ("A", "C"))
                                if row_index+1 < len(lines):
                                    expected.append(f"A 13 {x(len(text)+1)} {y}")
                                    if pillars:
                                        expected.extend(f"P 60 {x(col)} {y}" for col in (len(text)+1, 0))
                                    y += 16
                            for _ in range(extra):
                                if pillars:
                                    expected.extend(f"P 60 {x(col)} {y}" for col in (0, 10))
                                y += 16
                            expected.append(f"end {y}")
        actual = subprocess.check_output([ROOT / "build/builder-string-traversal-test",
                                          ROOT / "generated/downwell-area-two-patterns.dwbp"], text=True)
        self.assertEqual(actual.splitlines(), expected)

    def test_shared_runtime_reader_matches_all_vm_rows(self):
        result = exporter.build(ROOT)
        expected = []
        for family_id, family in enumerate(result["families"], 6):
            for pattern in family["patterns"]:
                expected.append(f"{family_id} {pattern['sourceIndex']} {max(pattern['rowWidths'])} {len(pattern['rows'])}")
                expected.append(pattern["text"].encode("ascii").hex())
                expected.extend(pattern["rows"])
        actual = subprocess.check_output([ROOT / "build/area-two-pattern-pack-test",
                                          ROOT / "generated/downwell-area-two-patterns.dwbp"], text=True)
        self.assertEqual(actual.splitlines(), expected)

    def test_preserves_ids_raw_text_and_irregular_rows(self):
        result = exporter.parse_vm(fixture(), "partMain", 2)
        self.assertEqual([p["stringId"] for p in result], [45, 46])
        self.assertEqual([p["sourceIndex"] for p in result], [0, 1])
        self.assertEqual(result[0]["text"], "\r\n\tABC\r\n\tD\r\n\t")
        self.assertEqual(result[0]["rows"], ["ABC", "D"])
        self.assertEqual(result[0]["rowWidths"], [3, 1])
        self.assertEqual(result, exporter.parse_vm(fixture().replace("\n", "\r\n"), "partMain", 2))

    def test_rejects_unrecognized_vm_operations(self):
        source = fixture()
        mutations = [
            source.replace("pushi.e 0", "pushi.e 1", 1),
            source.replace(exporter.INCREMENT, exporter.INCREMENT.replace("pushi.e 1", "pushi.e 2")),
            source.replace(exporter.INCREMENT, "call.i random(argc=0)\n" + exporter.INCREMENT),
            source.replace("[array]self.partMain", "[array]self.partOther", 1),
            source.replace("pushi.e 100", "pushi.e 99"),
            source + 'push.s "extra"@99\n',
            source + "pop.v.v [array]self.partMain\n",
            source.replace("ABC", r"\q"),
            source.replace("push.v self.i\nconv.v.i", "pushi.e 7", 1),
        ]
        for mutated in mutations:
            with self.subTest(source=mutated), self.assertRaises(ValueError):
                exporter.parse_vm(mutated, "partMain", 2)
        with self.assertRaises(ValueError):
            exporter.parse_vm(source, "partMain", 3)
        with self.assertRaises(ValueError):
            exporter.parse_vm("", "partMain", 0)

    def test_pinned_export_and_independent_gml_view(self):
        result = exporter.build(ROOT)
        patterns = [p for f in result["families"] for p in f["patterns"]]
        self.assertEqual(len(patterns), 50)
        self.assertEqual(sum(len(p["rows"]) for p in patterns), 722)
        self.assertEqual(sum(sum(p["rowWidths"]) for p in patterns), 8078)
        self.assertEqual(result["extractor"]["version"], "0.9.2.0")
        self.assertEqual(result["source"]["sha256"], exporter.SOURCE_SHA256)
        for family in result["families"]:
            self.assertIsInstance(family["scriptId"], int)
            self.assertIsInstance(family["codeId"], int)
            gml = (ROOT / "extracted/utmt-0.9.2.0/CodeEntries" /
                   family["script"].replace(".asm", ".gml")).read_text()
            # JSON string decoding is independent of the VM escape decoder.
            literals = re.findall(rf'{family["array"]}\[i\]\s*=\s*("(?:\\.|[^"\\])*");', gml)
            self.assertEqual([json.loads(value) for value in literals],
                             [p["text"] for p in family["patterns"]])


if __name__ == "__main__":
    unittest.main()
