set pagination off
set confirm off
target remote 127.0.0.1:2345
break downwellGameplayLongRunReady
continue
if gDownwellTerrainRasterFailures != 0 || gDownwellTerrainRasterChecks < 50 || gDownwellGameBufferFailures != 0
    printf "Gameplay cache verification failed: raster=%lu checks=%lu buffers=%lu\n", gDownwellTerrainRasterFailures, gDownwellTerrainRasterChecks, gDownwellGameBufferFailures
    quit 1
end
printf "Gameplay cache verified through 600 simulation ticks: %lu fresh-composition comparisons\n", gDownwellTerrainRasterChecks
printf "DOWNWELL_GAMEPLAY_CACHE_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
