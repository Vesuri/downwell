set pagination off
set confirm off
target remote 127.0.0.1:2345
# BOB_VERIFY=1 exercises overflow even in sparse scenes; player stays in sprite 0.
break downwellGameplayBobsReady
continue
if gDownwellGameplayBobChecks < 100 || gDownwellGameplayBobFailures != 0 || gDownwellGameBufferFailures != 0 || gDownwellTerrainRasterFailures != 0
    printf "Overflow actor verification failed: checks=%lu pixels=%lu buffers=%lu terrain=%lu\n", gDownwellGameplayBobChecks, gDownwellGameplayBobFailures, gDownwellGameBufferFailures, gDownwellTerrainRasterFailures
    quit 1
end
printf "Overflow actors verified: %lu scalar-oracle comparisons\n", gDownwellGameplayBobChecks
printf "DOWNWELL_GAMEPLAY_BOBS_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
