set pagination off
target remote 127.0.0.1:2345
break downwellMenuTitleFrameReady
continue
dump binary memory .run/menu-title-frame.bin &gDownwellMenuFrameCapture &gDownwellMenuFrameCapture+1
append binary memory .run/menu-title-frame.bin gDownwellMenuForeground gDownwellMenuForeground+24576
append binary memory .run/menu-title-frame.bin gDownwellMenuBackground gDownwellMenuBackground+24576
append binary memory .run/menu-title-frame.bin gDownwellMenuPlayerSprite gDownwellMenuPlayerSprite+68
append binary memory .run/menu-title-frame.bin gDownwellMenuTitleSprites gDownwellMenuTitleSprites+476
printf "DOWNWELL_MENU_TITLE_FRAME_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
