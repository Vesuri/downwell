set pagination off
set confirm off
target remote 127.0.0.1:2345
break downwellProfileReady
continue
if gDownwellProfileFrozen != 1 || gDownwellProfileFrames[1] != 180 || gDownwellProfileFields[1] == 0
    printf "Invalid profiling window\n"
    quit 1
end
printf "GARDEN: frames=%lu PAL_fields=%lu FPS=%.2f max_gap=%lu fields\n", gDownwellProfileFrames[1], gDownwellProfileFields[1], 50.0*gDownwellProfileFrames[1]/gDownwellProfileFields[1], gDownwellProfileMaxGap[1]
printf "DOWNWELL_PROFILE_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
