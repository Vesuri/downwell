set pagination off
target remote 127.0.0.1:2345
break downwellStyleRevealFrameReady
continue
dump binary memory .run/style-reveal-frame.bin gDownwellStyleFrame gDownwellStyleFrame+12496
dump binary memory .run/style-reveal-mask.bin gDownwellStyleMask gDownwellStyleMask+6248
# At this gate the gradient starts at display row 128: source row 284.
# Verify both complete immutable masks, including the hidden guard word.
set $masks = gDownwellStyleMask - 156*22
dump binary memory .run/style-all-masks.bin $masks $masks+2*648*22
dump binary memory .run/style-reveal-copper.bin gDownwellStyleCopper gDownwellStyleCopper+400
printf "DOWNWELL_STYLE_REVEAL_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
