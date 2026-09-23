set pagination off
target remote 127.0.0.1:2345
break downwellMenuSpriteReady
continue
printf "DOWNWELL_MENU_SPRITE_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
