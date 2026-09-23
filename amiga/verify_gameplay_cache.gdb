set pagination off
set confirm off
target remote 127.0.0.1:2345
break downwellGameplayLongRunReady
continue
if gDownwellTerrainRasterFailures != 0 || gDownwellTerrainRasterChecks < 50 || gDownwellGameBufferFailures != 0 || gDownwellGameplayBobFailures != 0
    printf "Gameplay cache verification failed: raster=%lu checks=%lu buffers=%lu\n", gDownwellTerrainRasterFailures, gDownwellTerrainRasterChecks, gDownwellGameBufferFailures
    quit 1
end
if gDownwellLiveGenerationFailed != 0 || gDownwellLiveFragments == 0 || gDownwellLiveFragments >= 19 || gDownwellLiveEnemies == 0
    printf "Live generation verification failed: failure=%lu fragments=%lu enemies=%lu\n", gDownwellLiveGenerationFailed, gDownwellLiveFragments, gDownwellLiveEnemies
    quit 1
end
printf "Live generated prefix: %lu fragments, %lu enemies, %lu RNG draws at last builder step\n", gDownwellLiveFragments, gDownwellLiveEnemies, gDownwellLiveRandomDraws
printf "Overflow actors: %lu scalar-oracle comparisons, %lu mismatched bytes\n", gDownwellGameplayBobChecks, gDownwellGameplayBobFailures
printf "Gameplay cache verified through 600 simulation ticks: %lu fresh-composition comparisons\n", gDownwellTerrainRasterChecks
printf "DOWNWELL_GAMEPLAY_CACHE_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
