#!/usr/bin/env python3
"""The W3.5 census report: ring decoding, classes and handoff latency."""
import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest

spec = importlib.util.spec_from_file_location(
    'census', Path(__file__).resolve().parents[1] / 'tools/amiga-census-report.py')
census = importlib.util.module_from_spec(spec)
spec.loader.exec_module(census)

TICKS = 80000  # any field length; the log names it (the target uses 313 x 256)
MS = TICKS // 20


def record(work_ms, scene=3, frame=1, speed=60, flags=0, stage=5, span_ms=None):
    span = (span_ms if span_ms is not None else max(work_ms, 20)) * MS
    return struct.pack('>IIIBBBB', work_ms * MS, span, frame, scene, speed, flags, stage)


class Census(unittest.TestCase):
    def load(self, records, count=None, capacity=None):
        capacity = capacity or len(records)
        count = len(records) if count is None else count
        with tempfile.TemporaryDirectory() as directory:
            ring = Path(directory) / 'census.bin'
            ring.write_bytes(b''.join(records) + bytes(16 * (capacity - len(records))))
            log = f'noise\nCENSUS count={count} capacity={capacity} ticks_per_field={TICKS}\n'
            return census.load(ring, log)

    def test_classes_follow_their_precedence(self):
        loaded = self.load([
            record(5), record(30, flags=census.SLOWED | census.DELIBERATE),
            record(40, flags=census.HANDOFF | census.OVERLAY), record(8, flags=census.OVERLAY),
            record(9, flags=census.SIDE_CAMERA), record(2, scene=1),
            record(700, flags=census.WARMUP), record(3, scene=1, flags=census.HANDOFF)])
        self.assertEqual([r['class'] for r in loaded['records']],
                         ['gameplay', 'slowdown', 'handoff', 'overlay', 'camera jump',
                          'front end', 'handoff', 'handoff'])
        self.assertEqual((loaded['records'][0]['area'], loaded['records'][0]['level']), (1, 1))

    def test_budget_summary_counts_only_work_over_twenty(self):
        summary = census.summarise(self.load([record(19), record(20), record(25, frame=7)])['records'])
        self.assertEqual((summary['over_budget'], summary['worst_frame']), (1, 7))
        self.assertAlmostEqual(summary['excess_ms'], 5, places=3)
        self.assertAlmostEqual(summary['work_mean_ms'], 64 / 3, places=3)

    def test_wrapped_ring_keeps_the_newest_in_order(self):
        # Five records written into a ring of four: slot 0 holds the fifth.
        ring = [record(5, frame=5), record(2, frame=2), record(3, frame=3), record(4, frame=4)]
        loaded = self.load(ring, count=5, capacity=4)
        self.assertEqual(loaded['overwritten'], 1)
        self.assertEqual([r['frame'] for r in loaded['records']], [2, 3, 4, 5])
        self.assertEqual([r['index'] for r in loaded['records']], [1, 2, 3, 4])

    def test_handoff_latency_groups_consecutive_updates(self):
        records = self.load([
            record(1, scene=2), record(30, scene=2, flags=census.HANDOFF, span_ms=40),
            record(900, flags=census.HANDOFF | census.WARMUP, span_ms=950), record(5),
            record(6, flags=census.HANDOFF), record(7, stage=6, flags=census.HANDOFF), record(5, stage=6)])['records']
        transitions = census.handoffs(records)
        self.assertEqual([(t['from'], t['to'], t['updates']) for t in transitions],
                         [('style select', 'gameplay 1-1', 2), ('gameplay 1-1', 'gameplay 1-2', 2)])
        self.assertAlmostEqual(transitions[0]['latency_ms'], 990, places=2)
        self.assertAlmostEqual(transitions[0]['worst_ms'], 900, places=2)

    def test_malformed_input_is_rejected(self):
        with self.assertRaisesRegex(ValueError, 'exceeds span'):
            self.load([struct.pack('>IIIBBBB', 2, 1, 1, 3, 60, 0, 5)])
        with tempfile.TemporaryDirectory() as directory:
            ring = Path(directory) / 'census.bin'
            ring.write_bytes(record(1))
            with self.assertRaisesRegex(ValueError, 'not 2 records'):
                census.load(ring, f'CENSUS count=1 capacity=2 ticks_per_field={TICKS}\n')
            with self.assertRaisesRegex(ValueError, 'no CENSUS line'):
                census.load(ring, 'DOWNWELL_PROFILE_READY\n')


if __name__ == '__main__':
    unittest.main()
