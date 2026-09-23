set pagination off
target remote 127.0.0.1:2345
break downwellStyleFrameReady
continue
dump binary memory .run/style-frame.bin gDownwellStyleFrame gDownwellStyleFrame+12496
printf "DOWNWELL_STYLE_FRAME_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
