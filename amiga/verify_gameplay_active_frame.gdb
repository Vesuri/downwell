set pagination off
target remote 127.0.0.1:2345
break downwellGameplayActiveFrameReady
continue
if gDownwellTerrainRasterFailures != 0 || gDownwellTerrainRasterChecks == 0
    printf "Terrain raster differs from fresh composition\n"
    quit 1
end
printf "Terrain raster oracle checks: %lu\n", gDownwellTerrainRasterChecks
if gDownwellGameBufferFailures != 0
    printf "Display ring differs from static backing\n"
    quit 1
end
dump binary memory .run/gameplay-active-frame.bin &gDownwellGameplayRingOrigin &gDownwellGameplayRingOrigin+1
append binary memory .run/gameplay-active-frame.bin gDownwellGameplayGame gDownwellGameplayGame+30720
append binary memory .run/gameplay-active-frame.bin gDownwellGameplayHud gDownwellGameplayHud+27264
append binary memory .run/gameplay-active-frame.bin gDownwellGameplayPlayerSprite gDownwellGameplayPlayerSprite+68
append binary memory .run/gameplay-active-frame.bin gDownwellGameplayActorSprites gDownwellGameplayActorSprites+408
append binary memory .run/gameplay-active-frame.bin gDownwellGameplayOverflowSprite gDownwellGameplayOverflowSprite+68
printf "DOWNWELL_GAMEPLAY_ACTIVE_FRAME_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
