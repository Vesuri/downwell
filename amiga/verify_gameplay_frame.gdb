set pagination off
target remote 127.0.0.1:2345
break downwellGameplayFrameReady
continue
printf "CAPTURE_STATE tick=%lu player_q8=%ld,%ld view=%ld,%ld\n", gDownwellCaptureTick, gDownwellCapturePlayerXQ8, gDownwellCapturePlayerYQ8, gDownwellCaptureViewX, gDownwellCaptureViewY
if gDownwellTerrainRasterFailures != 0 || gDownwellTerrainRasterChecks == 0
    printf "Terrain raster differs from fresh composition\n"
    quit 1
end
if gDownwellGameBufferFailures != 0
    printf "Display ring differs from static backing\n"
    quit 1
end
dump binary memory .run/gameplay-frame.bin &gDownwellGameplayRingOrigin &gDownwellGameplayRingOrigin+1
append binary memory .run/gameplay-frame.bin gDownwellGameplayGame gDownwellGameplayGame+30720
append binary memory .run/gameplay-frame.bin gDownwellGameplayHud gDownwellGameplayHud+27264
append binary memory .run/gameplay-frame.bin gDownwellGameplayPlayerSprite gDownwellGameplayPlayerSprite+68
append binary memory .run/gameplay-frame.bin gDownwellGameplayActorSprites gDownwellGameplayActorSprites+408
append binary memory .run/gameplay-frame.bin gDownwellGameplayOverflowSprite gDownwellGameplayOverflowSprite+68
printf "DOWNWELL_GAMEPLAY_FRAME_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
