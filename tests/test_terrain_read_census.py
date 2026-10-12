"""The diagnostic report must reject incomplete or inactive captures."""
import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest

spec = importlib.util.spec_from_file_location(
    'census', Path(__file__).resolve().parents[1] / 'tools/report-terrain-reads.py')
census = importlib.util.module_from_spec(spec)
spec.loader.exec_module(census)


class CensusTest(unittest.TestCase):
    def setUp(self):
        self.width = 2 * len(census.NAMES)
        self.words = [0] * (165 * self.width)
        for sample in range(1, 165):
            self.words[sample * self.width] = sample
            self.words[sample * self.width + 12] = sample
            self.words[sample * self.width + len(census.NAMES)] = sample
            self.words[sample * self.width + len(census.NAMES) + 15] = sample

    def read(self, words):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'census.bin'
            path.write_bytes(struct.pack('>' + 'I' * len(words), *words))
            return census.read(path)

    def test_valid_endian_and_window(self):
        samples = self.read(self.words)
        self.assertEqual(samples[164][0], 164)
        self.assertEqual(samples[121][0] - samples[55][0], 66)

    def test_truncated(self):
        with self.assertRaisesRegex(ValueError, 'incomplete'):
            self.read(self.words[:-1])

    def test_startup_leak(self):
        self.words[0] = 1
        with self.assertRaisesRegex(ValueError, 'startup'):
            self.read(self.words)

    def test_counter_reset(self):
        self.words[100 * self.width] = 0
        with self.assertRaisesRegex(ValueError, 'decreased'):
            self.read(self.words)

    def test_missing_positive_controls(self):
        with self.assertRaisesRegex(ValueError, 'positive controls'):
            self.read([0] * len(self.words))

    def test_unused_consumer_identity_is_valid(self):
        for sample in range(165):
            self.words[sample * self.width] = 0
        self.assertEqual(self.read(self.words)[164][0], 0)


if __name__ == '__main__':
    unittest.main()
