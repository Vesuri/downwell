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
                        'set width 0','set remotetimeout 30','set language c++'):
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
    retryRoute=deathRoute and os.environ.get('DOWNWELL_ROUTE_RETRY')=='1'
    retrySamples=-1;retryLive=0
    # DOWNWELL_ROUTE_CENSUS=1 (W3.6): a CENSUS=1 FPSCOUNT build; read the
    # per-update work census once where the route ends (census_dump.gdb).
    # The stops at every input sample halt the whole emulated machine, beam
    # included, so they add no work to the measured updates.
    census=os.environ.get('DOWNWELL_ROUTE_CENSUS')=='1'
    # DOWNWELL_ROUTE_TRACE=N (W3.7, a TRACE_FRAMES build): at the input
    # sample of gameplay update N-1, after its input, record
    # DOWNWELL_ROUTE_TRACE_FIELDS whole fields of every instruction into
    # DOWNWELL_ROUTE_TRACE_OUTPUT/trace-<N-1>.bin, then end the route
    # (tools/amiga-trace-report.py excludes the partial first and last
    # updates). No input changes during the capture, so update N gets
    # update N-1's input; the driver reports when the tape differs there.
    traceFrame=int(os.environ.get('DOWNWELL_ROUTE_TRACE','0'))
    traceFields=int(os.environ.get('DOWNWELL_ROUTE_TRACE_FIELDS','20'))
    traceOutput=os.environ.get('DOWNWELL_ROUTE_TRACE_OUTPUT','')
    if traceFrame and (traceFrame<3 or not traceOutput):
        raise RuntimeError('DOWNWELL_ROUTE_TRACE needs a gameplay update >= 3 and DOWNWELL_ROUTE_TRACE_OUTPUT')
    gardenShot=os.environ.get('DOWNWELL_GARDEN_SHOT')=='1'
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
            if any(word in out for word in ('No symbol','not defined','Cannot','Error','error:','syntax error')):
                raise RuntimeError(command+'\n'+out)
            return out
        # LTO renames file-local statics (.lto_priv.N) from link to link.
        checked(f"set $w=(GameplayWorld*)&'{storageSymbol(r'_ZL18portRuntimeStorage')}'")
        checked(f"set $s=(AmigaSceneRuntime*)&'{storageSymbol(r'_ZZL11runDownwellvE12sceneStorage')}'")
        checked(f"set $a=(downwell::AmigaSoundBanks*)&'{storageSymbol(r'_ZZL11runDownwellvE16soundBankStorage')}'")
        voiceFormat='printf "VOICES'+' %d %d %u'*4+'\\n"'+''.join(
            f',$a->audio_.plan_.voices_[{c}].active,$a->audio_.plan_.voices_[{c}].asset,$a->audio_.plan_.voices_[{c}].serial' for c in range(4))
        def dumpCensus():
            if not census: return
            print(checked('source census_dump.gdb').strip(),flush=True)
            # The VBlank count is independent of the ring's beam clock.
            print(checked('printf "CENSUS fields gameplay frames=%lu fields=%lu\\n", gDownwellProfileFrames[3],gDownwellProfileFields[3]').strip(),flush=True)
            if os.environ.get('EFFECT_BLIT_CENSUS') in ('1','2'):
                # Section 5 C: the parallel effect-blit ring, same indices.
                checked('dump binary memory .run/effect-census.bin &gDownwellEffectCensus[0] &gDownwellEffectCensus[gDownwellCensusCapacity]')
            if os.environ.get('CENSUS_PARTS')=='1':
                # W3.7: the work of each update by gameplay loop stage.
                checked('dump binary memory .run/census-parts.bin &gDownwellCensusParts[0] &gDownwellCensusParts[gDownwellCensusCapacity]')
            if os.environ.get('BLIT_CENSUS')=='1':
                # W3.15: every blit started, same indices.
                checked('dump binary memory .run/blit-census.bin &gDownwellBlitCensus[0] &gDownwellBlitCensus[gDownwellCensusCapacity]')
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
        # DOWNWELL_TERRAIN_SAMPLE=1 (WR.5): the native terrain lifetimes'
        # run descriptors and source records ever in use, and live cells.
        # Builds without source records (the shipping build) report records=0.
        terrainPeak=None
        terrainFormat='printf "TERRAIN %u 0 %u\\n",$nt->runs_.used_,$nt->runs_.population_'
        if os.environ.get('DOWNWELL_TERRAIN_SAMPLE')=='1':
            checked(f"set $nt=(GameplayWorld::NativeTerrain*)&'{storageSymbol(r'_ZZL11runDownwellvE20nativeTerrainStorage')}'")
            terrainPeak=[0,0,0]
            if 'sources_' in g.cmd('ptype $nt->runs_'):
                terrainFormat=('printf "TERRAIN %u %u %u\\n",$nt->runs_.used_,$nt->runs_.sources_.used_,'
                               '$nt->runs_.population_+$nt->runs_.sources_.population_')
        def sampleTerrain():
            if terrainPeak is None: return
            values=re.search(r'TERRAIN (\d+) (\d+) (\d+)',checked(terrainFormat))
            for i in range(3): terrainPeak[i]=max(terrainPeak[i],int(values[i+1]))
        def sampleMemory(tick,scene):
            if tick%32==0: sampleTerrain()
            if not memoryRegions: return
            free=list(map(int,re.search(r'FREE ([\d ]+)',checked(memoryFormat))[1].split()))
            for (node,attributes),value in zip(memoryRegions,free):
                low=memoryLow.get(node)
                if low is None: memoryLow[node]=[value,value,tick,scene,attributes]
                elif value<low[1]: low[1:4]=value,tick,scene
        def reportMemory():
            if terrainPeak is not None:
                print(f'TERRAIN peak runs={terrainPeak[0]} records={terrainPeak[1]} cells={terrainPeak[2]}',flush=True)
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
        # DOWNWELL_MEMORY_LEDGER=1: every exec AllocMem/FreeMem, attributed
        # to the game function that asked (operator new's caller for new),
        # with the live set at the route's Chip and public peaks. Calls from
        # ROM (dos buffers, LoadSeg) are not attributed; the low-water marks
        # of DOWNWELL_MEMORY_SAMPLE=1 include them.
        ledger=os.environ.get('DOWNWELL_MEMORY_LEDGER')=='1'
        ledgerLive={};ledgerPeak={'chip':(0,{}, ''),'public':(0,{}, '')}
        ledgerScene=['boot'];ledgerBreaks={}
        def breakNumber(out):
            return int(re.search(r'Breakpoint (\d+) at',out)[1])
        if ledger:
            execBase=int(checked('printf "%lu\\n", *(unsigned long*)4').split()[-1])
            allocEntry=int(checked(f'printf "%lu\\n", *(unsigned long*)({execBase}-198+2)').split()[-1])
            freeEntry=int(checked(f'printf "%lu\\n", *(unsigned long*)({execBase}-210+2)').split()[-1])
            ledgerBreaks[breakNumber(checked(f'break *{allocEntry}'))]='alloc'
            ledgerBreaks[breakNumber(checked(f'break *{freeEntry}'))]='free'
            ledgerBreaks[breakNumber(checked("break *'_Znwm'"))]='new'
        pendingNew=[None]
        def symbolAt(address):
            out=g.cmd(f'info symbol {address}')
            m=re.search(r'^(.*?) in section',out,re.M)
            return m[1].strip() if m else None
        def ledgerTotals():
            totals={'chip':0,'public':0}
            for size,flags,caller in ledgerLive.values():
                totals['chip' if flags&2 else 'public']+=size
            return totals
        def ledgerStop(stop):
            m=re.search(r'Breakpoint (\d+),',stop)
            kind=ledgerBreaks.get(int(m[1])) if m else None
            if kind is None: return False
            ret=int(checked('printf "%lu\\n", *(unsigned long*)$sp').split()[-1])
            if kind=='new':
                pendingNew[0]=symbolAt(ret) or f'{ret:#x}'
                return True
            caller=symbolAt(ret)
            if caller is None: return True
            if kind=='free':
                address=int(checked('printf "%lu\\n", $a1').split()[-1])
                ledgerLive.pop(address,None)
                return True
            size,flags=(int(x) for x in checked('printf "%lu %lu\\n", $d0, $d1').split()[-2:])
            if caller.startswith('operator new') or caller=='_Znwm':
                caller='new: '+(pendingNew[0] or '?')
                pendingNew[0]=None
            out=checked(f'tbreak *{ret}')
            done=checked('continue')
            if 'Temporary breakpoint' not in done: raise RuntimeError('ledger lost AllocMem return: '+done)
            address=int(checked('printf "%lu\\n", $d0').split()[-1])
            if address:
                ledgerLive[address]=(size,flags,caller)
                totals=ledgerTotals()
                for key in ('chip','public'):
                    if totals[key]>ledgerPeak[key][0]:
                        ledgerPeak[key]=(totals[key],dict(ledgerLive),ledgerScene[0])
            return True
        def resume():
            while True:
                stop=checked('continue')
                if not ledger or not ledgerStop(stop): return stop
        def reportLedger():
            if not ledger: return
            for key in ('chip','public'):
                total,live,where=ledgerPeak[key]
                groups={}
                for size,flags,caller in live.values():
                    k=('chip' if flags&2 else 'public',caller)
                    n,b=groups.get(k,(0,0));groups[k]=(n+1,b+size)
                print(f'LEDGER peak {key}={total} at {where}',flush=True)
                for (kind,caller),(n,b) in sorted(groups.items(),key=lambda kv:-kv[1][1]):
                    print(f'LEDGER {key}-peak {kind} {b} x{n} {caller}',flush=True)
            for size,flags,caller in sorted(ledgerLive.values(),key=lambda v:-v[0]):
                print(f'LEDGER end {"chip" if flags&2 else "public"} {size} {caller}',flush=True)
        def sampleVoices(tick):
            out=checked(voiceFormat)
            m=re.search(r'VOICES ([\d -]+)',out)
            if not m: raise RuntimeError('voice table read: '+repr(out))
            v=list(map(int,m[1].split()))
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
            # soundPlayOL's cutPrv stops the channel's previous sound at once,
            # so a play cut by a later play on its channel in the same tick is
            # never audible (six gunwall explosions in one tick leave one).
            plays=[line.split() for line in Path(base+'.sounds').read_text().splitlines()]
            plays=[f for f in plays if len(f)>7 and f[1]=='play' and f[7]!='5']
            for i,f in enumerate(plays):
                if any(g[0]==f[0] and g[7]==f[7] and g[6]=='1' for g in plays[i+1:i+16]):continue
                host[int(f[2])]=host.get(int(f[2]),0)+1
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
                stop=resume()
                if 'downwellWorldOneComplete' in stop:
                    dumpCensus()
                    state=checked('printf "END %d %d %u %d %u\\n", $w->run_.stage.area,$w->run_.stage.level,$w->tick_,$w->health_,(unsigned)$w->enemyRandom_.drawCount_')
                    print(state,flush=True)
                    reportMemory()
                    reportLedger()
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
                state=checked('printf "STATE %d %u %d %d %d %d %d %d\\n", $s->scene_,$w->tick_,$w->run_.stage.area,$w->run_.stage.level,$w->health_,$w->generationSucceeded_,$w->run_.levelResultRequested&&!$w->run_.levelClearActive,(int)(($s->menuX_+128)>>8)')
                m=re.search(r'STATE (\d+) (\d+) (\d+) (\d+) (-?\d+) (\d+) (\d+) (-?\d+)',state)
                if not m: raise RuntimeError(state)
                scene,tick,area,level,hp,good,resultPending,gardenX=map(int,m.groups())
                # Runs that end early (a trace or capture tick) never check
                # sounds; skip the voice table's round trip at every sample.
                if not traceFrame and shotTick<0: sampleVoices(tick)
                sampleMemory(tick,scene)
                ledgerScene[0]=f'scene={scene} tick={tick} level={area}-{level}'
                if scene!=oldScene:
                    # W1.14: the garden's own sounds (jump, hop, landings and
                    # footsteps by surface) start before the run's first tick.
                    if oldScene==1: print('GARDEN sounds '+' '.join(map(str,sorted(nativeStarts))),flush=True)
                    oldScene=scene;sceneFrame=0
                sceneFrame+=1
                if scene==0: packed=4 if sceneFrame%60==0 else 0
                elif scene==1:
                    # src/core/GardenMotion.h gardenRouteInput: right until over
                    # the well, fire from x 240 (the rim stops a walk).
                    packed=(2 if gardenX<300 else 0)|(4 if 240<=gardenX<300 else 0) if sceneFrame>40 else 0
                    if gardenShot and gardenX>=272:
                        gardenShot=False;shot('garden-jump')
                elif scene==2: packed=4 if sceneFrame==120 else 0
                elif retrySamples>=0:
                    # DOWNWELL_ROUTE_RETRY=1: after the results bar, fire takes
                    # the default RETRY (cursor 0); the new run must reach 1-1
                    # alive and keep running hands-off.
                    retrySamples+=1
                    if hp>0 and area==1 and level==1 and good:
                        retryLive+=1
                        packed=0
                        if retryLive==200:
                            dumpCensus()
                            print(state,flush=True)
                            checked('delete breakpoints')
                            shot('world-one-retry')
                            print('DOWNWELL_WORLD_ONE_RETRY_READY',flush=True)
                            Path('.run/gdb-ready.marker').touch()
                            return
                    else:
                        if retryLive: raise RuntimeError('retried run stopped: '+state)
                        packed=4 if retrySamples%40<4 else 0
                    if retrySamples>3000: raise RuntimeError('Retry did not reach 1-1: '+state)
                elif deathRoute and hp<=0:
                    # Hands off: the results sequence fills the progress bar.
                    packed=0
                    deathSamples+=1
                    notice=checked('printf "NOTICE %d %u\\n", $w->run_.results.menu.goalNotice,$w->run_.results.progress.goal')
                    if deathSamples%50==0: print(notice,flush=True)
                    stage=int(re.search(r'NOTICE (-?\d+)',notice)[1])
                    if 330 in nativeStarts:
                        if not retryRoute: dumpCensus()
                        reportMemory()
                        # Without a gem seed no goal is reached: the bar's
                        # completion sound (sfxUiRetry 330) ends the results.
                        checkSounds()
                        if retryRoute:
                            print('RESULTS DONE '+state,flush=True)
                            retrySamples=0
                            continue
                        print('DOWNWELL_WORLD_ONE_RESULTS_READY',flush=True)
                        Path('.run/gdb-ready.marker').touch()
                        return
                    if stage>=3:
                        dumpCensus()
                        reportMemory()
                        checkSounds()
                        for _ in range(20): resume()
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
                        dumpCensus()
                        if os.environ.get('DOWNWELL_ROUTE_CELL_ORACLE')=='1':
                            # CELL_REPAIR_VERIFY builds: merged destroyed-cell
                            # repair against the per-cell reference.
                            oracle=checked('printf "CELL ORACLE checks=%lu merges=%lu failures=%lu\\n", gDownwellCellRepairChecks,gDownwellCellRepairMerges,gDownwellCellRepairFailures').strip()
                            print(oracle,flush=True)
                            counts=[int(n) for n in re.findall(r'\d+',oracle.split('ORACLE',1)[1])]
                            if counts[0]==0 or counts[1]==0 or counts[2]:
                                raise RuntimeError('cell repair oracle failed: '+oracle)
                        if os.environ.get('DOWNWELL_ROUTE_EFFECT_PIXEL_ORACLE')=='1':
                            # EFFECT_PIXEL_VERIFY builds: every composition's
                            # blitter surface against the CPU reference (a
                            # mismatch already stopped the route).
                            pixels=checked('printf "EFFECT PIXEL ORACLE checks=%u failures=%u stamps=%u\\n", gDownwellEffectDmaChecks,gDownwellEffectDmaFailures,gDownwellEffectDmaStamps').strip()
                            print(pixels,flush=True)
                            counts=[int(n) for n in re.findall(r'\d+',pixels.split('ORACLE',1)[1])]
                            if counts[0]==0 or counts[1]:
                                raise RuntimeError('effect pixel oracle failed: '+pixels)
                        if os.environ.get('DOWNWELL_ROUTE_EFFECT_CLEAR_ORACLE')=='1':
                            # EFFECT_CLEAR_VERIFY builds: the rectangle clear
                            # left the whole effect surface zero.
                            oracle=checked('printf "EFFECT CLEAR ORACLE checks=%lu failures=%lu first=%lu at=%lu bytes=%lu last=%lu\\n", gDownwellEffectClearChecks,gDownwellEffectClearFailures,gDownwellEffectClearOffset,gDownwellEffectClearFirstCheck,gDownwellEffectClearBytes,gDownwellEffectClearLast').strip()
                            print(oracle,flush=True)
                            coverage=checked('printf "EFFECT COVERAGE checks=%lu failures=%lu row=%lu word=%lu plane=%lu stamp=%ld,%ld %ldx%ld stride=%ld kind=%ld\\n", gDownwellEffectCoverageChecks,gDownwellEffectCoverageFailures,gDownwellEffectCoverageRow,gDownwellEffectCoverageWord,gDownwellEffectCoveragePlane,gDownwellEffectCoverageStamp[0],gDownwellEffectCoverageStamp[1],gDownwellEffectCoverageStamp[2],gDownwellEffectCoverageStamp[3],gDownwellEffectCoverageStamp[4],gDownwellEffectCoverageStamp[5]').strip()
                            print(coverage,flush=True)
                            counts=[int(n) for n in re.findall(r'\d+',oracle.split('ORACLE',1)[1])]
                            covered=[int(n) for n in re.findall(r'\d+',coverage.split('COVERAGE',1)[1])[:2]]
                            if counts[0]==0 or counts[1] or covered[0]==0 or covered[1]:
                                raise RuntimeError('effect clear oracle failed: '+oracle+'; '+coverage)
                        reportMemory()
                        reportLedger()
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
                if traceFrame and scene==3:
                    update=int(re.search(r'UPDATE (\d+)',checked('printf "UPDATE %lu\\n", gDownwellProfileFrames[3]+1'))[1])
                    if update>traceFrame-1: raise RuntimeError(f'route passed update {traceFrame-1} before the trace')
                    if update==traceFrame-1:
                        nextInput=tape[tick+1] if tick+1<len(tape) else None
                        print(f'TRACE input update={traceFrame} tick={tick+1} tape={nextInput} held={packed}'
                              f'{"" if nextInput==packed else " (differs)"}',flush=True)
                        checked('delete breakpoints')
                        out=Path(traceOutput)/f'trace-{update}.bin'
                        print(f'TRACE start frame={update}',flush=True)
                        out.parent.mkdir(parents=True,exist_ok=True)
                        profile(g,traceFields,out.resolve())
                        print(checked('printf "TRACE end start=%lu frames_completed=%lu\\n", '+str(update)+', gDownwellProfileFrames[3]').strip(),flush=True)
                        print('DOWNWELL_WORLD_ONE_TRACE_READY',flush=True)
                        Path('.run/gdb-ready.marker').touch()
                        return
        raise RuntimeError('route exhausted frame bound')
    finally:
        log.close()
        g.p.terminate()
        try: g.p.wait(timeout=5)
        except subprocess.TimeoutExpired: g.p.kill();g.p.wait()


if __name__=='__main__':
    main()
