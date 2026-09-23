set pagination off
target remote 127.0.0.1:2345
break downwellMenuEntryScrollReady
continue
if gDownwellMenuCopperPatchFailures != 0
    printf "Patched garden copper differs from freshly built oracle\n"
    quit 1
end
printf "DOWNWELL_MENU_ENTRY_SCROLL_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
