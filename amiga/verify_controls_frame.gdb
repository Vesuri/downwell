set pagination off
target remote 127.0.0.1:2345
break downwellControlsFrameReady
continue
dump binary memory .run/controls-frame.bin gDownwellControlsFrame gDownwellControlsFrame+12496
printf "DOWNWELL_CONTROLS_FRAME_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
