set pagination off
target remote 127.0.0.1:2345
break downwellSceneGameplayActive
continue
printf "DOWNWELL_GAMEPLAY_ACTIVE\n"
shell touch .run/gdb-ready.marker
detach
quit
