set pagination off
set confirm off
target remote 127.0.0.1:2345
break downwellTerrainBatchMismatch
commands
    silent
    printf "Native terrain batch pixel mismatch\n"
    quit 1
end
break downwellProfileReady
continue
if gDownwellProfileFrozen != 1 || gDownwellProfileFrames[3] != gDownwellProfileTargetFrames || gDownwellProfileGameSamples != gDownwellProfileTargetFrames
    printf "Terrain verification did not finish the gameplay tape\n"
    quit 1
end
if gDownwellTerrainBatchChecks + gDownwellTerrainDirectChecks == 0 || gDownwellTerrainBatchFailures != 0
    printf "Native batched terrain differs from retained CPU composition\n"
    quit 1
end
printf "PROFILE terrain pixels: %lu exact full-ring comparisons in %lu gameplay updates; failures=%lu\n", gDownwellTerrainBatchChecks, gDownwellProfileGameSamples, gDownwellTerrainBatchFailures
printf "PROFILE terrain direct-cache pixels: %lu exact dirty-span comparisons\n", gDownwellTerrainDirectChecks
printf "DOWNWELL_PROFILE_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
