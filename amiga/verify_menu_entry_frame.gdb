set pagination off
target remote 127.0.0.1:2345
break downwellMenuEntryFrameReady
continue
dump binary memory .run/menu-entry-frame.bin &gDownwellMenuEntryTitlePartCount &gDownwellMenuEntryTitlePartCount+1
append binary memory .run/menu-entry-frame.bin gDownwellMenuEntryFrame gDownwellMenuEntryFrame+6248
printf "DOWNWELL_MENU_ENTRY_FRAME_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
