#!/usr/bin/env python3
"""Run release/Install in real Installer 43 under FS-UAE with scripted answers,
then start the installed game from its icon and from the Shell.
From Wing Commander 2's tools/test_release_installer.py.

    python3 tools/test_release_installer.py [mode ...] [--archive LHA] [--keep DIR]

Run it through `make test-installer`, which holds the shared emulator lock.
The installation files are the package members (tools/package_release.py from
the current build), or with --archive those decoded from a release archive.

Modes (all by default):
  fresh    no Downwell drawer yet: the Steam drawer is asked, the converter runs
  keep     Keep and Use existing over a complete install: no Steam question, the
           data stays as it was, the program, icon stack and our files update
  replace  Keep and Replace: the data becomes the converter's, stray files go
  incomplete  Keep over an install missing a converted file: no reuse question
  remove   Remove: everything but save.ini goes, then a fresh install
  wrong    Replace with a changed language.ini: the converter refuses, the
           Installer shows its message, the drawer is unchanged, no staging left
  start-icon, start-shell  the drawer `fresh` installed, started from its
           Workbench icon (the icon's stack) and from a Shell with Stack 8192
           by amiga/verify_startup.sh VERIFY_INSTALLED with
           tools/verify-workbench-start.py: to the menu, Escape, result 0.

The emulator is the target A1200 with 4 MB Fast (AMIGA_SETUP=a1200-4m, warp,
not cycle exact), Kickstart 3.1 and the Workbench 2.04 floppy's C: and Libs:.
The player's files are data/, mounted read-only. Requester answers are
substituted in the script (script-flow coverage, not requester behaviour);
no copy, rename, delete or converter run is mocked. The converted data must
equal what the host build of the same converter writes from data/. Saves
must survive every mode, and unrelated files in the parent drawer too.

DownwellInstallData builds every installed file. Should one ever be missing,
the checks that need the complete set report PENDING instead of failing (the
installed data's completeness, and the start checks, which then supplement
the missing files from the development build, amiga/out), so a full pass
prints no PENDING line. The emulator window stays visible.
"""
import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from package_release import FILES, OWN_DATA, PREFIX, members  # noqa: E402

DATA = ROOT / 'data'
HOST_CONVERTER = ROOT / 'build/install-data/DownwellInstallData'
MODES = ('fresh', 'keep', 'replace', 'incomplete', 'remove', 'wrong', 'start-icon', 'start-shell')
EXISTING = ('keep', 'replace', 'incomplete', 'remove', 'wrong')
INSTALLED = [f'Downwell.{k}' for k in FILES]
SAVE = b'[save]\nsaved by the game\n'
PACKAGE = {}
EXPECTED = {}  # the host converter's output: name -> bytes
PENDING = []
NO_POSITION = b'\x80\0\0\0\x80\0\0\0'


def replace_form(text, start, replacement):
    a = text.index(start)
    depth, quoted, i = 0, False, a
    while i < len(text):
        c = text[i]
        if quoted and c == '\\':
            i += 2
            continue
        if c == '"':
            quoted = not quoted
        elif not quoted:
            if c == '(':
                depth += 1
            elif c == ')':
                depth -= 1
                if depth == 0:
                    return text[:a] + replacement + text[i + 1:]
        i += 1
    raise ValueError('unbalanced Installer form: ' + start)


def mark(name, then=''):
    return f'((textfile (dest "DH2:{name}") (append "yes")) {then})'


def script(mode):
    s = PACKAGE['Install'].decode('latin-1')
    s = replace_form(s, '(welcome)', '(if 0 (welcome))')
    s = replace_form(s, '(set #source', '(set #source "DH0:")')
    s = s.replace('SYS:Utilities/MultiView', 'Execute DH0:multiview').replace('SYS:Utilities/More', 'Execute DH0:more')
    s = replace_form(s, '(set #parent', mark('parent-asked', '(set #parent "DH2:out")'))
    s = replace_form(s, '(set #remove-existing\n', mark('remove-asked', f'(set #remove-existing {int(mode == "remove")})'))
    s = replace_form(s, '(set #install-data\n', mark('reuse-asked', f'(set #install-data {int(mode in ("replace", "wrong"))})'))
    s = replace_form(s, '(set #steam\n', mark('steam-asked', '(set #steam "DH1:")'))
    s = replace_form(s, '(abort #failure)', '((textfile (dest "DH2:failure") (append #failure)) (exit (quiet)))')
    s = replace_form(s, '(exit)', '((textfile (dest "DH2:completed") (append "yes")) (exit (quiet)))')
    assert 'abort "The converted data' in s
    return s.encode('latin-1')


