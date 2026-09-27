set pagination off
set confirm off
target remote 127.0.0.1:2345
break downwellSceneStackFailed
break downwellGameplayFrameReady
continue
printf "STACK_STAGE %lu\n", gDownwellSceneStackFailureStage
bt 8
printf "DOWNWELL_STACK_DIAGNOSTIC_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
