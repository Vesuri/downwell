set pagination off
set confirm off
target remote 127.0.0.1:2345
source chip_memory_watch.gdb
break downwellStartupFailed
commands
    silent
    printf "Startup failed before results verification: reason=%lu Chip free=%lu largest=%lu remaining=%lu\n", gDownwellStartupFailure, gDownwellStartupChipFree, gDownwellStartupChipLargest, gDownwellStartupChipRemaining
    quit 1
end
break downwellSceneStackFailed
commands
    silent
    printf "Scene stack guard failed before results verification\n"
    quit 1
end
break downwellResultsRestartReady
continue
if gDownwellPreciseMaskChecks != 2
    printf "Native terrain mask probe did not complete\n"
    quit 1
end
printf "Initial Chip allocation: free=%lu largest=%lu remaining=%lu; native terrain mask checks=%lu\n", gDownwellStartupChipFree, gDownwellStartupChipLargest, gDownwellStartupChipRemaining, gDownwellPreciseMaskChecks
if gDownwellResultsRestartCount != 3 || gDownwellResultsRestartMask != 7
    printf "Not all results destinations executed\n"
    quit 1
end
if gDownwellResultsHudChecks != 4 || gDownwellResultsHudFailures != 0
    printf "HUD residency failed across results destinations: checks=%lu failures=%lu\n", gDownwellResultsHudChecks, gDownwellResultsHudFailures
    quit 1
end
printf "HUD residency: four presentation entries match the offline bank byte-for-byte\n"
if gDownwellResultsBankChecks != 4 || gDownwellResultsBankFailures != 0
    printf "Unrelated overlay bank resident on gameplay entry: checks=%lu failures=%lu\n", gDownwellResultsBankChecks, gDownwellResultsBankFailures
    quit 1
end
printf "Results artwork: four gameplay entries have no unrelated overlay bank\n"
if gDownwellStackGuardReady != 1
    quit 1
end
if gDownwellOverlayCopperChecks != 640 || gDownwellOverlayCopperFailures != 0
    printf "Retained overlay copper patch verification failed\n"
    quit 1
end
set $stack_guard = (unsigned long *)SysBase->ThisTask->tc_SPLower
set $word = 0
while $word < 64
    if $stack_guard[$word] != 0x57ac6a4d
        printf "Restart exhausted the stack safety margin\n"
        quit 1
    end
    set $word = $word + 1
end
if gDownwellLiveGenerationFailed != 0 || gDownwellTerrainRasterFailures != 0 || gDownwellGameBufferFailures != 0 || gDownwellGameplayBobFailures != 0 || gDownwellStyleReferenceFailures != 0 || gDownwellMenuCopperPatchFailures != 0
    printf "Scene reentry renderer or generation failure\n"
    quit 1
end
# Boot and front banks at startup; style select and surface each reload front.
if gDownwellSceneBankLoads != 4
    printf "Unexpected scene bank loads: %lu\n", gDownwellSceneBankLoads
    quit 1
end
printf "Scene bank loads: %lu\n", gDownwellSceneBankLoads
printf "Three results destinations executed; final gameplay reached 200 updates\n"
printf "DOWNWELL_RESULTS_RESTART_READY\n"
downwell-chip-snapshot
shell touch .run/gdb-ready.marker
detach
quit
