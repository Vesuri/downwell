#!/usr/bin/env python3
"""Content growth must not silently grow the resident native art bank."""
import importlib.util
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from amiga_scene_contract import GAME_IDS, BANK_BUDGETS, require_budget

spec = importlib.util.spec_from_file_location('amiga_hud', ROOT / 'tools/generate-amiga-hud.py')
hud = importlib.util.module_from_spec(spec)
spec.loader.exec_module(hud)


def record(ident, frame=0):
    return ({'id': ident}, frame, 16, b'')


class Admission(unittest.TestCase):
    def test_existing_ownership_and_shared_dependencies(self):
        records = [record(696), record(369), record(43), record(51, 7),
                   record(51, 13), record(51, 0), record(27), record(76),
                   record(63027), record(654)]
        banks = hud.scene_banks(records)
        keys = lambda bank: [(r[0]['id'], r[1]) for r in banks[bank]]
        self.assertEqual(keys('boot'), [(696, 0)])
        self.assertEqual(keys('front'), [(369, 0), (43, 0), (51, 7), (51, 13), (27, 0), (76, 0)])
        self.assertEqual(keys('game'), [(51, 7), (51, 13), (51, 0), (27, 0), (76, 0), (63027, 0)])

    def test_unrelated_exporter_growth_is_rejected(self):
        for ident in (200, 62002, 63002, 65111):
            with self.assertRaisesRegex(ValueError, f'asset {ident} frame 0 has no scene owner'):
                hud.scene_banks([record(51), record(ident)])
        # Merely extending the exporter selection must not declare residency.
        hud.SELECTIONS['game'][200] = None
        try:
            with self.assertRaises(ValueError):
                hud.scene_banks([record(200)])
        finally:
            del hud.SELECTIONS['game'][200]

    def test_unrelated_owned_scene_does_not_grow_game(self):
        game = [record(ident) for ident in sorted(GAME_IDS)]
        baseline = hud.scene_banks(game)['game']
        for amount in (1, 10, 1000):
            extended = game + [record(696, frame) for frame in range(amount)]
            self.assertEqual(hud.scene_banks(extended)['game'], baseline)

    def test_budgets_are_fixed_and_fail_closed(self):
        for bank, limit in BANK_BUDGETS.items():
            require_budget(bank, limit)
            for size in (0, limit + 1, limit * 2):
                with self.assertRaises(ValueError):
                    require_budget(bank, size)
        with self.assertRaisesRegex(ValueError, 'undeclared scene bank'):
            require_budget('future-world', 1)


if __name__ == '__main__':
    unittest.main()
