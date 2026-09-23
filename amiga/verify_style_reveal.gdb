set pagination off
target remote 127.0.0.1:2345
break downwellStyleRevealFrameReady
continue
dump binary memory .run/style-reveal-frame.bin gDownwellStyleFrame gDownwellStyleFrame+12496
dump binary memory .run/style-reveal-mask.bin gDownwellStyleMask gDownwellStyleMask+6248
dump binary memory .run/style-reveal-copper.bin gDownwellStyleCopper gDownwellStyleCopper+400
printf "DOWNWELL_STYLE_REVEAL_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
