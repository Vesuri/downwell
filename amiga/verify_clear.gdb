set pagination off
set confirm off
target remote 127.0.0.1:2345
break downwellClearProbe
commands
silent
printf "Clear phase=%lu frames=%lu choices=%lu failures=%lu reentries=%lu\n", gDownwellClearPhase, gDownwellClearFrames, gDownwellClearChoices, gDownwellClearFailures, gDownwellClearReentries
printf "Gameplay tick=%lu field frames=%lu pixel failures=%lu\n", gDownwellClearTick, gDownwellClearFieldFrames, gDownwellGameBufferFailures
if gDownwellClearFailures != 0
    dump binary memory .run/clear-actual-hud.bin gDownwellClearActualHud gDownwellClearActualHud+27264
    dump binary memory .run/clear-expected-hud.bin gDownwellClearExpectedHud gDownwellClearExpectedHud+27264
    dump binary memory .run/clear-actual-game.bin gDownwellClearActualGame gDownwellClearActualGame+30720
    dump binary memory .run/clear-expected-game.bin gDownwellClearExpectedGame gDownwellClearExpectedGame+30720
    quit 1
end
if gDownwellClearPhase >= 100
    quit 1
end
continue
end
break downwellClearReady
continue
if gDownwellClearFrames < 400 || gDownwellClearChoices < 60 || gDownwellClearFailures != 0
    printf "Clear presentation failed: frames=%lu choices=%lu pixel failures=%lu\n", gDownwellClearFrames, gDownwellClearChoices, gDownwellClearFailures
    quit 1
end
if gDownwellClearReentries != 2 || gDownwellClearGameplayUpdates != 60
    printf "Clear departure did not resume gameplay\n"
    quit 1
end
if gDownwellClearCursorMasks[0] != 7 || gDownwellClearCursorMasks[1] != 7
    printf "Upgrade navigation missed choices: first=%lu second=%lu\n", gDownwellClearCursorMasks[0], gDownwellClearCursorMasks[1]
    quit 1
end
printf "All three upgrade cursors covered on both room entries\n"
if gDownwellClearFieldFrames < 180 || gDownwellGameBufferFailures != 0
    printf "Exit field coverage failed: frames=%lu pixel failures=%lu\n", gDownwellClearFieldFrames, gDownwellGameBufferFailures
    quit 1
end
printf "Exit field: %lu retained planar frames compared\n", gDownwellClearFieldFrames
if gDownwellStackGuardReady != 1
    quit 1
end
set $guard = (unsigned long *)SysBase->ThisTask->tc_SPLower
set $word = 0
while $word < 64
    if $guard[$word] != 0x57ac6a4d
        printf "Clear presenter exhausted stack margin\n"
        quit 1
    end
    set $word = $word + 1
end
printf "Two clears and 60 third-level updates passed; %lu planar frames compared\n", gDownwellClearFrames
printf "DOWNWELL_CLEAR_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
