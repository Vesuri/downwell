set pagination off
target remote 127.0.0.1:2345
break downwellSelectedStyleFrameReady
continue
dump binary memory .run/selected-style-frame.bin gDownwellSelectedStyleFrame gDownwellSelectedStyleFrame+12496
printf "DOWNWELL_SELECTED_STYLE_FRAME_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
