set pagination off
set confirm off
target remote 127.0.0.1:2345
break downwellClearProbe
commands
silent
printf "Natural tick=%lu clear=%lu reentries=%lu failures=%lu\n", gDownwellClearTick, gDownwellClearFrames, gDownwellClearReentries, gDownwellClearFailures
if gDownwellClearFailures != 0 || gDownwellGameBufferFailures != 0
    quit 1
end
continue
end
break downwellNaturalReady
continue
if gDownwellNaturalExitTick != 2156 || gDownwellClearReentries != 1 || gDownwellClearGameplayUpdates != 914
    printf "Natural route timing/reentry mismatch: exit=%lu entries=%lu updates=%lu\n", gDownwellNaturalExitTick, gDownwellClearReentries, gDownwellClearGameplayUpdates
    quit 1
end
if gDownwellNaturalFailures != 0 || gDownwellNaturalReturnFrames != 45 || gDownwellShopLiveFailures != 0 || gDownwellShopLivePixelFailures != 0 || gDownwellShopLiveFrames < 7
    printf "Natural shop/return failed: logic=%lu return=%lu frames=%lu raster=%lu\n", gDownwellNaturalFailures, gDownwellNaturalReturnFrames, gDownwellShopLiveFrames, gDownwellShopLivePixelFailures
    quit 1
end
if gDownwellClearFrames < 200 || gDownwellClearFailures != 0 || gDownwellGameBufferFailures != 0 || gDownwellClearFieldFrames == 0
    printf "Natural presentation comparison failed\n"
    quit 1
end
if gDownwellStackGuardReady != 1
    quit 1
end
set $guard = (unsigned long *)SysBase->ThisTask->tc_SPLower
set $word = 0
while $word < 64
    if $guard[$word] != 0x57ac6a4d
        quit 1
    end
    set $word = $word + 1
end
printf "Natural exit and reentry passed: %lu clear frames, %lu exit field frames\n", gDownwellClearFrames, gDownwellClearFieldFrames
printf "Natural shop and return: %lu HUD comparisons, %lu return updates\n", gDownwellShopLiveFrames, gDownwellNaturalReturnFrames
printf "Natural route totals: exit=%lu second-level updates=%lu\n", gDownwellNaturalExitTick, gDownwellClearGameplayUpdates
printf "DOWNWELL_NATURAL_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
