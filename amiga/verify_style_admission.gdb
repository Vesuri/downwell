set pagination off
set confirm off
target remote 127.0.0.1:2345
break AmigaPlatform::restore
commands
silent
printf "Unexpected restore before direct-style checkpoint\n"
quit 1
end
break downwellFrameLoopReady
continue
if gDownwellStartupScene != 2 || gDownwellSceneBankLoads != 1 || gDownwellSplashPreparations != 0 || gDownwellGardenPreparations != 0
    printf "Direct style admitted unrelated front-end work\n"
    quit 1
end
if gDownwellGameArtLoads != 0 || gDownwellGamePlaneLoads != 0
    printf "Direct style admitted gameplay assets\n"
    quit 1
end
printf "Direct style: 64 frames, only front bank resident\n"
printf "DOWNWELL_STYLE_ADMISSION_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
