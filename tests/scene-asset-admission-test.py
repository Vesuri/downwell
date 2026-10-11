#!/usr/bin/env python3
"""Content growth must not silently grow the resident native art bank."""
import importlib.util
from pathlib import Path
import sys
import struct
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from amiga_scene_contract import GAME_IDS, CAVERN_IDS, CAVERN_ROOTS, UPGRADE_PLAYER_IDS, BANK_BUDGETS, require_budget

spec = importlib.util.spec_from_file_location('amiga_hud', ROOT / 'tools/generate-amiga-hud.py')
hud = importlib.util.module_from_spec(spec)
spec.loader.exec_module(hud)
audit_spec = importlib.util.spec_from_file_location('scene_load_audit', ROOT / 'tools/audit-amiga-scene-loads.py')
audit = importlib.util.module_from_spec(audit_spec)
audit_spec.loader.exec_module(audit)


def record(ident, frame=0):
    return ({'id': ident}, frame, 16, b'')


class Admission(unittest.TestCase):
    def test_rejected_elf_cannot_bypass_audit_on_retry(self):
        makefile = (ROOT / 'amiga/Makefile').read_text()
        # Exercise the production Hunk publication rule with an existing ELF
        # and executable, not a reimplementation of its dependency ordering.
        begin = makefile.index('.PHONY: FORCE_HUNK_OPTIONS')
        end = makefile.index('\n$(OUT).elf:', begin)
        rule = makefile[begin:end]
        self.assertIn('audit: $(OUT).elf\n', makefile)
        with tempfile.TemporaryDirectory(prefix='downwell-scene-guard-') as directory:
            root = Path(directory)
            (root / 'Downwell.elf').write_bytes(b'existing rejected ELF')
            executable = root / 'Downwell.exe'
            executable.write_bytes(b'previous accepted executable')
            (root / 'Downwell.hunk-options').write_text('1\n')
            harness = ('OUT := Downwell\nSTRIP := 1\n.PHONY: audit\n' + rule +
                       '\naudit: $(OUT).elf\n\t@echo AUDIT_REJECTED\n\t@false\n')
            for _ in range(2):
                result = subprocess.run(['make', '-j4', '-f', '-', 'Downwell.exe'],
                                        input=harness, text=True, cwd=root,
                                        stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
                self.assertNotEqual(result.returncode, 0, result.stdout)
                self.assertIn('AUDIT_REJECTED', result.stdout)
                self.assertEqual(executable.read_bytes(), b'previous accepted executable')
                self.assertFalse((root / 'Downwell.hunk-options.tmp').exists())

    def test_shipping_load_path_audit(self):
        cavern = b'PROGDIR:Downwell.cavern\0PROGDIR:Downwell.cavernplanes\0'
        self.assertEqual(audit.violations(cavern), [])
        for path in audit.FORBIDDEN:
            self.assertEqual(audit.violations(cavern + path), [path[:-1].decode('ascii')])

    def test_cavern_admission_is_not_the_gameplay_union(self):
        for ident in (91, 116, 205, 206, 207, 208, 460):
            for offset in (0, 62000, 63000):
                self.assertNotIn(ident + offset, CAVERN_IDS)
        for ident in (31, 115, 133, 153, 702, 63031, 63153, 63702, 65110):
            self.assertIn(ident, CAVERN_IDS)
        self.assertTrue(CAVERN_IDS < GAME_IDS)
        self.assertNotIn(200, CAVERN_ROOTS)
        # Later gameplay assets cannot gain residency by catalogue growth.
        baseline = [i for i in sorted(GAME_IDS) if i in CAVERN_IDS]
        extended = GAME_IDS | frozenset(range(10000, 20000))
        self.assertEqual([i for i in sorted(extended) if i in CAVERN_IDS], baseline)

    def test_cavern_and_upgrade_player_payloads_match_full_reference(self):
        def records(path):
            data = path.read_bytes()
            count = struct.unpack_from('>I', data, 8)[0]
            result = {}
            for i in range(count):
                row = hud.RECORD.unpack_from(data, 20 + i * 24)
                result[row[:2]] = (row[2:8], data[row[8]:row[8]+row[9]])
            return result
        full = records(ROOT / 'generated/amiga-game.dwap')
        cavern = records(ROOT / 'generated/amiga-cavern.dwap')
        self.assertEqual(cavern, {key: value for key, value in full.items() if key[0] in CAVERN_IDS})
        self.assertLess(len(cavern), len(full))
        clear = records(ROOT / 'generated/amiga-clear.dwap')
        dma_ids = {63000 + ident for ident in UPGRADE_PLAYER_IDS}
        self.assertEqual({key: value for key, value in clear.items() if key[0] in dma_ids},
                         {key: value for key, value in full.items() if key[0] in dma_ids})
        self.assertTrue(all(any(key[0] == ident for key in clear) for ident in dma_ids))
        self.assertFalse(any(key[0] in UPGRADE_PLAYER_IDS for key in clear))

    def test_upgrade_player_dependency_does_not_follow_game_catalogue(self):
        selection = dict(hud.SELECTIONS['clear'])
        for ident in range(10000, 11000):
            hud.SELECTIONS['game'][ident] = None
        try:
            self.assertEqual(hud.SELECTIONS['clear'], selection)
            self.assertEqual(UPGRADE_PLAYER_IDS, {1, 3, 15, 27, 31, 37})
        finally:
            for ident in range(10000, 11000):
                del hud.SELECTIONS['game'][ident]

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
