#!/usr/bin/env python3
"""GDB driver for verify_startup.sh: normal binary, joystick events only.

Use GDB=../tools/verify-world-one-native.py with the shared arm64 emulator.
No target-memory writes, shortcuts, injected exits or altered health/RNG.
The host route file is indexed by simulation tick (Draw-only frames repeat).
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


class Debugger(Gdb):
    def cmd(self, command, timeout=30):
        # This toolchain's LTO debug info emits benign CU warnings even for
        # monitor commands; don't confuse those with a harness error reply.
        out=super().cmd(command,timeout)
        return re.sub(r'warning: \(Internal error: pc [^\n]*\)\n?', '', out)

    def __init__(self, port, elf):
        # Load symbols before connecting so the stub relocates each section.
        self.sent='@@WORLD-ONE@@'
        self.buf=b''
        self.running=False
        self.p=subprocess.Popen(['m68k-amiga-elf-gdb','-q','-nx',elf],
            stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,bufsize=0)
        for command in ('set pagination off','set confirm off','set height 0',
                        'set width 0','set remotetimeout 30'):
            self.cmd(command)
        self.cmd(f'target remote 127.0.0.1:{port}',timeout=60)


def main():
    script=Path(sys.argv[sys.argv.index('-x')+1]).read_text()
    port=int(re.search(r'target remote 127\.0\.0\.1:(\d+)',script)[1])
    tape=Path('../tmp/world-one-gameplay.inputs').read_bytes()
    witnesses={row[0]:row[1:] for row in
        (list(map(int,line.split())) for line in Path('../tmp/world-one-gameplay.states').read_text().splitlines())}
    g=Debugger(port,sys.argv[-1])
    log=Path('../tmp/world-one-native-inputs.log').open('w')
    try:
        def checked(command):
            out=g.cmd(command,timeout=60)
            out='\n'.join(line for line in out.splitlines() if not line.startswith('warning:'))
            if any(word in out for word in ('No symbol','not defined','Cannot','Error','error:')):
                raise RuntimeError(command+'\n'+out)
            return out
        checked("set $w=(GameplayWorld*)&'_ZL18portRuntimeStorage.lto_priv.0'")
        checked("set $s=(AmigaSceneRuntime*)&'runDownwell()::sceneStorage'")
        # Break at the instruction entry, before volatile device loads. A
        # source-line breakpoint can stop after those loads in optimized code.
        checked("break *'AmigaInput::sample'")
        checked('break downwellWorldOneComplete')
        # Optional evidence run: DOWNWELL_CAPTURE_TICK=4494 (with
        # READY_MARKER=DOWNWELL_WORLD_ONE_CAPTURE_READY) checks the route up to
        # that simulation tick, saves captures/world-one-<tick>, and stops.
        shotTick=int(os.environ.get('DOWNWELL_CAPTURE_TICK','-1'))
        def shot(name):
            capture=Path(f'../tmp/{name}.bin')
            profile(g,2,capture)
            kind,image=screenshot_from_trace(capture)
            extension='png' if image.startswith(b'\x89PNG') else 'jpg' if image.startswith(b'\xff\xd8') else None
            if extension is None: raise RuntimeError(f'unknown screenshot encoding {kind}')
            destination=Path(f'../captures/{name}.{extension}')
            destination.parent.mkdir(exist_ok=True)
            destination.write_bytes(image)
            capture.unlink()
        previous=-1
        sceneFrame=0
        oldScene=-1
        oldLevel=-1
        levelStart=-1
        deadline=time.monotonic()+1200
        with Input(g,log=lambda r: log.write(str(r)+'\n')) as controls:
            for frame in range(14000):
                if time.monotonic()>deadline:
                    raise TimeoutError('bounded world-one route')
                stop=checked('continue')
                if 'downwellWorldOneComplete' in stop:
                    state=checked('printf "END %d %d %u %d %u\\n", $w->run_.stage.area,$w->run_.stage.level,$w->tick_,$w->health_,(unsigned)$w->enemyRandom_.drawCount_')
                    print(state,flush=True)
                    if not re.search(r'END 2 1 '+str(len(tape))+r' [1-9]\d* \d+',state):
                        raise RuntimeError('wrong slice destination or route length')
                    checked('delete breakpoints')
                    controls.fields(2)
                    shot('world-one-end')
                    print('DOWNWELL_WORLD_ONE_READY',flush=True)
                    Path('.run/gdb-ready.marker').touch()
                    return
                if 'AmigaInput::sample' not in stop:
                    raise RuntimeError('unexpected native stop: '+stop)
                state=checked('printf "STATE %d %u %d %d %d %d %d\\n", $s->scene_,$w->tick_,$w->run_.stage.area,$w->run_.stage.level,$w->health_,$w->generationSucceeded_,$w->run_.levelResultRequested&&!$w->run_.levelClearActive')
                m=re.search(r'STATE (\d+) (\d+) (\d+) (\d+) (-?\d+) (\d+) (\d+)',state)
                if not m: raise RuntimeError(state)
                scene,tick,area,level,hp,good,resultPending=map(int,m.groups())
                if scene!=oldScene:
                    oldScene=scene;sceneFrame=0
                sceneFrame+=1
                if scene==0: packed=4 if sceneFrame%60==0 else 0
                elif scene==1: packed=2 if sceneFrame>40 else 0
                elif scene==2: packed=4 if sceneFrame==120 else 0
                else:
                    if hp<=0 or not good or tick>=len(tape): raise RuntimeError(state)
                    # The normal renderer retires gameplay before Result Create
                    # in a Draw-only iteration. Compare after that creation,
                    # where the host witness for the same tick is recorded.
                    if level!=oldLevel: oldLevel=level;levelStart=tick
                    # The host witness for a level's first tick is recorded
                    # after next-level creation; native samples input before
                    # scenes.step builds it. Later checks cover those draws.
                    # Diagnostic: DOWNWELL_DENSE_CHECK=from,to checks every tick
                    # in that window and prints the compact time-stop state.
                    dense=os.environ.get('DOWNWELL_DENSE_CHECK')
                    inDense=False
                    if dense:
                        lo,hi=map(int,dense.split(','))
                        inDense=lo<=tick<=hi
                        if inDense:
                            print(checked('printf "DENSE %u shards=%u speed=%d rng=%u\\n", $w->tick_,$w->timeStop_.shards.count_,$w->timeStop_.roomSpeed,(unsigned)$w->enemyRandom_.drawCount_'),flush=True)
                    if tick and (tick%16==0 or inDense) and not resultPending and tick!=levelStart:
                        observed=checked('printf "CHECK %d %ld %ld %d %u\\n", $w->run_.stage.level,$w->player_.x_,$w->player_.y_,$w->health_,(unsigned)$w->enemyRandom_.drawCount_')
                        match=re.search(r'CHECK ([\d -]+)',observed)
                        actual=list(map(int,match[1].split())) if match else []
                        if actual!=witnesses[tick]:
                            raise RuntimeError(f'first checked divergence tick={tick}: native={actual}, host={witnesses[tick]}')
                    packed=tape[tick]
                    if tick==shotTick:
                        # Evidence mode ends here: the capture runs the
                        # emulator past the following samples.
                        checked('delete breakpoints')
                        shot(f'world-one-{tick}')
                        print('DOWNWELL_WORLD_ONE_CAPTURE_READY',flush=True)
                        Path('.run/gdb-ready.marker').touch()
                        return
                if frame%100==0: print(state,flush=True)
                log.write(f'{frame} {scene} {tick} {level} {packed}\n');log.flush()
                for bit,event in ((1,'JOY2_LEFT'),(2,'JOY2_RIGHT'),(4,'JOY2_FIRE_BUTTON')):
                    if previous<0 or bool(previous&bit)!=bool(packed&bit):
                        controls.button(event,bool(packed&bit))
                previous=packed
        raise RuntimeError('route exhausted frame bound')
    finally:
        log.close()
        g.p.terminate()
        try: g.p.wait(timeout=5)
        except subprocess.TimeoutExpired: g.p.kill();g.p.wait()


if __name__=='__main__':
    main()
