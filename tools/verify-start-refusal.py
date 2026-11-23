#!/usr/bin/env python3
"""GDB driver for verify_startup.sh: a start that refuses says why (WI.6).

    GDB=../tools/verify-start-refusal.py READY_MARKER=DOWNWELL_REFUSAL_READY \\
        VERIFY_REFUSAL='Cannot read data/Downwell.shop: reinstall Downwell.' \\
        [VERIFY_WORKBENCH=1] [VERIFY_SHELL_LOG=1] bash verify_startup.sh

verify_startup.sh arranges the cause (VERIFY_DATA_MISSING, VERIFY_DATA_SHORT,
VERIFY_DATA_CORRUPT or a MACHINE with too little memory). The program must
stop at AmigaStart.c's downwellRefuse with a message that VERIFY_REFUSAL (a
Python regular expression) matches whole, then return 20.
- From the Shell (VERIFY_SHELL_LOG=1: `Downwell >DH0:shell.log`, then
  `Echo >>DH0:shell.log "RC $RC"`), the host file must hold the message line
  and the Shell must see result 20.
- From Workbench, the requester must open: a window titled Downwell on the
  Workbench screen (its picture goes to captures/). Left-Amiga+B answers
  it; the result is 20 and the WBStartup message is held for the reply.
"""
import os
from pathlib import Path
import re
import signal
import subprocess
import sys
import time

sys.path.insert(0, os.environ['AMIGA_PYLIB'])
from amigaxdev.gdbserve import Gdb, profile, screenshot_from_trace
from amigaxdev.input import Input



class Debugger(Gdb):
    def cmd(self, command, timeout=60):
        out = super().cmd(command, timeout)
        return re.sub(r'warning: \(Internal error: pc [^\n]*\)\n?', '', out)

    def __init__(self, port, elf):
        self.sent = '@@START@@'
        self.buf = b''
        self.running = False
        self.p = subprocess.Popen(['m68k-amiga-elf-gdb', '-q', '-nx', elf],
            stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, bufsize=0)
        for command in ('set pagination off', 'set confirm off', 'set height 0',
                        'set width 0', 'set remotetimeout 30', 'set print elements 0'):
            self.cmd(command)
        self.cmd(f'target remote 127.0.0.1:{port}', timeout=60)


