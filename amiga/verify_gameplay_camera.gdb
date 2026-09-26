set pagination off
set confirm off
target remote 127.0.0.1:2345
break downwellGameplayLongRunReady
continue
if gDownwellCameraLocks < 3 || gDownwellCameraReleases < 3
    printf "Side-lock collision/release coverage missing: locks=%lu releases=%lu\n", gDownwellCameraLocks, gDownwellCameraReleases
    quit 1
end
if gDownwellCameraColumns != 7 || gDownwellCameraTransitions < 6 || gDownwellCameraStaged == 0 || gDownwellCameraFallback == 0
    printf "Camera coverage missing: columns=%lu transitions=%lu staged=%lu fallback=%lu\n", gDownwellCameraColumns, gDownwellCameraTransitions, gDownwellCameraStaged, gDownwellCameraFallback
    quit 1
end
if gDownwellCameraFrameChecks != 2 * gDownwellCameraTransitions
    printf "Both display slots were not checked after each camera transition\n"
    quit 1
end
if gDownwellColdStageRows == 0
    printf "Cold side-room staging was not exercised\n"
    quit 1
end
printf "Cold side-room staging: %lu freshly composed rows\n", gDownwellColdStageRows
if gDownwellTerrainRasterFailures != 0 || gDownwellTerrainRasterChecks < 50 || gDownwellGameBufferFailures != 0 || gDownwellGameplayBobFailures != 0
    printf "Camera raster mismatch: raster=%lu checks=%lu display=%lu bobs=%lu\n", gDownwellTerrainRasterFailures, gDownwellTerrainRasterChecks, gDownwellGameBufferFailures, gDownwellGameplayBobFailures
    printf "First cache/fresh mismatch: viewX=%ld worldY=%ld height=%ld row=%ld plane=%ld word=%ld cached=%04lx fresh=%04lx\n", gDownwellTerrainFirstMismatch[0], gDownwellTerrainFirstMismatch[1], gDownwellTerrainFirstMismatch[2], gDownwellTerrainFirstMismatch[3], gDownwellTerrainFirstMismatch[4], gDownwellTerrainFirstMismatch[5], gDownwellTerrainFirstMismatch[6], gDownwellTerrainFirstMismatch[7]
    quit 1
end
if gDownwellStackGuardReady != 1 || gDownwellLiveGenerationFailed != 0 || gDownwellResultsAssetFailures != 0
    printf "Camera fixture initialization/generation/reserved asset failure\n"
    quit 1
end
set $guard = (unsigned long *)SysBase->ThisTask->tc_SPLower
set $i = 0
while $i < 64
    if $guard[$i] != 0x57ac6a4d
        printf "Camera fixture exhausted stack margin\n"
        quit 1
    end
    set $i = $i + 1
end
printf "Camera fixture: 600 updates, columns=%lu transitions=%lu staged=%lu fallback=%lu fresh_comparisons=%lu display_checks=%lu; zero raster/display mismatches and stack intact\n", gDownwellCameraColumns, gDownwellCameraTransitions, gDownwellCameraStaged, gDownwellCameraFallback, gDownwellTerrainRasterChecks, gDownwellCameraFrameChecks
printf "DOWNWELL_GAMEPLAY_CAMERA_READY\n"
printf "Side-camera collision locks=%lu releases=%lu\n", gDownwellCameraLocks, gDownwellCameraReleases
shell touch .run/gdb-ready.marker
detach
quit
