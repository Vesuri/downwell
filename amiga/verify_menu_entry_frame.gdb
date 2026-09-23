set pagination off
target remote 127.0.0.1:2345
break downwellMenuEntryFrameReady
continue
if gDownwellMenuCopperPatchFailures != 0
    quit 1
end
dump binary memory .run/menu-entry-frame.bin &gDownwellMenuFrameCapture &gDownwellMenuFrameCapture+1
append binary memory .run/menu-entry-frame.bin gDownwellMenuEntryFrame gDownwellMenuEntryFrame+27648
append binary memory .run/menu-entry-frame.bin gDownwellMenuBackground gDownwellMenuBackground+24576
append binary memory .run/menu-entry-frame.bin gDownwellMenuForeground gDownwellMenuForeground+24576
printf "DOWNWELL_MENU_ENTRY_FRAME_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
