#!/usr/bin/env python3
import importlib.util
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('quiet', Path(__file__).resolve().parents[1] / 'tools/verify-quiet-fall.py')
quiet = importlib.util.module_from_spec(spec)
spec.loader.exec_module(quiet)


def witness(fields=208):
    return (f'QUIET_CADENCE frames=164 fields={fields} fps={8200/fields:.3f} worst_gap=7 worst_frame=163\n'
            + 'QUIET_LANDING ' + ' '.join(f'{k}={v}' for k, v in quiet.LANDING.items()) + '\n'
            + 'QUIET_MOTION ' + ' '.join(f'{k}={v}' for k, v in quiet.MOTION.items()) + '\n'
            + 'DOWNWELL_QUIET_CADENCE_READY\n')


class Witness(unittest.TestCase):
    def test_shipping_profile_ceiling_preserves_accepted_improvement(self):
        script = (Path(__file__).resolve().parents[1] / 'amiga/profile.sh').read_text()
        ceilings = re.findall(r'verify-quiet-fall\.py[^\n]*--max-fields (\d+)', script)
        self.assertEqual(ceilings, ['197'])
        ceiling = int(ceilings[0])
        for fields in (181, 191, 197):
            self.assertEqual(quiet.verify(witness(fields), ceiling), fields)
        for fields in (198, 200, 206):
            with self.assertRaisesRegex(ValueError, 'cadence regression'):
                quiet.verify(witness(fields), ceiling)

    def test_workload_is_not_a_speed_gate(self):
        for fields in (181, 208, 400):
            self.assertEqual(quiet.verify(witness(fields)), fields)

    def test_explicit_regression_ceiling(self):
        for fields in (181, 200, 206):
            self.assertEqual(quiet.verify(witness(fields), 206), fields)
        for fields in (207, 208, 400):
            with self.assertRaisesRegex(ValueError, 'cadence regression'):
                quiet.verify(witness(fields), 206)
        with self.assertRaises(ValueError):
            quiet.verify(witness(181), 163)
        with self.assertRaisesRegex(ValueError, 'landing mismatch'):
            quiet.verify(witness(181).replace('health=4', 'health=3'), 206)

    def test_ceiling_command_exit_status(self):
        with tempfile.TemporaryDirectory(prefix='downwell-quiet-ceiling-') as directory:
            path = Path(directory) / 'run.log'
            for fields, accepted in ((200, True), (206, True), (207, False), (400, False)):
                path.write_text(witness(fields))
                result = subprocess.run([sys.executable, spec.origin, str(path), '--max-fields', '206'],
                                        capture_output=True, text=True)
                self.assertEqual(result.returncode == 0, accepted, result.stderr)
                self.assertEqual('goal completion not judged' in result.stdout, accepted)
            for ceiling in ('bad', '0', '-1', '163'):
                result = subprocess.run([sys.executable, spec.origin, str(path), '--max-fields', ceiling],
                                        capture_output=True, text=True)
                self.assertNotEqual(result.returncode, 0)

    def test_every_state_component_is_checked(self):
        for expected in (quiet.LANDING, quiet.MOTION):
            for key, value in expected.items():
                with self.subTest(key=key), self.assertRaises(ValueError):
                    quiet.verify(re.sub(rf'(?<!\w){key}={value}(?=\s)', f'{key}={value+1}', witness()))

    def test_command_exit_status(self):
        with tempfile.TemporaryDirectory(prefix='downwell-quiet-witness-') as directory:
            path = Path(directory) / 'run.log'
            for text, accepted in ((witness(), True), (witness().replace('health=4', 'health=3'), False)):
                path.write_text(text)
                result = subprocess.run([sys.executable, spec.origin, str(path)], capture_output=True, text=True)
                self.assertEqual(result.returncode == 0, accepted, result.stderr)
                self.assertEqual('performance not judged' in result.stdout, accepted)

    def test_incomplete_ambiguous_or_failed_runs_are_rejected(self):
        text = witness()
        for invalid in (text.replace('DOWNWELL_QUIET_CADENCE_READY', ''),
                        text + 'DOWNWELL_QUIET_INVALID_CADENCE\n', text + text,
                        text.replace('QUIET_MOTION', 'MISSING_MOTION'),
                        text.replace('x=0 ', 'x=0 x=0 '), text.replace('bullets=0', '')):
            with self.subTest(invalid=invalid), self.assertRaises(ValueError):
                quiet.verify(invalid)

    def test_cadence_accounting(self):
        for old, new in [('frames=164', 'frames=163'), ('fields=208', 'fields=0'),
                         ('fps=39.423', 'fps=50'), ('fps=39.423', 'fps=nan'),
                         ('worst_gap=7', 'worst_gap=209'), ('worst_frame=163', 'worst_frame=165')]:
            with self.subTest(new=new), self.assertRaises(ValueError):
                quiet.verify(witness().replace(old, new))


if __name__ == '__main__':
    unittest.main()
