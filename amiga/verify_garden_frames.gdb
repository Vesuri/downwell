set pagination off
target remote 127.0.0.1:2345
break downwellMenuWakeFrameReady
continue
if gDownwellMenuCopperPatchFailures != 0
    quit 1
end
set $i = 0
while $i < 68
    if gDownwellMenuPlayerSprite[$i] != gDownwellMenuSpriteOracle[$i]
        quit 1
    end
    set $i = $i + 1
end
dump binary memory .run/menu-wake-frame.bin &gDownwellMenuFrameCapture &gDownwellMenuFrameCapture+1
append binary memory .run/menu-wake-frame.bin gDownwellMenuForeground gDownwellMenuForeground+24576
append binary memory .run/menu-wake-frame.bin gDownwellMenuBackground gDownwellMenuBackground+24576
append binary memory .run/menu-wake-frame.bin gDownwellMenuPlayerSprite gDownwellMenuPlayerSprite+68
delete breakpoints
break downwellMenuTitleFrameReady
continue
if gDownwellMenuCopperPatchFailures != 0
    quit 1
end
dump binary memory .run/menu-title-frame.bin &gDownwellMenuFrameCapture &gDownwellMenuFrameCapture+1
append binary memory .run/menu-title-frame.bin gDownwellMenuForeground gDownwellMenuForeground+24576
append binary memory .run/menu-title-frame.bin gDownwellMenuBackground gDownwellMenuBackground+24576
append binary memory .run/menu-title-frame.bin gDownwellMenuPlayerSprite gDownwellMenuPlayerSprite+68
delete breakpoints
break downwellMenuEntryFrameReady
continue
if gDownwellMenuCopperPatchFailures != 0
    quit 1
end
dump binary memory .run/menu-entry-frame.bin &gDownwellMenuFrameCapture &gDownwellMenuFrameCapture+1
append binary memory .run/menu-entry-frame.bin gDownwellMenuEntryFrame gDownwellMenuEntryFrame+27648
append binary memory .run/menu-entry-frame.bin gDownwellMenuBackground gDownwellMenuBackground+24576
append binary memory .run/menu-entry-frame.bin gDownwellMenuForeground gDownwellMenuForeground+24576
printf "DOWNWELL_GARDEN_FRAMES_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
