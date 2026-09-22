set pagination off
target remote 127.0.0.1:2345
break downwellMenuTitleSpriteReady
continue
printf "DOWNWELL_MENU_TITLE_SPRITE_READY\n"
detach
quit
