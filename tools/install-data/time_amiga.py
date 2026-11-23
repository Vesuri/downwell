#!/usr/bin/env python3
"""Time DownwellInstallData on the emulated target (a1200-4m: cycle-exact
68EC020 at 14 MHz, 4 MB Fast), from the Shell with Stack 16384, over data/
mounted read-only. Prints each stage's time, the peak allocation, the stack
used and the result, and compares the outputs with the host converter's.

    make -C tools/install-data amiga-timing all
    exec 9>>tmp/amiga.lock; python3 tools/hold-lock.py 9; python3 tools/install-data/time_amiga.py [setup]

--probe [BUILD] times the amiga-cmot-probe build (BUILD: its build directory,
default build/install-data/cmot-probe) instead: the exe stage alone, with no
output comparison. --multiply-test [BUILD] runs the amiga-multiply-test
build: binary64_amiga.s against its portable oracle.
"""
import argparse
import filecmp
import os
import shutil
import socket
import subprocess
import sys
import tempfile
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
BUILD = ROOT / 'build/install-data'
STAGES = ['inputs', 'data.win', 'text', 'rooms', 'masks', 'code', 'exe', 'art', 'derived', 'audio', 'soundtables']


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('setup', nargs='?', default='a1200-4m')
    parser.add_argument('--probe', nargs='?', const=str(BUILD / 'cmot-probe'), type=str)
    # A Workbench boot runs SetPatch, which turns on the 68030's data cache
    # (Kickstart 3.1 leaves it off; 0x2001 against 0x2111). The 68020 has none.
    parser.add_argument('--no-setpatch', dest='setpatch', action='store_false',
                        help='skip $SETPATCH QUIET before the converter')
    parser.add_argument('--multiply-test', nargs='?', const=str(BUILD / 'multiply-test'), type=str)
    options = parser.parse_args()
    setup = options.setup
    if options.multiply_test:
        options.probe = options.multiply_test
    probe = Path(options.probe).resolve() if options.probe else None
    built = probe or BUILD / 'timing'
    elf = built / 'DownwellInstallData.elf'
    with tempfile.TemporaryDirectory(prefix='install-timing-', dir=ROOT / 'tmp') as tmp:
        base = Path(tmp)
        boot, out = base / 'boot', base / 'out'
        (boot / 's').mkdir(parents=True)
        out.mkdir()
        for d in ('state', 'home'):
            (base / d).mkdir()
        shutil.copyfile(built / 'DownwellInstallData.exe', boot / 'DownwellInstallData')
        first = ''
        if options.setpatch:
            (boot / 'c').mkdir()
            shutil.copyfile(os.environ['SETPATCH'], boot / 'c/SetPatch')
            first = 'DH0:c/SetPatch QUIET\n'
        (boot / 's/startup-sequence').write_text(first + 'Stack 16384\nDownwellInstallData DH1: DH2:\n')
        s = socket.socket(); s.bind(('127.0.0.1', 0)); port = s.getsockname()[1]; s.close()
        if options.multiply_test:
            (base / 'timing.gdb').write_text('\n'.join([
                'set pagination off', 'set confirm off', 'set remotetimeout 120',
                f'target remote 127.0.0.1:{port}', 'break multiply_test_done', 'continue',
                'printf "MULTIPLY cases=%lu admitted=%lu failures=%lu wide=%d first=%08lx%08lx*%08lx%08lx\\n", '
                'multiply_test_cases, multiply_test_admitted, multiply_test_failures, install_wide_multiply, '
                'multiply_test_first[0], multiply_test_first[1], multiply_test_first[2], multiply_test_first[3]',
                'printf "ADD admitted=%lu failures=%lu first=%08lx%08lx+%08lx%08lx\\n", add_test_admitted, '
                'add_test_failures, add_test_first[0], add_test_first[1], add_test_first[2], add_test_first[3]',
                'detach', 'quit']) + '\n')
        lines = ['set pagination off', 'set confirm off', 'set remotetimeout 120',
                 f'target remote 127.0.0.1:{port}', 'break installer_test_done', 'continue',
                 'printf "TIMING result=%d peak=%lu stack=%lu unused=%lu\\n", installer_result, install_peak, '
                 'installer_stack_size, installer_stack_unused',
                 'printf "TIMING cache=%lu\\n", install_cache_bits']
        lines += [f'printf "TIMING tick{i}=%ld\\n", install_ticks[{i}]' for i in range(len(STAGES) + 1)]
        lines += ['detach', 'quit']
        if not options.multiply_test:
            (base / 'timing.gdb').write_text('\n'.join(lines) + '\n')
        env = dict(os.environ, HOME=str(base / 'home'), XDG_CACHE_HOME=str(base / 'home'))
        launcher = base / 'launch.sh'
        launcher.write_text(
            '#!/bin/bash\nset -euo pipefail\ncd "$1"\n. "$2/amiga/env.sh"\nFSUAE_RUN=.\n. "$FSUAE_COMMON"\n'
            "trap 'fsuae_stop' EXIT\nFSUAE=\"$FSUAE_ARM\"\n"
            f'AMIGA_SETUP={setup} DEBUG=1 DEBUG_PORT="$3" SOUND=0 WARP=1\n'
            'fsuae_claim_port\nfsuae_options\n'
            'fsuae_launch --hard_drive_0="$PWD/boot" --hard_drive_1="$4" --hard_drive_1_read_only=1 '
            '--hard_drive_2="$PWD/out" --remote_debugger_trigger=DownwellInstallData --state_dir="$PWD/state" '
            '--floppy_drive_volume=0 --floppy_drive_volume_empty=0\nwait "$FSUAE_PID"\n')
        with (base / 'fsuae.log').open('w') as log:
            emu = subprocess.Popen(['/bin/bash', str(launcher), str(base), str(ROOT), str(port), str(ROOT / 'data')],
                                   stdout=log, stderr=log)
            try:
                for _ in range(300):
                    if emu.poll() is not None:
                        raise SystemExit('FS-UAE exited: ' + (base / 'fsuae.log').read_text()[-2000:])
                    if subprocess.run(['lsof', '-nP', f'-iTCP:{port}', '-sTCP:LISTEN'], capture_output=True).returncode == 0:
                        break
                    time.sleep(.2)
                started = time.time()
                r = subprocess.run(['m68k-amiga-elf-gdb', '-q', '-batch', '-x', str(base / 'timing.gdb'), str(elf)],
                                   env=env, capture_output=True, text=True, timeout=4 * 3600)
                if options.multiply_test:
                    found = [l for l in (r.stdout + r.stderr).splitlines() if l.startswith(('MULTIPLY', 'ADD'))]
                    print('\n'.join(found) if found else (r.stdout + r.stderr)[-2000:])
                    return 0 if len(found) == 2 and all(' failures=0 ' in l for l in found) else 1
                values = {}
                for line in (r.stdout + r.stderr).splitlines():
                    if line.startswith('TIMING'):
                        for field in line.split()[1:]:
                            k, v = field.split('=')
                            values[k] = int(v)
                if 'result' not in values:
                    raise SystemExit('no result:\n' + (r.stdout + r.stderr)[-3000:])
                ticks = [values[f'tick{i}'] for i in range(len(STAGES) + 1)]
                print(f'result {values["result"]}, peak allocation {values["peak"]:,} bytes, '
                      f'stack used {values["stack"] - values["unused"]:,} of {values["stack"]:,}; wall {time.time() - started:.0f} s')
                cache = values.get('cache', 0)
                names = [n for bit, n in ((1, 'instruction'), (0x100, 'data'), (0x2000, 'write-allocate'),
                                          (0x80000000, 'copyback')) if cache & bit]
                print(f'caches: {", ".join(names) or "none"} (CacheControl {cache:#x})')
                for i, name in enumerate(STAGES):
                    print(f'  {name:12s} {(ticks[i + 1] - ticks[i]) / 50:8.1f} s')
                print(f'  {"total":12s} {(ticks[-1] - ticks[0]) / 50:8.1f} s')
            finally:
                subprocess.run(['/bin/bash', '-c', '. "$1/amiga/env.sh"; FSUAE_RUN="$2"; . "$FSUAE_COMMON"; fsuae_stop_previous',
                                'stop', str(ROOT), str(base)], check=False)
                try:
                    emu.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    emu.kill(); emu.wait()
        if probe:
            return 0 if values['result'] == 0 else 1
        host = base / 'host'
        host.mkdir()
        subprocess.run([str(BUILD / 'DownwellInstallData'), str(ROOT / 'data'), str(host)], check=True, capture_output=True)
        names = sorted(p.name for p in host.iterdir())
        made = sorted(p.name for p in out.iterdir())
        same = [n for n in names if (out / n).exists() and filecmp.cmp(host / n, out / n, shallow=False)]
        print(f'{len(same)} of {len(names)} outputs equal the host converter\'s'
              + ('' if made == names else f'; target wrote {made}'))
        return 0 if values['result'] == 0 and len(same) == len(names) else 1


if __name__ == '__main__':
    sys.exit(main())
