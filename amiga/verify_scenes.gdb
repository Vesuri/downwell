set pagination off
target remote 127.0.0.1:2345
break downwellSceneGameplayLanded
continue
printf "DOWNWELL_GAMEPLAY_LANDED\n"
detach
quit
