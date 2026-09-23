set pagination off
target remote 127.0.0.1:2345
break downwellSceneGameplayActive
continue
if gDownwellSplashPublicationFailures != 0
    printf "Splash list overwritten on %lu frames\n", gDownwellSplashPublicationFailures
    quit 1
end
if gDownwellSplashCheckedFrames[2] != 180 || gDownwellSplashCheckedFrames[3] != 180 || gDownwellSplashCheckedFrames[4] != 240 || gDownwellSplashCheckedFrames[5] != 60 || gDownwellSplashCheckedFrames[6] != 60
    printf "Incomplete splash interval verification\n"
    quit 1
end
printf "SPLASH_PUBLICATION_OK: logo=180 credits=180 controls=240 fade-out=60 fade-in=60\n"
printf "DOWNWELL_GAMEPLAY_ACTIVE\n"
shell touch .run/gdb-ready.marker
detach
quit
