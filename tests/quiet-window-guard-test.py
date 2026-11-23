#!/usr/bin/env python3
import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('quiet_window', ROOT / 'tools/verify-quiet-window.py')
guard = importlib.util.module_from_spec(spec)
spec.loader.exec_module(guard)


class Guard(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.directory = Path(self.temporary.name)
        self.baseline = json.loads((ROOT / 'docs/amiga-quiet-frame-baseline.json').read_text())
        self.current = json.loads(json.dumps(self.baseline))
        self.elf = self.directory / 'frozen.elf'
        self.elf.write_bytes(b'test-only ELF identity fixture')
        self.current['elf_sha256'] = hashlib.sha256(self.elf.read_bytes()).hexdigest()

    def verify(self):
        old, new = self.directory / 'before.json', self.directory / 'after.json'
        old.write_text(json.dumps(self.baseline))
        new.write_text(json.dumps(self.current))
        return guard.verify(old, new, self.elf)

    def test_control(self):
        mean, worst, generation = self.verify()
        self.assertAlmostEqual(mean, 20.58, places=2)
        self.assertAlmostEqual(worst, 27.5, places=1)
        self.assertEqual(generation, 390230 * (20 / 142102))

    def test_mean_regression_without_new_worst(self):
        maximum = max(r['cycles'] for r in self.current['frames'] if r['frame'] < 122)
        for r in self.current['frames']:
            if r['frame'] < 122:
                r['cycles'] = min(maximum, r['cycles'] + 5000)
        with self.assertRaisesRegex(ValueError, 'quiet mean'):
            self.verify()

    def test_worst_regression_without_mean_regression(self):
        row = max((r for r in self.current['frames'] if r['frame'] < 122), key=lambda r: r['cycles'])
        row['cycles'] += 8000
        with self.assertRaisesRegex(ValueError, 'quiet worst'):
            self.verify()

    def test_wait_redistribution_keeps_window(self):
        a, b = self.current['frames'][:2]
        a['cycles'], b['cycles'] = b['cycles'], a['cycles']
        self.verify()

    def test_half_millisecond_mean_noise(self):
        for row in self.current['frames']:
            if row['frame'] < 122:
                row['cycles'] += 3500
        self.verify()

    def test_generation_regression(self):
        self.current['frames'][-1]['cycles'] += 8000
        with self.assertRaisesRegex(ValueError, 'generation 122'):
            self.verify()

    def test_missing_quiet_frame(self):
        del self.current['frames'][4]
        with self.assertRaisesRegex(ValueError, 'missing complete frames'):
            self.verify()

    def test_missing_generation_frame(self):
        self.current['frames'].pop()
        with self.assertRaisesRegex(ValueError, 'missing complete frames'):
            self.verify()

    def test_partial_frame(self):
        self.current['partial_frames'] = [69]
        with self.assertRaisesRegex(ValueError, 'partial frame'):
            self.verify()

    def test_duplicate_frame(self):
        self.current['frames'].append(self.current['frames'][0])
        with self.assertRaisesRegex(ValueError, 'duplicate'):
            self.verify()

    def test_wrong_clock(self):
        self.current['pal_field_cycles'] = 100000
        with self.assertRaisesRegex(ValueError, 'different CPU clocks'):
            self.verify()

    def test_wrong_executable(self):
        self.elf.write_bytes(b'rebuilt executable must not validate old trace')
        with self.assertRaisesRegex(ValueError, 'ELF identity'):
            self.verify()


class TargetWork(unittest.TestCase):
    """The A1200's quiet frames fill their fields: only work can regress."""

    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.directory = Path(self.temporary.name)
        self.baseline_path = ROOT / 'docs/amiga-quiet-work-baseline-a1200-4m.json'
        self.baseline = json.loads(self.baseline_path.read_text())
        self.current = json.loads(json.dumps(self.baseline))
        self.elf = self.directory / 'frozen.elf'
        self.elf.write_bytes(b'test-only ELF identity fixture')
        self.current['elf_sha256'] = hashlib.sha256(self.elf.read_bytes()).hexdigest()

    def path(self, name, value):
        path = self.directory / name
        path.write_text(json.dumps(value))
        return path

    def verify(self):
        return guard.verify(self.path('before.json', self.baseline),
                            self.path('after.json', self.current), self.elf)

    def test_control_and_baseline_selection(self):
        mean, worst, generation = self.verify()
        self.assertLess(worst, 20)
        self.assertEqual(guard.matching_baseline(self.path('after.json', self.current)),
                         self.baseline_path)
        frames = json.loads((ROOT / 'docs/amiga-quiet-frame-baseline.json').read_text())
        self.assertEqual(guard.matching_baseline(self.path('a500.json', frames)),
                         ROOT / 'docs/amiga-quiet-frame-baseline.json')
        # Target whole frames have no baseline: they fill the field regardless of work.
        frames['pal_field_cycles'] = 284204
        self.assertIsNone(guard.matching_baseline(self.path('target.json', frames)))

    def test_work_regression_hidden_by_the_wait(self):
        # The frame keeps its length; the wait shrinks as work grows.
        for row in self.current['frames']:
            if row['frame'] < 122:
                row['wait_cycles'] -= 8000
        with self.assertRaisesRegex(ValueError, 'quiet mean'):
            self.verify()

    def test_different_wait_is_not_compared(self):
        self.current['wait_symbol'] = 'runDownwell()::{lambda#10}'
        with self.assertRaisesRegex(ValueError, 'different things'):
            self.verify()

    def test_invalid_wait(self):
        self.current['frames'][0]['wait_cycles'] = self.current['frames'][0]['cycles']
        with self.assertRaisesRegex(ValueError, 'invalid wait'):
            self.verify()


if __name__ == '__main__':
    unittest.main()