def old_install(dest, mode):
    """A distinct earlier install: every converted file (one missing for
    `incomplete`), old program and own files, a stray data file, the save."""
    data = dest / 'data'
    data.mkdir(parents=True)
    for name in INSTALLED + [f'Downwell.{k}' for k in OWN_DATA]:
        if not (mode == 'incomplete' and name == 'Downwell.tables'):
            (data / name).write_bytes(b'old ' + name.encode())
    (data / 'stray').write_bytes(b'stray file of an old install')
    (dest / 'Downwell').write_bytes(b'old program')
    icon = bytearray(PACKAGE['Downwell.inf'])
    icon[58:66] = bytes.fromhex('0000002000000010')  # placed by the player
    icon[74:78] = (4096).to_bytes(4, 'big')
    (dest / 'Downwell.info').write_bytes(icon)
    (dest / 'save.ini').write_bytes(SAVE)
    (dest / 'old-marker').write_bytes(b'kept unless removed')


def tree(base):
    return {p.relative_to(base).as_posix(): p.read_bytes() for p in sorted(base.rglob('*'))
            if p.is_file() and not p.name.endswith('.uaem')}


def fixture(mode, base):
    boot, out = base / 'boot', base / 'out'
    for p in (boot / 's', boot / 'c', out, base / 'state'):
        p.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(os.environ['INSTALLER43'], boot / 'Installer')
    shutil.copyfile(os.environ['SETPATCH'], boot / 'c/SetPatch')
    for name, data in PACKAGE.items():
        (boot / name).write_bytes(data)
    (boot / 'Install').write_bytes(script(mode))
    for viewer, rc in (('multiview', 20 if mode == 'fresh' else 0), ('more', 0)):
        (boot / viewer).write_text(f'.key FILE/A\nEcho "<FILE>" >DH2:{viewer}-readme\nQuit {rc}\n')
    steam = DATA
    if mode == 'wrong':
        steam = base / 'steam'
        steam.mkdir()
        for p in DATA.iterdir():
            if p.is_file() and p.name != 'language.ini':
                os.symlink(p, steam / p.name)
        (steam / 'language.ini').write_bytes((DATA / 'language.ini').read_bytes() + b'\n')
    if mode in EXISTING:
        old_install(out / 'Downwell', mode)
    (out / 'unrelated').write_text('keep')
    (boot / 's/startup-sequence').write_text(
        'CD DH0:\nDH0:c/SetPatch QUIET\nDF0:C/Assign C: DF0:C\nAssign LIBS: DF0:Libs\nAssign DEVS: DF0:Devs\n'
        'MakeDir RAM:ENV RAM:T\nAssign ENV: RAM:ENV\nAssign T: RAM:T\nPath DH0: ADD\nLoadWB\n'
        'Installer SCRIPT DH0:Install APPNAME Downwell MINUSER NOVICE DEFUSER NOVICE '
        'LOGFILE DH2:installer.log NOPRETEND >DH2:console.log\nEcho done >DH2:finished\n')
    return steam


LAUNCH = '''#!/bin/bash
set -euo pipefail
cd "$1"
. "$2/amiga/env.sh"
FSUAE_RUN=.
. "$FSUAE_COMMON"
trap 'fsuae_stop' EXIT
AMIGA_SETUP=a1200-4m DEBUG=0 SOUND=0 WARP=1 NTSC=0
EXTRA_ARGS='--uae_cpu_speed=max --uae_cpu_cycle_exact=false --uae_cpu_memory_cycle_exact=false'
fsuae_options
fsuae_launch --hard_drive_0="$PWD/boot" --hard_drive_0_priority=10 --hard_drive_1="$3" \\
  --hard_drive_1_read_only=1 --hard_drive_2="$PWD" --floppy_drive_0="$WORKBENCH_ADF" \\
  --floppy_drive_volume=0 --floppy_drive_volume_empty=0 --state_dir="$PWD/state"
wait "$FSUAE_PID"
'''


