#!/usr/bin/env python3
"""GDB driver for verify_startup.sh: the program's start and exit.

    GDB=../tools/verify-workbench-start.py READY_MARKER=DOWNWELL_START_READY \\
        [VERIFY_WORKBENCH=1] bash verify_startup.sh

Lets the normal build run 100 input samples (on at least 8192 bytes of
stack, AmigaStart.c swapping in its own for a smaller VERIFY_STACK_BYTES),
then presses Escape, which
leaves the game outside gameplay with result 0. At AmigaStart.c's
downwellStartReturn seam it reads the result DOS or Workbench gets and
whether a WBStartup message is held for the reply. With VERIFY_WORKBENCH=1
(started from the icon) the message must be present, from the Shell absent.
After the reply under Forbid() the process must end: Workbench gets its
message back and the emulator keeps running Workbench.
"""
import os
from pathlib import Path
import re
import subprocess
import sys

sys.path.insert(0, os.environ['AMIGA_PYLIB'])
from amigaxdev.gdbserve import Gdb, profile, screenshot_from_trace
from amigaxdev.input import Input

ESCAPE = 0x45


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
                        'set width 0', 'set remotetimeout 30'):
            self.cmd(command)
        self.cmd(f'target remote 127.0.0.1:{port}', timeout=60)


def main():
    script = Path(sys.argv[sys.argv.index('-x') + 1]).read_text()
    port = int(re.search(r'target remote 127\.0\.0\.1:(\d+)', script)[1])
    workbench = os.environ.get('VERIFY_WORKBENCH') == '1'
    g = Debugger(port, sys.argv[-1])
    try:
        def checked(command, timeout=60):
            out = g.cmd(command, timeout)
            out = '\n'.join(l for l in out.splitlines() if not l.startswith('warning:'))
            if any(w in out for w in ('No symbol', 'not defined', 'Cannot', 'Error', 'error:')):
                raise RuntimeError(command + '\n' + out)
            return out
        checked("break *'downwellStartReturn'")
        checked("break *'AmigaInput::sample'")
        # The first input sample: the scene loop runs, the keyboard is ready.
        try:
            stop = checked('continue', timeout=int(os.environ.get('START_SECONDS', '180')))
        except TimeoutError:
            # Show what the machine is doing: interrupt and save a screenshot.
            import signal, time
            os.kill(g.p.pid, signal.SIGINT); time.sleep(2); g.buf = b''
            capture = Path('../tmp/workbench-start.bin')
            profile(g, 2, capture)
            kind, image = screenshot_from_trace(capture)
            Path('../captures').mkdir(exist_ok=True)
            Path('../captures/workbench-start.' + ('png' if image.startswith(b'\x89PNG') else 'jpg')).write_bytes(image)
            raise
        if 'AmigaInput::sample' not in stop: raise RuntimeError('no input sample: ' + stop)
        # WI.6: the game runs on at least 8192 bytes of stack, whatever the
        # Shell or icon gave (VERIFY_STACK_BYTES): ExecBase ThisTask (276),
        # its tc_SPLower (58) and tc_SPUpper (62), with sp between them.
        stack = checked('printf "STACK %u %u %u\\n", *(unsigned long*)(*(unsigned long*)(*(unsigned long*)4+276)+58), '
                        '*(unsigned long*)(*(unsigned long*)(*(unsigned long*)4+276)+62), $sp')
        print(stack, flush=True)
        lower, upper, sp = map(int, re.search(r'STACK (\d+) (\d+) (\d+)', stack).groups())
        if upper - lower < 8192 or not lower <= sp < upper:
            raise RuntimeError(f'game stack {upper - lower} bytes, sp {sp:#x} outside {lower:#x}..{upper:#x}')
        # Keys are delivered while the game runs between input samples, as in
        # verify-menus-native.py: press at one sample, release three later.
        with Input(g) as controls:
            for sample in range(200):
                if sample == 100: controls.key(ESCAPE, 1)
                if sample == 103: controls.key(ESCAPE, 0)
                stop = checked('continue', timeout=120)
                if 'downwellStartReturn' in stop: break
                if 'AmigaInput::sample' not in stop: raise RuntimeError('unexpected stop: ' + stop)
            else:
                raise RuntimeError('Escape did not end the program')
        # At the entry (break *): the argument is above the return address.
        state = checked('printf "START result=%d message=%d\\n", *(int*)($sp+4), gDownwellWorkbenchStartup != 0')
        print(state, flush=True)
        m = re.search(r'START result=(-?\d+) message=(\d)', state)
        if not m: raise RuntimeError(state)
        result, message = int(m[1]), int(m[2])
        if result != 0: raise RuntimeError(f'result {result}, expected 0')
        if message != int(workbench):
            raise RuntimeError(f'message {message}, expected {int(workbench)} for a '
                               + ('Workbench' if workbench else 'Shell') + ' start')
        if workbench:
            # Past the reply the task must be gone: Workbench unloads the
            # code, so a stop inside it afterwards would be a fault.
            checked('delete breakpoints')
            import time
            g.cont(); time.sleep(5); g.stop()
            pc = checked('printf "PC %#x\\n", $pc')
            where = g.cmd('info symbol $pc')
            print(pc, where, flush=True)
            if 'No symbol matches' not in where:
                raise RuntimeError('still in the program after the reply: ' + where)
        print('DOWNWELL_START_READY', flush=True)
        Path('.run/gdb-ready.marker').touch()
    finally:
        g.p.terminate()
        try: g.p.wait(timeout=5)
        except subprocess.TimeoutExpired: g.p.kill(); g.p.wait()


if __name__ == '__main__':
    main()
