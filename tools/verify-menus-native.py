#!/usr/bin/env python3
"""GDB driver for verify_startup.sh: W1.10 menus on the normal binary.

Use GDB=../tools/verify-menus-native.py with VERIFY_SAVE pointing at a seeded
save.ini (style 1 unlocked, hard mode unlocked). Joystick and keyboard events
only; GDB reads state, it never writes target memory. The route:
style select (right to style 1, confirm), gameplay, P opens the pause menu,
PALETTE (next palette), OPTIONS (Timer on, volume 6 to 5), resume and watch
the timer, then pause again and QUIT through the confirm menu. W2.6: GDB
samples the Paula mixer's voices at every input sample; the front end's and
the pause menu's sounds must each start natively, the volume press must set
the mixer's master gain to 0.5 and the save's masterGain to 0.5. Captures land in
captures/menus-<name>.<ext>; the written .run/dh1/save.ini is checked after
the program exits.
"""
import os
from pathlib import Path
import re
import subprocess
import sys
import time

sys.path.insert(0, os.environ['AMIGA_PYLIB'])
from amigaxdev.gdbserve import Gdb, profile, screenshot_from_trace
from amigaxdev.input import Input

KEY_P = 0x19
KEY_ESCAPE = 0x45
SCREENS = {'closed': 0, 'pause': 1, 'confirm': 2, 'palette': 3, 'stats': 4, 'options': 5}


