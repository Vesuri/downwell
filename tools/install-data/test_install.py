#!/usr/bin/env python3
"""Host test of DownwellInstallData: convert data/ and compare each output
with the file the Python generators made in generated/, byte for byte.

    python3 tools/install-data/test_install.py build/install-data/DownwellInstallData

Until the converter builds every Downwell.tables section it writes each
built section as <TAG>.bin; those are compared with their generated inputs.
The fields and columns files are our own computation and ship in the archive.
"""
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
GEN = ROOT / 'generated'
DATA = ROOT / 'data'

# Installed name (Downwell.<suffix>) -> generated file.
FILES = {
    'boot': 'amiga-boot.dwap', 'front': 'amiga-front.dwap', 'game': 'amiga-game.dwap',
    'planes': 'amiga-game.planes', 'cavern': 'amiga-cavern.dwap', 'cavernplanes': 'amiga-cavern.planes',
    'results': 'amiga-results.dwap', 'shop': 'amiga-shop.dwap', 'pause': 'amiga-pause.dwap',
    'clear': 'amiga-clear.dwap', 'hud': 'amiga-hud.dwap', 'patterns': 'downwell-builder-patterns.dwbp',
    'exitmask': 'amiga-game.exit-mask', 'entrymask': 'amiga-game.entry-mask',
    'selectionmask': 'amiga-game.selection-mask', 'terrain': 'amiga-pattern-terrain.bin',
    'encountermasks': 'downwell-encounter-masks.dwsm', 'sndrun': 'amiga-sfx-run.bin',
    'sndfront': 'amiga-sfx-front.bin', 'sndresults': 'amiga-sfx-results.bin',
    'sndclear': 'amiga-sfx-clear.bin', 'tables': 'amiga-tables.bin',
}
# Downwell.tables section -> its generated input (tools/pack-amiga-tables.py).
SECTIONS = {
    'ROOM': 'downwell-runtime-rooms.dwrm', 'OBJT': 'downwell-objects.dwob', 'SCOL': 'downwell-sprites.dwsc',
    'STMP': 'amiga-effect-stamps.dwef', 'TPH6': 'amiga-terrain-phase6.bin', 'TINI': 'amiga-initial-terrain.bin',
    'AMFA': 'installed/amiga-ammo-factored.h.bin', 'AMDV': 'installed/amiga-ammo-dividers.h.bin',
    'HUDT': 'installed/amiga-hud-text.h.bin', 'GOAL': 'installed/downwell-goal-mask.h.bin',
    'TFLD': 'installed/downwell-time-field-mask.h.bin', 'ATTL': 'installed/downwell-area-titles.h.bin',
    'MTXT': 'installed/downwell-menu-text.h.bin', 'SPCH': 'installed/downwell-shop-speech.h.bin',
    'PALS': 'installed/downwell-palettes.h.bin', 'RISE': 'installed/downwell-rising-texts.h.bin',
    'RTPL': 'installed/amiga-room-terrain-plan.inc.bin', 'SNDD': 'installed/sound-durations.inc.bin',
    'SNDG': 'installed/sound-gains.inc.bin', 'HART': 'installed/amiga-health-art.h.bin',
    'DMGT': 'installed/amiga-damage-text.h.bin', 'CMOT': 'installed/contact-motion.inc.bin',
    'STRG': 'installed/smoke-trig.inc.bin',
}


def first_difference(a, b):
    for i, (x, y) in enumerate(zip(a, b)):
        if x != y:
            return i
    return min(len(a), len(b))


def convert(tool, source, out):
    return subprocess.run([str(tool), str(source), str(out)], capture_output=True, text=True)


def main():
    tool = Path(sys.argv[1]).resolve()
    failures, checked, missing = [], 0, []
    with tempfile.TemporaryDirectory(prefix='install-data-', dir=ROOT / 'tmp') as tmp:
        out = Path(tmp) / 'out'
        out.mkdir()
        r = convert(tool, DATA, out)
        if r.returncode:
            print(r.stdout + r.stderr)
            return 1
        made = {p.name: p.read_bytes() for p in out.iterdir()}
        pairs = [(f'Downwell.{k}', v) for k, v in FILES.items()] + [(f'{k}.bin', v) for k, v in SECTIONS.items()]
        for name, generated in pairs:
            if name not in made:
                missing.append(name)
                continue
            want, got = (GEN / generated).read_bytes(), made.pop(name)
            checked += 1
            if want != got:
                failures.append(f'{name}: differs from generated/{generated} at byte {first_difference(want, got)}'
                                f' ({len(got)} bytes, want {len(want)})')
        if 'Downwell.tables' not in missing:
            missing = [m for m in missing if not (len(m) == 8 and m.endswith('.bin'))]
        failures += [f'{name}: unexpected output' for name in made]
        # A changed input is refused before anything is written.
        wrong = Path(tmp) / 'wrong'
        wrong.mkdir()
        for p in DATA.iterdir():
            if p.is_file():
                os.symlink(p, wrong / p.name)
        (wrong / 'language.ini').unlink()
        (wrong / 'language.ini').write_bytes((DATA / 'language.ini').read_bytes() + b'\n')
        empty = Path(tmp) / 'empty'
        empty.mkdir()
        r = convert(tool, wrong, empty)
        if r.returncode == 0 or 'language.ini' not in r.stdout or any(empty.iterdir()):
            failures.append(f'a changed language.ini was not refused: {r.stdout.strip()}')
    for f in failures:
        print('FAIL', f)
    print(f'{checked} outputs identical to generated/, {len(failures)} failures; not built yet: {", ".join(missing) or "none"}')
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
