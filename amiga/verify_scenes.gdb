set pagination off
target remote 127.0.0.1:2345
break downwellSceneGameplayActive
continue
printf "DOWNWELL_GAMEPLAY_ACTIVE\n"
detach
quit
