set pagination off
set confirm off
target remote 127.0.0.1:2345
break downwellGameplayLongRunReady
continue
if gDownwellStackGuardReady != 1
    printf "Target could not install the task-stack guard\n"
    quit 1
end
# The CPU initialized these words before platform/scene initialization.
set $stack_guard = (unsigned long *)SysBase->ThisTask->tc_SPLower
set $guard_word = 0
while $guard_word < 64
    if $stack_guard[$guard_word] != 0x57ac6a4d
        printf "Gameplay exhausted the 256-byte task-stack safety margin: word=%u value=%lx base=%p\n", $guard_word, $stack_guard[$guard_word], $stack_guard
        quit 1
    end
    set $guard_word = $guard_word + 1
end
if gDownwellTerrainRasterFailures != 0 || gDownwellTerrainRasterChecks < 50 || gDownwellGameBufferFailures != 0 || gDownwellGameplayBobFailures != 0
    printf "Gameplay cache verification failed: raster=%lu checks=%lu buffers=%lu\n", gDownwellTerrainRasterFailures, gDownwellTerrainRasterChecks, gDownwellGameBufferFailures
    quit 1
end
if gDownwellLiveGenerationFailed != 0 || gDownwellLiveFragments == 0 || gDownwellLiveFragments >= 19 || gDownwellLiveEnemies == 0
    printf "Live generation verification failed: failure=%lu fragments=%lu enemies=%lu\n", gDownwellLiveGenerationFailed, gDownwellLiveFragments, gDownwellLiveEnemies
    quit 1
end
printf "Live generated prefix: %lu fragments, %lu enemies, %lu RNG draws at last builder step\n", gDownwellLiveFragments, gDownwellLiveEnemies, gDownwellLiveRandomDraws
if gDownwellResultsAssetFailures != 0 || gDownwellResultsAssetBytes == 0
    printf "Results/HUD reserved asset corruption: failures=%lu bytes=%lu\n", gDownwellResultsAssetFailures, gDownwellResultsAssetBytes
    quit 1
end
printf "Results and HUD packs intact after gameplay staging: %lu results bytes\n", gDownwellResultsAssetBytes
printf "Overflow actors: %lu scalar-oracle comparisons, %lu mismatched bytes\n", gDownwellGameplayBobChecks, gDownwellGameplayBobFailures
printf "Gameplay cache verified through 600 simulation ticks: %lu fresh-composition comparisons\n", gDownwellTerrainRasterChecks
printf "Task stack retained its 256-byte safety margin\n"
printf "DOWNWELL_GAMEPLAY_CACHE_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
