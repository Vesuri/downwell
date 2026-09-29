set pagination off
set confirm off
target remote 127.0.0.1:2345
break downwellSceneStackFailed
commands
    silent
    printf "Actor verifier stack guard failed at stage %lu\n", gDownwellSceneStackFailureStage
    quit 1
end
# BOB_VERIFY=1 exercises overflow for every actor, including the player.
break downwellGameplayBobsReady
continue
if gDownwellGameplayBobChecks < 100 || gDownwellDamageTextChecks != 32 || gDownwellGameplayBobFailures != 0 || gDownwellGameBufferFailures != 0 || gDownwellTerrainRasterFailures != 0
    printf "Overflow actor verification failed: checks=%lu pixels=%lu buffers=%lu terrain=%lu\n", gDownwellGameplayBobChecks, gDownwellGameplayBobFailures, gDownwellGameBufferFailures, gDownwellTerrainRasterFailures
    quit 1
end
printf "Overflow actors verified: %lu scalar-oracle comparisons\n", gDownwellGameplayBobChecks
printf "Prepared damage text verified: %u clipped/wrapped integer labels\n", gDownwellDamageTextChecks
printf "DOWNWELL_GAMEPLAY_BOBS_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
