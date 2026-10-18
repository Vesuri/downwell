#!/usr/bin/env python3
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('budget', Path(__file__).resolve().parents[1] / 'tools/amiga-fall-budget.py')
budget = importlib.util.module_from_spec(spec)
spec.loader.exec_module(budget)


class ExactWork(unittest.TestCase):
    def data(self):
        return dict(schema='downwell-frame-work/v1', pal_field_cycles=142102,
                    partial_frames=[], wait_symbol=budget.WAIT,
                    frames=[dict(frame=139, cycles=150000, wait_cycles=8000,
                                 parts_cycles={'work': 142000, budget.WAIT: 8000})])

    def parse(self, data):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'work.json'
            path.write_text(json.dumps(data))
            return budget.parse([path])

    def test_small_wait_and_unrounded_threshold(self):
        result = self.parse(self.data())[139]
        self.assertGreater(result[0], 20)
        self.assertLess(result[1], 20)
        self.assertAlmostEqual(result[1], 142000 * 20 / 142102)

    def test_invalid_accounting_rejected(self):
        for field, value in [('wait_cycles', 150001), ('wait_cycles', -1),
                             ('cycles', 150001), ('frame', 0)]:
            data = self.data()
            data['frames'][0][field] = value
            with self.assertRaises(ValueError):
                self.parse(data)
        data = self.data()
        data['frames'] *= 2
        with self.assertRaises(ValueError):
            self.parse(data)
        data = self.data()
        data['partial_frames'] = [139]
        with self.assertRaises(ValueError):
            self.parse(data)


if __name__ == '__main__':
    unittest.main()
