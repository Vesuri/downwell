#!/usr/bin/env python3
"""Time DownwellInstallData on the emulated target (a1200-4m: cycle-exact
68EC020 at 14 MHz, 4 MB Fast), from the Shell with Stack 16384, over data/
mounted read-only. Prints each stage's time, the peak allocation, the stack
used and the result, and compares the outputs with the host converter's.

    make -C tools/install-data amiga-timing all
    exec 9>>tmp/amiga.lock; python3 tools/hold-lock.py 9; python3 tools/install-data/time_amiga.py
"""
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
ELF = BUILD / 'timing/DownwellInstallData.elf'
STAGES = ['inputs', 'data.win', 'text', 'rooms', 'masks', 'code', 'exe', 'art', 'derived', 'audio', 'soundtables']


def main():
    setup = sys.argv[1] if len(sys.argv) > 1 else 'a1200-4m'
    with tempfile.TemporaryDirectory(prefix='install-timing-', dir=ROOT / 'tmp') as tmp:
        base = Path(tmp)
        boot, out = base / 'boot', base / 'out'
        (boot / 's').mkdir(parents=True)
        out.mkdir()
        for d in ('state', 'home'):
            (base / d).mkdir()
        shutil.copyfile(BUILD / 'timing/DownwellInstallData.exe', boot / 'DownwellInstallData')
        (boot / 's/startup-sequence').write_text('Stack 16384\nDownwellInstallData DH1: DH2:\n')
        s = socket.socket(); s.bind(('127.0.0.1', 0)); port = s.getsockname()[1]; s.close()
        lines = ['set pagination off', 'set confirm off', 'set remotetimeout 120',
                 f'target remote 127.0.0.1:{port}', 'break installer_test_done', 'continue',
                 'printf "TIMING result=%d peak=%lu stack=%lu unused=%lu\\n", installer_result, install_peak, '
                 'installer_stack_size, installer_stack_unused']
        lines += [f'printf "TIMING tick{i}=%ld\\n", install_ticks[{i}]' for i in range(len(STAGES) + 1)]
        lines += ['detach', 'quit']
        (base / 'timing.gdb').write_text('\n'.join(lines) + '\n')
        env = dict(os.environ, HOME=str(base / 'home'), XDG_CACHE_HOME=str(base / 'home'))
        launcher = base / 'launch.sh'
        launcher.write_text(
            '#!/bin/bash\nset -euo pipefail\ncd "$1"\n. "$2/amiga/env.sh"\nFSUAE_RUN=.\n. "$FSUAE_COMMON"\n'
            "trap 'fsuae_stop' EXIT\nFSUAE=\"$FSUAE_ARM\"\n"
            f'AMIGA_SETUP={setup} DEBUG=1 DEBUG_PORT="$3" SOUND=0 WINDOW=none WARP=1\n'
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
                r = subprocess.run(['m68k-amiga-elf-gdb', '-q', '-batch', '-x', str(base / 'timing.gdb'), str(ELF)],
                                   env=env, capture_output=True, text=True, timeout=4 * 3600)
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