def emulate(base, steam, seconds):
    launch = base / 'launch.sh'
    launch.write_text(LAUNCH)
    started = time.monotonic()
    try:
        with (base / 'emulator.log').open('w') as log:
            emu = subprocess.Popen(['/bin/bash', str(launch), str(base), str(ROOT), str(steam)],
                                   stdout=log, stderr=log)
            deadline = started + seconds
            while time.monotonic() < deadline and not (base / 'finished').exists():
                if emu.poll() is not None:
                    raise SystemExit(f'FAIL: the emulator exited ({base})')
                time.sleep(.5)
            time.sleep(1)
    finally:
        subprocess.run(['/bin/bash', '-c', '. "$1/amiga/env.sh"; FSUAE_RUN="$2"; . "$FSUAE_COMMON"; '
                        'fsuae_stop_previous', 'stop', str(ROOT), str(base)], check=True)
        if 'emu' in locals():
            emu.wait(timeout=20)
    assert (base / 'finished').exists(), f'Installer did not finish within {seconds} s: {base}'
    return time.monotonic() - started


def check(mode, base, before):
    asked = lambda name: (base / name).exists()
    out = base / 'out'
    dest = out / 'Downwell'
    data = dest / 'data'
    assert (base / 'multiview-readme').read_text().strip() == 'DH0:ReadMe', 'ReadMe first'
    assert asked('more-readme') == (mode == 'fresh'), 'More fallback'
    assert asked('parent-asked')
    assert (out / 'unrelated').read_text() == 'keep', 'unrelated file in the parent drawer'
    assert not (out / 'Downwell-InstallStaging').exists(), 'staging drawer left behind'
    assert not (dest / 'data-previous').exists(), 'previous data left behind'
    existing = mode in EXISTING
    assert asked('remove-asked') == existing, 'Keep/Remove question'
    assert asked('reuse-asked') == (mode in ('keep', 'replace', 'wrong')), 'Use existing/Replace question'
    assert asked('steam-asked') == (mode != 'keep'), 'Steam drawer question'
    if mode == 'wrong':
        failure = (base / 'failure').read_text('latin-1')
        print('Installer message:', failure.strip().replace('\n', ' | '), flush=True)
        assert 'language.ini is not the Steam release of Downwell 1.0.5' in failure, failure
        assert 'DownwellInstallData failed:' in failure, failure
        assert not asked('completed')
        assert tree(dest) == before, 'a refused conversion changed the install'
        return
    assert not asked('failure'), (base / 'failure').read_text('latin-1') if asked('failure') else ''
    assert asked('completed'), 'Installer did not reach its end'
    files = tree(dest)
    assert files.pop('save.ini', None) == (SAVE if existing else None), 'saved game'
    assert (files.pop('old-marker', None) is not None) == (mode in ('keep', 'replace', 'incomplete')), 'old marker'
    assert files.pop('Downwell') == PACKAGE['Downwell'], 'program'
    assert files.pop('ReadMe') == PACKAGE['ReadMe'] and files.pop('ReadMe.info') == PACKAGE['ReadMe.info']
    icon = files.pop('Downwell.info')
    assert icon[48] == 3 and int.from_bytes(icon[74:78], 'big') == 8192, 'tool icon with an 8 KB stack'
    if mode in ('keep', 'replace', 'incomplete'):
        assert icon[58:66] == bytes.fromhex('0000002000000010'), "the player's icon position"
    else:
        assert icon[58:66] == NO_POSITION, 'a new icon is unplaced'
    for k in OWN_DATA:
        assert files.pop(f'data/Downwell.{k}') == PACKAGE[f'Downwell.{k}'], f'Downwell.{k}'
    installed = {k[5:]: v for k, v in files.items() if k.startswith('data/')}
    assert set(files) == {'data/' + k for k in installed}, sorted(set(files) - {'data/' + k for k in installed})
    if mode == 'keep':
        want = {n: b'old ' + n.encode() for n in INSTALLED}
        want['stray'] = b'stray file of an old install'
        assert installed == want, 'Use existing changed the converted data'
    else:
        assert installed.keys() == EXPECTED.keys(), \
            f'converted data: missing {sorted(EXPECTED.keys() - installed.keys())}, ' \
            f'unexpected {sorted(installed.keys() - EXPECTED.keys())}'
        for name, want in EXPECTED.items():
            assert installed[name] == want, f'data/{name} differs from the host converter output'
        missing = sorted(set(INSTALLED) - installed.keys())
        if missing:
            PENDING.append(f'{mode}: the converter does not write {len(missing)} installed files yet: '
                           + ', '.join(missing))
    assert (out / 'Downwell.info').exists() or existing, 'drawer icon'