def main():
    script = Path(sys.argv[sys.argv.index('-x') + 1]).read_text()
    port = int(re.search(r'target remote 127\.0\.0\.1:(\d+)', script)[1])
    workbench = os.environ.get('VERIFY_WORKBENCH') == '1'
    expected = os.environ['VERIFY_REFUSAL']
    case = os.environ.get('VERIFY_CASE', 'refusal')
    g = Debugger(port, sys.argv[-1])
    try:
        def checked(command, timeout=60):
            out = g.cmd(command, timeout)
            out = '\n'.join(l for l in out.splitlines() if not l.startswith('warning:'))
            if any(w in out for w in ('No symbol', 'not defined', 'Cannot', 'Error', 'error:')):
                raise RuntimeError(command + '\n' + out)
            return out

        def word(address):
            out = checked(f'printf "W %u\\n", *(unsigned long*){address}')
            return int(re.search(r'W (\d+)', out)[1])

        def text(address):
            out = g.cmd(f'printf "S<%s>\\n", (char*){address}')
            return re.search(r'S<(.*)>', out, re.S)[1]

        checked("break *'downwellRefuse'")
        checked("break *'downwellStartReturn'")
        stop = g.cmd('continue', timeout=int(os.environ.get('START_SECONDS', '180')))
        if 'downwellRefuse' not in stop: raise RuntimeError('no refusal: ' + stop)
        # At the entry (break *): the argument is above the return address.
        message = text(word('($sp+4)'))
        print(f'REFUSAL {message}', flush=True)
        if not re.fullmatch(expected, message): raise RuntimeError(f'refusal {message!r}, expected {expected!r}')
        checked("delete 1")
        if workbench:
            # ExecBase LibList (offset 378) to intuition.library; its
            # FirstScreen (60), each Screen's FirstWindow (4), each Window's
            # NextWindow (0) and Title (32); ActiveWindow (52).
            def windows(intuition):
                screen = word(intuition + 60)
                while screen:
                    window = word(screen + 4)
                    while window:
                        address = word(window + 32)
                        yield window, text(address) if address else ''
                        window = word(window)
                    screen = word(screen)
            # Short steps only: the run is warped, and Workbench asks whether
            # to wait for a SYS:WBStartup program that has not returned after
            # some emulated seconds. A player's double-click has no such wait.
            def intuition_base():
                node = word(word(4) + 378)
                while node and word(node):
                    if text(word(node + 10)) == 'intuition.library': return node
                    node = word(node)
                raise RuntimeError('intuition.library not found')

            def requesters():
                return [w for w, title in windows(intuition_base()) if title == 'Downwell']

            def titles():
                return [title for _, title in windows(intuition_base())]

            def picture(suffix):
                capture = Path('../tmp/refusal-requester.bin')
                profile(g, 2, capture)
                kind, image = screenshot_from_trace(capture)
                Path('../captures').mkdir(exist_ok=True)
                path = Path(f'../captures/refusal-{case}{suffix}.' + ('png' if image.startswith(b'\x89PNG') else 'jpg'))
                path.write_bytes(image)
                return path

            found = []
            for attempt in range(40):
                g.cont(); time.sleep(0.25); g.stop()
                found = requesters()
                if found: break
            if not found: raise RuntimeError('no requester titled Downwell')
            requester = found[0]
            before = [title for title in titles() if title != 'Downwell']
            print(f'REQUESTER {requester:#x} windows {titles()} picture {picture("")}', flush=True)
            # The player answers OK. EasyRequest's rightmost gadget (its only
            # one here) also answers Left-Amiga+B (raw 0x66, 0x35) while the
            # requester is the active window (IntuitionBase ActiveWindow, 52),
            # which it is when it opens; no pointer position is involved.
            active = word(intuition_base() + 52)
            if active != requester:
                raise RuntimeError(f'active window {active:#x}, not the requester {requester:#x}')
            # Only Left-Amiga's press runs fields (monitor fields): GDB plants
            # breakpoints only when it resumes the target itself, so the
            # program must not reach downwellStartReturn inside a monitor
            # step. B's press and both releases are queued, and the continue
            # below delivers them with the breakpoint planted.
            with Input(g) as controls:
                controls.key(0x66, 1); controls.fields(2)
                controls.key(0x35, 1); controls.key(0x35, 0); controls.key(0x66, 0)
            print('ANSWER Left-Amiga+B', flush=True)
        try:
            stop = g.cmd('continue', timeout=60)
        except TimeoutError:
            # Show where the program is instead of only timing out.
            os.kill(g.p.pid, signal.SIGINT); time.sleep(2); g.buf = b''
            print('NO RETURN at', g.cmd('info symbol $pc'), flush=True)
            raise
        if 'downwellStartReturn' not in stop: raise RuntimeError('no return: ' + stop)
        state = checked('printf "START result=%d message=%d\\n", *(int*)($sp+4), gDownwellWorkbenchStartup != 0')
        print(state, flush=True)
        m = re.search(r'START result=(-?\d+) message=(\d)', state)
        if not m: raise RuntimeError(state)
        if int(m[1]) != 20: raise RuntimeError(f'result {m[1]}, expected 20')
        if int(m[2]) != int(workbench): raise RuntimeError(f'message {m[2]} for this start')
        if workbench:
            # After the reply under Forbid() the process ends and Workbench
            # has its message back: the CPU is outside the program, and only
            # Workbench's own windows are left (no "has not yet returned").
            checked('delete breakpoints')
            g.cont(); time.sleep(3); g.stop()
            where = g.cmd('info symbol $pc')
            left = titles()
            print(f'AFTER pc {where.strip()!r} windows {left} picture {picture("-after")}', flush=True)
            if 'No symbol matches' not in where:
                raise RuntimeError('still in the program after the reply: ' + where)
            if len(left) != len(before) or 'Downwell' in left:
                raise RuntimeError(f'windows {left} after the reply, {before} before the requester')
        else:
            # The Shell carries on: the redirected output and $RC reach the host.
            checked('delete breakpoints')
            log = Path('.run/dh0/shell.log')
            g.cont()
            deadline = time.time() + 30
            while time.time() < deadline:
                if log.exists() and 'RC ' in log.read_text(errors='replace'): break
                time.sleep(0.5)
            g.stop()
            lines = log.read_text(errors='replace').splitlines() if log.exists() else []
            print('SHELL ' + ' | '.join(lines), flush=True)
            if len(lines) != 2 or not re.fullmatch(expected, lines[0]) or lines[1] != 'RC 20':
                raise RuntimeError(f'Shell output {lines!r}, expected {[expected, "RC 20"]!r}')
        print('DOWNWELL_REFUSAL_READY', flush=True)
        Path('.run/gdb-ready.marker').touch()
    finally:
        g.p.terminate()
        try: g.p.wait(timeout=5)
        except subprocess.TimeoutExpired: g.p.kill(); g.p.wait()


if __name__ == '__main__':
    main()
