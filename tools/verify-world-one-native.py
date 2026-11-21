#!/usr/bin/env python3
"""GDB driver for verify_startup.sh: normal binary, joystick events only.

Use GDB=../tools/verify-world-one-native.py with the shared arm64 emulator.
No target-memory writes, shortcuts, injected exits or altered health/RNG.
The host route file is indexed by simulation tick (Draw-only frames repeat).

W2.6 sound: at every input sample GDB also reads the Paula mixer's voice
table (each channel's active flag, asset and play serial). A new serial is a
sound the mixer started for the hardware. At the end the native starts are
compared with the host route's sound log (<tape>.sounds): no request may be
missing from the loaded banks, no pack load may fail, and every sound the
host plays must have started natively at least once (a play stolen or
expired before the next sample is not seen, so counts may differ).
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


def storageSymbol(stem):
    """The ELF's data symbol whose name is stem, with or without an LTO suffix."""
    objdump=os.environ.get('OBJDUMP','m68k-amiga-elf-objdump')
    table=subprocess.run([objdump,'-t',sys.argv[-1]],check=True,capture_output=True,text=True).stdout
    names=[line.split()[-1] for line in table.splitlines() if line.split() and
           re.fullmatch(re.escape(stem)+r'(\.lto_priv\.\d+)?',line.split()[-1])]
    if len(names)!=1: raise RuntimeError(f'expected one {stem} symbol, found {names}')
    return names[0]

