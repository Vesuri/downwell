#!/usr/bin/env python3
"""Boundary tests for the host compiler, independent of the native consumer."""
import importlib.util
from pathlib import Path
import unittest

root = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("room_plan", root / "tools/generate-room-terrain-plan.py")
compiler = importlib.util.module_from_spec(spec)
spec.loader.exec_module(compiler)


def manifest(records):
    instances = [dict(instanceId=100+i, objectId=obj, x=x, y=y)
                 for i, (obj, x, y) in enumerate(records)]
    order = [dict(index=i, instanceId=r["instanceId"], objectId=r["objectId"])
             for i, r in enumerate(instances)]
    return dict(schemaVersion=3, source=dict(sha256=compiler.SOURCE),
                resources=dict(rooms=[dict(id=11, instances=instances, instanceCreationOrder=order)]))


class CompilerTest(unittest.TestCase):
    def test_prefixes_and_slots(self):
        count, commands = compiler.compile_room(manifest([
            (35, 240, 176), (112, 0, 0), (93, -512, 8176),
            (60, -512, 8176), (99, 1, -9), (60, -512, 8176)]))
        self.assertEqual(count, 6)
        self.assertEqual(commands, [
            (102, 2, 93, -512, 8176, 65408, 1, 0, 34),
            (103, 3, 60, -512, 8176, 65408, 1, 34, 123),
            (104, 4, 99, 1, -9, 0, 0, 0, 0),
            (105, 5, 60, -512, 8176, 65408, 1, 123, 187)])

    def test_overflow(self):
        with self.assertRaises(ValueError):
            compiler.compile_room(manifest([(60, 0, 0)] * 4))

    def test_geometry(self):
        for x, y in [(-528, 0), (1536, 0), (0, -16), (0, 8192), (1, 0), (0, 1)]:
            with self.subTest(x=x, y=y), self.assertRaises(ValueError):
                compiler.compile_room(manifest([(60, x, y)]))
        _, commands = compiler.compile_room(manifest([(60, 1520, 8176)]))
        self.assertEqual(commands[0][5:7], (65535, 1))

    def test_order(self):
        source = manifest([(60, 0, 0)])
        source["resources"]["rooms"][0]["instanceCreationOrder"][0]["index"] = 1
        with self.assertRaises(ValueError):
            compiler.compile_room(source)


if __name__ == "__main__":
    unittest.main()
