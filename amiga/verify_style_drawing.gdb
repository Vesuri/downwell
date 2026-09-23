set pagination off
target remote 127.0.0.1:2345
break downwellStyleFrameReady
continue
dump binary memory .run/style-frame.bin gDownwellStyleFrame gDownwellStyleFrame+12496
delete breakpoints
break downwellLockedStyleFrameReady
continue
dump binary memory .run/locked-style-frame.bin gDownwellLockedStyleFrame gDownwellLockedStyleFrame+12496
delete breakpoints
break downwellSelectedStyleFrameReady
continue
dump binary memory .run/selected-style-frame.bin gDownwellSelectedStyleFrame gDownwellSelectedStyleFrame+12496
dump binary memory .run/selected-style-mask.bin gDownwellStyleMask gDownwellStyleMask+6248
printf "Style full-redraw oracle: %lu frames, %lu mismatches\n", gDownwellStyleReferenceFrames, gDownwellStyleReferenceFailures
if gDownwellStyleReferenceFrames < 100 || gDownwellStyleReferenceFailures != 0
    quit 1
end
printf "DOWNWELL_STYLE_DRAWING_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
