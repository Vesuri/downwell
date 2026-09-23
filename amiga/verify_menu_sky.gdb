set pagination off
set confirm off
target remote 127.0.0.1:2345
break downwellMenuSkyCovered
continue
if gDownwellMenuCopperPatchFailures != 0
    printf "Patched sky-cover copper differs from its reference\n"
    quit 1
end
printf "DOWNWELL_MENU_SKY_COVERED\n"
shell touch .run/gdb-ready.marker
detach
quit