def start(mode, installed):
    """Start the installed drawer through amiga/verify_startup.sh."""
    drawer = Path(tempfile.mkdtemp(prefix=f'installed-{mode}-', dir=ROOT / 'tmp')) / 'Downwell'
    shutil.copytree(installed, drawer, ignore=shutil.ignore_patterns('*.uaem'))
    supplemented = [n for n in INSTALLED if not (drawer / 'data' / n).exists()]
    for n in supplemented:
        shutil.copyfile(ROOT / 'amiga/out' / n, drawer / 'data' / n)
    if supplemented:
        PENDING.append(f'{mode}: started with {len(supplemented)} development files (amiga/out) in place of '
                       'the converter\'s: ' + ', '.join(supplemented))
    env = dict(os.environ, VERIFY_INSTALLED=str(drawer), GDB='../tools/verify-workbench-start.py',
               READY_MARKER='DOWNWELL_START_READY', READY_DESCRIPTION='the installed game\'s start and exit',
               VERIFY_WORKBENCH='1' if mode == 'start-icon' else '0', VERIFY_TIMEOUT_SECONDS='600')
    r = subprocess.run(['bash', 'verify_startup.sh'], cwd=ROOT / 'amiga', env=env, capture_output=True, text=True)
    print(r.stdout[-1500:], r.stderr[-1500:], sep='', flush=True)
    assert r.returncode == 0, f'{mode}: the installed game did not start and exit'
    shutil.rmtree(drawer.parent)


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('modes', nargs='*', default=MODES)
    p.add_argument('--archive', type=Path)
    p.add_argument('--keep', type=Path, help='copy the last installed drawer to this directory')
    p.add_argument('--seconds', type=int, default=3600, help='wall-clock limit for one Installer run')
    args = p.parse_args()
    assert set(args.modes) <= set(MODES), args.modes
    for name in ('INSTALLER43', 'SETPATCH', 'WORKBENCH_ADF'):
        assert os.environ.get(name), f'{name} is not set: source amiga/env.sh'
    if args.archive:
        listing = subprocess.run(['lha', 'lq', str(args.archive)], check=True, capture_output=True, text=True).stdout
        for line in listing.splitlines():
            m = re.search(r' \d\d:\d\d (.+)$', line)
            name = m[1] if m else ''
            if name.startswith(PREFIX + '/'):
                PACKAGE[name[len(PREFIX) + 1:]] = subprocess.run(['lha', 'pq', str(args.archive), name],
                                                                 check=True, capture_output=True).stdout
        assert {'Install', 'Downwell', 'DownwellInstallData'} <= PACKAGE.keys(), sorted(PACKAGE)
    else:
        PACKAGE.update(members())
    (ROOT / 'tmp').mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='installer-host-', dir=ROOT / 'tmp') as tmp:
        r = subprocess.run([str(HOST_CONVERTER), str(DATA), tmp], capture_output=True, text=True)
        assert r.returncode == 0, r.stdout + r.stderr
        EXPECTED.update((p.name, p.read_bytes()) for p in Path(tmp).iterdir())
    installed = None
    for mode in args.modes:
        if mode.startswith('start-'):
            if installed is None:
                raise SystemExit(f'FAIL: {mode} needs the fresh mode first')
            start(mode, installed)
            print(f'PASS: {mode}', flush=True)
            continue
        base = Path(tempfile.mkdtemp(prefix=f'installer-{mode}-', dir=ROOT / 'tmp'))
        print(base, flush=True)
        steam = fixture(mode, base)
        before = tree(base / 'out/Downwell') if mode in EXISTING else {}
        seconds = emulate(base, steam, args.seconds)
        check(mode, base, before)
        print(f'PASS: Installer {mode} ({seconds:.0f} s)', flush=True)
        if mode == 'fresh':
            installed = base / 'out/Downwell'
        if args.keep and mode == [m for m in args.modes if not m.startswith('start-')][-1]:
            shutil.rmtree(args.keep, ignore_errors=True)
            shutil.copytree(base / 'out/Downwell', args.keep, ignore=shutil.ignore_patterns('*.uaem'))
            print('Installed drawer:', args.keep)
        if mode != 'fresh':
            shutil.rmtree(base)
    if installed is not None:
        shutil.rmtree(installed.parents[1])
    for line in PENDING:
        print('PENDING (needs the complete converter):', line)
    print('PASS:', ', '.join(args.modes) + (f'; {len(PENDING)} checks PENDING' if PENDING else ''))


if __name__ == '__main__':
    main()
