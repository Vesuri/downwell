set pagination off
set confirm off
target remote 127.0.0.1:2345
break downwellEffectSurfaceFailed
commands
    silent
    printf "DMA effect pixel mismatch: stamps=%u checks=%u publication=%u/%u\n", gDownwellEffectDmaStamps, gDownwellEffectDmaChecks, gDownwellEffectPublishChecks, gDownwellEffectPublishFailures
    quit 1
end
break downwellStartupFailed
commands
    silent
    printf "Actor verification startup failed: reason=%lu remaining Chip=%lu\n", gDownwellStartupFailure, gDownwellStartupChipRemaining
    quit 1
end
break downwellSceneStackFailed
commands
    silent
    printf "Actor verifier stack guard failed at stage %lu\n", gDownwellSceneStackFailureStage
    quit 1
end
# BOB_VERIFY=1 exercises overflow for every actor, including the player.
break downwellGameplayBobsReady
continue
if gDownwellGameplayBobChecks < 100 || gDownwellDamageTextChecks != 32 || gDownwellActorOrderChecks < 32 || gDownwellActorOrderFailures != 0 || gDownwellGameplayBobFailures != 0 || gDownwellGameBufferFailures != 0 || gDownwellTerrainRasterFailures != 0
    printf "Overflow actor verification failed: checks=%lu pixels=%lu buffers=%lu terrain=%lu\n", gDownwellGameplayBobChecks, gDownwellGameplayBobFailures, gDownwellGameBufferFailures, gDownwellTerrainRasterFailures
    quit 1
end
printf "Overflow actors verified: %lu scalar-oracle comparisons\n", gDownwellGameplayBobChecks
printf "Prepared damage text verified: %u clipped/wrapped integer labels\n", gDownwellDamageTextChecks
printf "Actor order verified: %u live/cached comparisons\n", gDownwellActorOrderChecks
if gDownwellEffectDmaStamps < 1 || gDownwellEffectDmaChecks < 129 || gDownwellEffectDmaFailures != 0
    printf "DMA effect verification failed: stamps=%u checks=%u failures=%u\n", gDownwellEffectDmaStamps, gDownwellEffectDmaChecks, gDownwellEffectDmaFailures
    quit 1
end
printf "DMA effects verified: %u pixel comparisons, %u stamps\n", gDownwellEffectDmaChecks, gDownwellEffectDmaStamps
# The actor-ready breakpoint precedes effect publication on frame 100.
if gDownwellEffectPublishChecks < 99 || gDownwellEffectPublishFailures != 0
    printf "DMA effect publication failed: checks=%u failures=%u\n", gDownwellEffectPublishChecks, gDownwellEffectPublishFailures
    quit 1
end
printf "DMA effect publication verified: %u full-ring comparisons\n", gDownwellEffectPublishChecks
printf "DOWNWELL_GAMEPLAY_BOBS_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
