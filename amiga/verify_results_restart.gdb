set pagination off
set confirm off
target remote 127.0.0.1:2345
break downwellResultsRestartReady
continue
if gDownwellResultsRestartCount != 3 || gDownwellResultsRestartMask != 7
    printf "Not all results destinations executed\n"
    quit 1
end
if gDownwellStackGuardReady != 1
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
printf "Three results destinations executed; final gameplay reached 200 updates\n"
printf "DOWNWELL_RESULTS_RESTART_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
