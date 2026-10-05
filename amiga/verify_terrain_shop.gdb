set pagination off
set confirm off
target remote 127.0.0.1:2345
break downwellTerrainBatchMismatch
commands
    silent
    printf "Native terrain pixel mismatch\n"
    quit 1
end
break downwellProfileReady
continue
if gDownwellShopProfileDone != 1 || gDownwellShopProfileFailures != 0 || gDownwellShopProfileFrames[0] != 2198 || gDownwellShopProfileFrames[1] != 670 || gDownwellShopProfileSideFrames != 61 || gDownwellShopProfileMenuFrames != 5
    printf "Natural shop tape incomplete or source state diverged\n"
    quit 1
end
if gDownwellProfileFrozen != 1 || gDownwellProfileFrames[3] != gDownwellProfileTargetFrames || gDownwellProfileDeadSamples != 0
    printf "Invalid shop verification window\n"
    quit 1
end
if !gDownwellInitialTerrainEnabled || gDownwellInitialTerrainChecks != 18 || gDownwellTerrainBatchFailures != 0 || gDownwellTerrainDirectChecks == 0
    printf "Initial/reentry terrain pixel coverage failed: prepared=%lu dirty=%lu failures=%lu\n", gDownwellInitialTerrainChecks, gDownwellTerrainDirectChecks, gDownwellTerrainBatchFailures
    quit 1
end
printf "SHOP_TERRAIN exact source-state/RNG; prepared spans=%lu dirty spans=%lu failures=%lu\n", gDownwellInitialTerrainChecks, gDownwellTerrainDirectChecks, gDownwellTerrainBatchFailures
printf "DOWNWELL_PROFILE_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
