set pagination off
set confirm off
target remote 127.0.0.1:2345
break downwellSceneGameplayActive
continue
if gDownwellMenuCopperPatchFailures != 0
    printf "Patched garden copper differs from freshly built oracle\n"
    quit 1
end
if gDownwellSplashPublicationFailures != 0
    printf "Splash list overwritten on %lu frames\n", gDownwellSplashPublicationFailures
    quit 1
end
if gDownwellSplashCheckedFrames[2] != 180 || gDownwellSplashCheckedFrames[3] != 180 || gDownwellSplashCheckedFrames[4] != 240 || gDownwellSplashCheckedFrames[5] != 60 || gDownwellSplashCheckedFrames[6] != 60
    printf "Incomplete splash interval verification\n"
    quit 1
end
if gDownwellStyleReferenceFrames < 100 || gDownwellStyleReferenceFailures != 0
    printf "Style sequence comparison failed: frames=%lu mismatches=%lu\n", gDownwellStyleReferenceFrames, gDownwellStyleReferenceFailures
    quit 1
end
if gDownwellStackGuardReady != 1
    printf "Startup stack guard was not initialized\n"
    quit 1
end
set $guard = (unsigned long *)SysBase->ThisTask->tc_SPLower
set $word = 0
while $word < 64
    if $guard[$word] != 0x57ac6a4d
        printf "Startup sequence exhausted stack safety margin\n"
        quit 1
    end
    set $word = $word + 1
end
printf "SPLASH_PUBLICATION_OK: logo=180 credits=180 controls=240 fade-out=60 fade-in=60\n"
printf "STYLE_SEQUENCE_OK: %lu frames, zero mismatches; 256-byte stack guard intact\n", gDownwellStyleReferenceFrames
printf "DOWNWELL_GAMEPLAY_ACTIVE\n"
shell touch .run/gdb-ready.marker
detach
quit
