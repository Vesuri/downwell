set pagination off
target remote 127.0.0.1:2345
break downwellLockedStyleFrameReady
continue
dump binary memory .run/locked-style-frame.bin gDownwellLockedStyleFrame gDownwellLockedStyleFrame+12496
printf "DOWNWELL_LOCKED_STYLE_FRAME_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
