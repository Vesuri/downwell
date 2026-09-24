set pagination off
target remote 127.0.0.1:2345
tbreak downwellStartupReady
continue
if gDownwellGemHighCapture != 1
    printf "Build with GEMHIGH_VERIFY=1; debugger writes are not used to enable fixtures\n"
    quit 1
end
printf "Gem-high fixture enabled: %lu\n", gDownwellGemHighCapture
tbreak downwellGameplayFrameReady
continue
printf "Landing: tick=%lu fixture=%lu\n", gDownwellCaptureTick, gDownwellGemHighCapture
tbreak downwellGameplayGemHighFrameReady
continue
if gDownwellGemHighCapture != 3 || gDownwellGemHighVisibleShots == 0
    printf "No visible powered projectile in native capture\n"
    quit 1
end
printf "CAPTURE_STATE tick=%lu player_q8=%ld,%ld view=%ld,%ld\n", gDownwellCaptureTick, gDownwellCapturePlayerXQ8, gDownwellCapturePlayerYQ8, gDownwellCaptureViewX, gDownwellCaptureViewY
printf "Powered projectiles visible: %lu\n", gDownwellGemHighVisibleShots
if gDownwellTerrainRasterFailures != 0 || gDownwellTerrainRasterChecks == 0 || gDownwellGameBufferFailures != 0
    printf "Terrain/display ring oracle failed\n"
    quit 1
end
dump binary memory .run/gameplay-gemhigh-frame.bin &gDownwellGameplayRingOrigin &gDownwellGameplayRingOrigin+1
append binary memory .run/gameplay-gemhigh-frame.bin gDownwellGameplayGame gDownwellGameplayGame+30720
append binary memory .run/gameplay-gemhigh-frame.bin gDownwellGameplayHud gDownwellGameplayHud+27264
append binary memory .run/gameplay-gemhigh-frame.bin gDownwellGameplayPlayerSprite gDownwellGameplayPlayerSprite+68
append binary memory .run/gameplay-gemhigh-frame.bin gDownwellGameplayActorSprites gDownwellGameplayActorSprites+408
append binary memory .run/gameplay-gemhigh-frame.bin gDownwellGameplayOverflowSprite gDownwellGameplayOverflowSprite+68
printf "DOWNWELL_GAMEPLAY_GEMHIGH_FRAME_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