class Debugger(Gdb):
    def cmd(self, command, timeout=30):
        out = super().cmd(command, timeout)
        return re.sub(r'warning: \(Internal error: pc [^\n]*\)\n?', '', out)

    def __init__(self, port, elf):
        self.sent = '@@MENUS@@'
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
    g = Debugger(port, sys.argv[-1])
    try:
        def checked(command):
            out = g.cmd(command, timeout=60)
            out = '\n'.join(line for line in out.splitlines() if not line.startswith('warning:'))
            if any(word in out for word in ('No symbol', 'not defined', 'Cannot', 'Error', 'error:')):
                raise RuntimeError(command + '\n' + out)
            return out

        def shot(name):
            capture = Path(f'../tmp/menus-{name}.bin')
            profile(g, 2, capture)
            kind, image = screenshot_from_trace(capture)
            extension = 'png' if image.startswith(b'\x89PNG') else 'jpg' if image.startswith(b'\xff\xd8') else None
            if extension is None: raise RuntimeError(f'unknown screenshot encoding {kind}')
            destination = Path(f'../captures/menus-{name}.{extension}')
            destination.parent.mkdir(exist_ok=True)
            destination.write_bytes(image)
            capture.unlink()
            print('SHOT', destination, flush=True)

        checked("set $w=(GameplayWorld*)&'_ZL18portRuntimeStorage.lto_priv.0'")
        checked("set $s=(AmigaSceneRuntime*)&'runDownwell()::sceneStorage'")
        checked("set $m=(downwell::PauseMenu*)&'_ZL9pauseMenu.lto_priv.0'")
        checked("set $a=(downwell::AmigaSoundBanks*)&'_ZZL11runDownwellvE16soundBankStorage.lto_priv.0'")
        checked("break *'AmigaInput::sample'")
        checked('break downwellGameplayFailed')
        voiceFormat = 'printf "VOICES' + ' %d %d %u' * 4 + '\\n"' + ''.join(
            f',$a->audio_.plan_.voices_[{c}].active,$a->audio_.plan_.voices_[{c}].asset,$a->audio_.plan_.voices_[{c}].serial'
            for c in range(4))
        seenSerials = set()
        started = {}
        def sampleVoices():
            v = list(map(int, re.search(r'VOICES ([\d -]+)', checked(voiceFormat))[1].split()))
            for c in range(4):
                active, asset, serial = v[3 * c:3 * c + 3]
                if active and serial not in seenSerials:
                    seenSerials.add(serial); started[asset] = started.get(asset, 0) + 1
        # Style select right (331) and confirm (332), the menu entry's fall
        # (360); pause open (326), cursor (324), palette step (338), palette
        # back (318), enter a sub-menu (322) and an option toggle (321).
        def advance():
            # Every extra stop is an input sample too: read the voices there.
            stop = checked('continue')
            if 'AmigaInput::sample' in stop: sampleVoices()
            return stop
        expectedSounds = {331, 332, 360, 326, 324, 338, 318, 322, 321}
        masterChecked = False

        # Plan: (expected screen, wanted cursor, final press) steps after P.
        plan = [
            ('pause', 3, 'fire', None),            # PALETTE
            ('palette', 0, 'right', 'palette'),    # next palette, recoloured
            ('palette', 0, 'fire', None),          # back to pause (cursor 3)
            ('pause', 5, 'fire', None),            # OPTIONS
            ('options', 2, 'fire', 'options'),     # Timer: ON
            ('options', 3, 'fire', None),          # volume 0.6 -> 0.5
            ('options', 0, 'fire', None),          # back (cursor 5)
            ('pause', 4, 'fire', 'pause'),         # STATS
            ('stats', 0, 'fire', 'stats'),         # back (cursor 4)
            ('pause', 0, 'fire', None),            # RESUME
        ]
        quit_plan = [
            ('pause', 6, 'fire', None),            # QUIT
            ('confirm', 0, 'fire', '!confirm'),    # YES: game_end (shot first)
        ]
        sceneFrame = oldScene = 0
        stage = 'boot'
        step = 0
        pending = []      # queued (events, pressed) per sample
        playTicks = 0
        resumedAt = None
        shotsTaken = set()
        deadline = time.monotonic() + int(os.environ.get('DOWNWELL_ROUTE_SECONDS', '900'))
        controls = Input(g).__enter__()
        try:
            held = {'JOY2_LEFT': False, 'JOY2_RIGHT': False, 'JOY2_FIRE_BUTTON': False}
            def joystick(left=False, right=False, fire=False):
                for event, value in (('JOY2_LEFT', left), ('JOY2_RIGHT', right), ('JOY2_FIRE_BUTTON', fire)):
                    if held[event] != value:
                        controls.button(event, value); held[event] = value
            for frame in range(20000):
                if time.monotonic() > deadline:
                    raise TimeoutError('bounded menus route')
                try:
                    stop = checked('continue')
                except (EOFError, OSError, TimeoutError):
                    stop = ''
                    if stage != 'quitting':
                        import signal
                        g.p.send_signal(signal.SIGINT)
                        time.sleep(2)
                        print('HANG', stage, g.cmd('bt 12', timeout=30), flush=True)
                        print(g.cmd('info registers pc sr', timeout=30), flush=True)
                        raise
                if stage == 'quitting' and 'AmigaInput::sample' not in stop:
                    break
                if 'downwellGameplayFailed' in stop:
                    raise RuntimeError(checked('printf "GAMEPLAY FAIL line=%u\\n", line'))
                if 'AmigaInput::sample' not in stop:
                    raise RuntimeError('unexpected native stop: ' + stop)
                state = checked('printf "STATE %d %u %d %d %d %d\\n", $s->scene_,$w->tick_,(int)$m->screen,$m->cursor,$s->styleCursor_,$w->health_')
                m = re.search(r'STATE (\d+) (\d+) (\d+) (-?\d+) (-?\d+) (-?\d+)', state)
                if not m: raise RuntimeError(state)
                scene, tick, screen, cursor, styleCursor, hp = map(int, m.groups())
                sampleVoices()
                if scene != oldScene: oldScene = scene; sceneFrame = 0
                sceneFrame += 1
                if frame % 100 == 0: print(state, stage, step, flush=True)
                if scene == 0:
                    joystick(fire=sceneFrame % 60 == 0); continue
                if scene == 1:
                    joystick(right=sceneFrame > 40); continue
                if scene == 2:
                    # Style 1 is unlocked: right once, then confirm.
                    if sceneFrame == 90 and 'style' not in shotsTaken:
                        joystick(); shot('style0'); shotsTaken.add('style')
                    joystick(right=sceneFrame == 100,
                             fire=sceneFrame == 160 and styleCursor == 1)
                    if sceneFrame == 150: shot('style1')
                    if sceneFrame > 400: raise RuntimeError('style select did not start the run')
                    continue
                # Gameplay: stand still; the menus need no movement.
                if hp <= 0: raise RuntimeError('died during the menus route: ' + state)
                if stage == 'boot':
                    playTicks += 1
                    joystick()
                    if playTicks == 90:
                        controls.key(KEY_P, True); stage = 'opening'
                    continue
                if stage == 'opening':
                    controls.key(KEY_P, False)
                    if screen == SCREENS['pause']:
                        # Mid-slide: an animating screen must never be empty.
                        shot('pause-open'); stage = 'menu'; step = 0
                    continue
                if stage in ('menu', 'quitmenu'):
                    steps = plan if stage == 'menu' else quit_plan
                    if any(held.values()):
                        joystick(); continue   # release between presses
                    want, wantCursor, press, name = steps[step]
                    if screen != SCREENS[want]:
                        raise RuntimeError(f'expected {want}, state {state}')
                    if cursor != wantCursor:
                        joystick(right=True); continue
                    if name and name.startswith('!'):
                        for _ in range(30): advance()
                        shot(name[1:]); name = None
                    if press == 'right': joystick(right=True)
                    else: joystick(fire=True)
                    step += 1
                    if name:
                        # Let the press land and the overlay compose first.
                        stop = advance(); joystick()
                        for _ in range(3): advance()
                        shot(name)
                    if step == len(steps):
                        if stage == 'menu': stage = 'resumed'; resumedAt = tick
                        else: stage = 'quitting'
                    continue
                if stage == 'resumed':
                    joystick()
                    if screen != SCREENS['closed']: raise RuntimeError('pause did not close: ' + state)
                    if tick - resumedAt == 40: shot('timer')
                    if not masterChecked:
                        master = checked('printf "MASTER %u\\n", $a->audio_.plan_.master_')
                        print(master.strip(), flush=True)
                        if 'MASTER 32768' not in master: raise RuntimeError('volume 0.5 is not the mixer master gain')
                        masterChecked = True
                    if tick - resumedAt == 60:
                        controls.key(KEY_P, True); stage = 'reopening'
                    continue
                if stage == 'reopening':
                    controls.key(KEY_P, False)
                    if screen == SCREENS['pause']: stage = 'quitmenu'; step = 0
                    continue
                if stage == 'quitting':
                    continue
        finally:
            # After QUIT the target is gone: nothing is left to release.
            if stage != 'quitting': controls.release_all()
        if stage != 'quitting':
            raise RuntimeError('route did not reach QUIT: ' + stage)
        print('SOUND started natively:', ' '.join(f'{a}x{n}' for a, n in sorted(started.items())), flush=True)
        if expectedSounds - started.keys():
            raise RuntimeError(f'menu sounds never started natively: {sorted(expectedSounds - started.keys())}')
        # W2.6: the garden's foliage footsteps (51-54) and nap-wake landing (69-72).
        for name, group in (('garden footstep', range(51, 55)), ('garden landing', range(69, 73))):
            if not started.keys() & set(group):
                raise RuntimeError(f'no {name} started natively')
        save = Path('.run/dh1/save.ini')
        deadline = time.monotonic() + 20
        while time.monotonic() < deadline and not save.exists(): time.sleep(0.5)
        text = save.read_text() if save.exists() else ''
        print(text, flush=True)
        for key, value in (('style', '1.000000'), ('shader', '1.000000'), ('showtimer', '1.000000'),
                           ('recordFurthestReached', '11.000000'), ('masterGain', '0.500000')):
            if f'{key}="{value}"' not in text:
                raise RuntimeError(f'save.ini lacks {key}={value}')
        print('DOWNWELL_MENUS_READY', flush=True)
        Path('.run/gdb-ready.marker').touch()
    finally:
        g.p.terminate()
        try: g.p.wait(timeout=5)
        except subprocess.TimeoutExpired: g.p.kill(); g.p.wait()


if __name__ == '__main__':
    main()
