set pagination off
target remote 127.0.0.1:2345
break downwellCreditsFrameReady
continue
dump binary memory .run/credits-frame.bin gDownwellCreditsFrame gDownwellCreditsFrame+27264
printf "DOWNWELL_CREDITS_FRAME_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
