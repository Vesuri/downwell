set pagination off
target remote 127.0.0.1:2345
break downwellMenuWakeFrameReady
continue
if gDownwellMenuCopperPatchFailures != 0
    printf "Patched garden copper differs from freshly built oracle\n"
    quit 1
end
set $i = 0
while $i < 68
    if gDownwellMenuPlayerSprite[$i] != gDownwellMenuSpriteOracle[$i]
        printf "Prepared sprite differs at word %u\n", $i
        quit 1
    end
    set $i = $i + 1
end
printf "PREPARED_SPRITE_MATCHES_ORACLE\n"
dump binary memory .run/menu-wake-frame.bin &gDownwellMenuFrameCapture &gDownwellMenuFrameCapture+1
append binary memory .run/menu-wake-frame.bin gDownwellMenuForeground gDownwellMenuForeground+24576
append binary memory .run/menu-wake-frame.bin gDownwellMenuBackground gDownwellMenuBackground+24576
append binary memory .run/menu-wake-frame.bin gDownwellMenuPlayerSprite gDownwellMenuPlayerSprite+68
printf "DOWNWELL_MENU_WAKE_FRAME_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