def main():
    script=Path(sys.argv[sys.argv.index('-x')+1]).read_text()
    port=int(re.search(r'target remote 127\.0\.0\.1:(\d+)',script)[1])
    # DOWNWELL_ROUTE_TAPE selects another host tape (base path without the
    # .inputs/.states suffix). DOWNWELL_ROUTE_DEATH=1 expects that route to
    # die: after the death it waits for the results' unlock notice (seed the
    # lifetime gems with VERIFY_SAVE) and captures captures/world-one-unlock.
    # The hard-mode sound route (tmp/sound-route-hard-mode-death) needs a save
    # with hardUnlocked and option hardMode set, so style select starts hard,
    # and no gem seed (as on the host): it ends at the bar's completion sound
    # (READY_MARKER=DOWNWELL_WORLD_ONE_RESULTS_READY).
    base=os.environ.get('DOWNWELL_ROUTE_TAPE','../tmp/world-one-gameplay')
    deathRoute=os.environ.get('DOWNWELL_ROUTE_DEATH')=='1'
    tape=Path(base+'.inputs').read_bytes()
    witnesses={row[0]:row[1:] for row in
        (list(map(int,line.split())) for line in Path(base+'.states').read_text().splitlines())}
    deathSamples=0
    g=Debugger(port,sys.argv[-1])
    log=Path('../tmp/world-one-native-inputs.log').open('w')
    try:
        def checked(command):
            out=g.cmd(command,timeout=60)
            out='\n'.join(line for line in out.splitlines() if not line.startswith('warning:'))
            if any(word in out for word in ('No symbol','not defined','Cannot','Error','error:')):
                raise RuntimeError(command+'\n'+out)
            return out
        # LTO renames file-local statics (.lto_priv.N) from link to link.
        checked(f"set $w=(GameplayWorld*)&'{storageSymbol(r'_ZL18portRuntimeStorage')}'")
        checked(f"set $s=(AmigaSceneRuntime*)&'{storageSymbol(r'_ZZL11runDownwellvE12sceneStorage')}'")
        checked(f"set $a=(downwell::AmigaSoundBanks*)&'{storageSymbol(r'_ZZL11runDownwellvE16soundBankStorage')}'")
        voiceFormat='printf "VOICES'+' %d %d %u'*4+'\\n"'+''.join(
            f',$a->audio_.plan_.voices_[{c}].active,$a->audio_.plan_.voices_[{c}].asset,$a->audio_.plan_.voices_[{c}].serial' for c in range(4))
        seenSerials=set()
        nativeStarts={}
        # DOWNWELL_MEMORY_SAMPLE=1 (WR.1): the free bytes of every exec
        # memory region at each input sample; the low-water marks give the
        # game's peak Chip and public use. The first sample follows the
        # executable's load, so add its hunks for the footprint.
        memoryRegions=[]
        memoryLow={}
        if os.environ.get('DOWNWELL_MEMORY_SAMPLE')=='1':
            node=int(checked('printf "%lu\\n", *(unsigned long*)(*(unsigned long*)4+322)').split()[-1])
            while int(checked(f'printf "%lu\\n", *(unsigned long*){node}').split()[-1]):
                attributes=int(checked(f'printf "%u\\n", *(unsigned short*)({node}+14)').split()[-1])
                memoryRegions.append((node,attributes))
                node=int(checked(f'printf "%lu\\n", *(unsigned long*){node}').split()[-1])
            memoryFormat='printf "FREE'+' %lu'*len(memoryRegions)+'\\n"'+''.join(
                f',*(unsigned long*)({node}+28)' for node,_ in memoryRegions)
        def sampleMemory(tick,scene):
            if not memoryRegions: return
            free=list(map(int,re.search(r'FREE ([\d ]+)',checked(memoryFormat))[1].split()))
            for (node,attributes),value in zip(memoryRegions,free):
                low=memoryLow.get(node)
                if low is None: memoryLow[node]=[value,value,tick,scene,attributes]
                elif value<low[1]: low[1:4]=value,tick,scene
        def reportMemory():
            if not memoryRegions: return
            use={'chip':0,'other':0}
            for node,(first,low,tick,scene,attributes) in memoryLow.items():
                kind='chip' if attributes&2 else 'fast' if attributes&4 else 'other'
                use['chip' if kind=='chip' else 'other']+=first-low
                print(f'MEMORY {kind} region={node:08x} first={first} low={low} use={first-low} at tick={tick} scene={scene}',flush=True)
            chipNeed=int(checked('printf "%lu\\n", downwellChipRequirement').split()[-1])
            publicNeed=int(checked('printf "%lu\\n", downwellPublicRequirement').split()[-1])
            print(f"MEMORY peak chip={use['chip']} other={use['other']} declared chip={chipNeed} public={publicNeed}",flush=True)
            # Region low-water marks need not coincide, so their sum bounds
            # the peak from above. With Fast present Chip holds DMA data only.
            if use['other'] and use['chip']>chipNeed:
                raise RuntimeError('Chip use exceeds downwellChipRequirement')
            if use['chip']+use['other']>chipNeed+publicNeed:
                raise RuntimeError('memory use exceeds the declared requirements')
        def sampleVoices(tick):
            v=list(map(int,re.search(r'VOICES ([\d -]+)',checked(voiceFormat))[1].split()))
            for c in range(4):
                active,asset,serial=v[3*c:3*c+3]
                if active and serial not in seenSerials:
                    seenSerials.add(serial)
                    first,count=nativeStarts.get(asset,(tick,0))
                    nativeStarts[asset]=(first,count+1)
        def checkSounds():
            counters=checked('printf "SOUND plays=%u steals=%u drops=%u missing=%u music=%u expired=%u stops=%u\\n", $a->audio_.plan_.counters_.plays,$a->audio_.plan_.counters_.steals,$a->audio_.plan_.counters_.drops,$a->audio_.plan_.counters_.missing,$a->audio_.plan_.counters_.music,$a->audio_.plan_.counters_.expired,$a->audio_.plan_.counters_.stops')
            banks=checked('printf "BANKS ready=%u loads=%u failures=%u longest=%u lines\\n", gDownwellSoundReady,gDownwellSoundBankLoads,gDownwellSoundBankFailures,$a->audio_.longestService_')
            print(counters.strip(),flush=True);print(banks.strip(),flush=True)
            host={}
            for line in Path(base+'.sounds').read_text().splitlines():
                f=line.split()
                if f[1]=='play' and f[7]!='5': host[int(f[2])]=host.get(int(f[2]),0)+1
            unseen=sorted(a for a in host if a not in nativeStarts)
            extra=sorted(a for a in nativeStarts if a not in host)
            print(f'SOUND host sounds {len(host)} ({sum(host.values())} plays), native started {len(nativeStarts)} sounds '
                  f'({len(seenSerials)} starts seen); front-end or native-only {extra}',flush=True)
            for a in unseen: print(f'SOUND not seen natively: {a} (host plays {host[a]})',flush=True)
            if not re.search(r'missing=0 ',counters): raise RuntimeError('sound requests missing from the loaded banks')
            if not re.search(r'ready=1 .*failures=0 ',banks): raise RuntimeError('sound banks not ready or a pack failed')
            if unseen: raise RuntimeError(f'{len(unseen)} host sounds never started natively')
        # Break at the instruction entry, before volatile device loads. A
        # source-line breakpoint can stop after those loads in optimized code.
        checked("break *'AmigaInput::sample'")
        checked('break downwellWorldOneComplete')
        checked('break downwellGameplayFailed')
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
        deadline=time.monotonic()+int(os.environ.get('DOWNWELL_ROUTE_SECONDS','1200'))
        with Input(g,log=lambda r: log.write(str(r)+'\n')) as controls:
            for frame in range(20000):
                if time.monotonic()>deadline:
                    raise TimeoutError('bounded world-one route')
                stop=checked('continue')
                if 'downwellWorldOneComplete' in stop:
                    state=checked('printf "END %d %d %u %d %u\\n", $w->run_.stage.area,$w->run_.stage.level,$w->tick_,$w->health_,(unsigned)$w->enemyRandom_.drawCount_')
                    print(state,flush=True)
                    reportMemory()
                    checkSounds()
                    if not re.search(r'END 2 1 '+str(len(tape))+r' [1-9]\d* \d+',state):
                        raise RuntimeError('wrong slice destination or route length')
                    checked('delete breakpoints')
                    controls.fields(2)
                    shot('world-one-end')
                    print('DOWNWELL_WORLD_ONE_READY',flush=True)
                    Path('.run/gdb-ready.marker').touch()
                    return
                if 'downwellGameplayFailed' in stop:
                    for probe in ('print gDownwellStreakFailure','print gDownwellStreakFailureView','print $w->gemHud_','print $w->currencyAccount_','print streakYOffset'):
                        print(checked(probe),flush=True)
                    raise RuntimeError(checked('printf "GAMEPLAY FAIL line=%u tick=%u actor=%u/%u\\n", line,$w->tick_,gDownwellActorFailureSprite,gDownwellActorFailureFrame'))
                if 'AmigaInput::sample' not in stop:
                    raise RuntimeError('unexpected native stop: '+stop)
                state=checked('printf "STATE %d %u %d %d %d %d %d\\n", $s->scene_,$w->tick_,$w->run_.stage.area,$w->run_.stage.level,$w->health_,$w->generationSucceeded_,$w->run_.levelResultRequested&&!$w->run_.levelClearActive')
                m=re.search(r'STATE (\d+) (\d+) (\d+) (\d+) (-?\d+) (\d+) (\d+)',state)
                if not m: raise RuntimeError(state)
                scene,tick,area,level,hp,good,resultPending=map(int,m.groups())
                sampleVoices(tick)
                sampleMemory(tick,scene)
                if scene!=oldScene:
                    oldScene=scene;sceneFrame=0
                sceneFrame+=1
                if scene==0: packed=4 if sceneFrame%60==0 else 0
                elif scene==1: packed=2 if sceneFrame>40 else 0
                elif scene==2: packed=4 if sceneFrame==120 else 0
                elif deathRoute and hp<=0:
                    # Hands off: the results sequence fills the progress bar.
                    packed=0
                    deathSamples+=1
                    notice=checked('printf "NOTICE %d %u\\n", $w->run_.results.menu.goalNotice,$w->run_.results.progress.goal')
                    if deathSamples%50==0: print(notice,flush=True)
                    stage=int(re.search(r'NOTICE (-?\d+)',notice)[1])
                    if 330 in nativeStarts:
                        reportMemory()
                        # Without a gem seed no goal is reached: the bar's
                        # completion sound (sfxUiRetry 330) ends the results.
                        checkSounds()
                        print('DOWNWELL_WORLD_ONE_RESULTS_READY',flush=True)
                        Path('.run/gdb-ready.marker').touch()
                        return
                    if stage>=3:
                        reportMemory()
                        checkSounds()
                        for _ in range(20): checked('continue')
                        checked('delete breakpoints')
                        shot('world-one-unlock')
                        print(notice,flush=True)
                        print('DOWNWELL_WORLD_ONE_UNLOCK_READY',flush=True)
                        Path('.run/gdb-ready.marker').touch()
                        return
                    if deathSamples>3000: raise RuntimeError('no unlock notice: '+notice)
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
                        observed=checked('printf "CHECK %d %ld %ld %d %u %u %d %u\\n", $w->run_.stage.level,$w->player_.x_,$w->player_.y_,$w->health_,(unsigned)$w->enemyRandom_.drawCount_,$w->currencyAccount_.currency,$w->run_.shops.noControl,$w->shopSpeechLive_')
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
