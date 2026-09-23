set pagination off
target remote 127.0.0.1:2345
break downwellMenuWakeFrameReady
continue
dump binary memory .run/menu-wake-frame.bin &gDownwellMenuFrameCapture &gDownwellMenuFrameCapture+1
append binary memory .run/menu-wake-frame.bin gDownwellMenuForeground gDownwellMenuForeground+24576
append binary memory .run/menu-wake-frame.bin gDownwellMenuBackground gDownwellMenuBackground+24576
append binary memory .run/menu-wake-frame.bin gDownwellMenuPlayerSprite gDownwellMenuPlayerSprite+68
printf "DOWNWELL_MENU_WAKE_FRAME_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
