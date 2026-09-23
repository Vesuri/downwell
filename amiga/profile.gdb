set pagination off
set confirm off
target remote 127.0.0.1:2345
break downwellProfileReady
continue
if gDownwellProfileFrozen != 1 || gDownwellProfileFrames[1] != 180 || gDownwellProfileFields[1] == 0
    printf "Invalid profiling window\n"
    quit 1
end
printf "GARDEN PROBES (instrumented, not shipping FPS): frames=%lu fields=%lu max_gap=%lu\n", gDownwellProfileFrames[1], gDownwellProfileFields[1], gDownwellProfileMaxGap[1]
printf "Phases: 0=other 1=vblank_wait 2=input 3=simulation 4=title 5=sprite 6=scroll 7=copper 8=publish\n"
printf "phase  calls  total_ms  ms/frame  max_bracket_ms\n"
set $i = 0
while $i < 9
    printf "%u %lu %.3f %.3f %.3f\n", $i, gDownwellProfileCalls[1][$i], gDownwellProfileTicks[1][$i]*20.0/80128.0, gDownwellProfileTicks[1][$i]*20.0/80128.0/gDownwellProfileFrames[1], gDownwellProfileMax[1][$i]*20.0/80128.0
    set $i = $i + 1
end
printf "max_frame_ms=%.3f clock_clamps=%lu\n", gDownwellProfileFrameMax[1]*20.0/80128.0, gDownwellProfileClockClamps
printf "DOWNWELL_PROFILE_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
