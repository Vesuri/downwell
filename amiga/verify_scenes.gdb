set pagination off
target remote 127.0.0.1:2345
break downwellSceneGameplayReady
continue
printf "DOWNWELL_GAMEPLAY_READY\n"
detach
quit
