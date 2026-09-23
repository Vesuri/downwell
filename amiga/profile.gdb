set pagination off
set confirm off
target remote 127.0.0.1:2345
break downwellProfileReady
continue
set $s = gDownwellProfileTargetScene
if gDownwellProfileFrozen != 1 || gDownwellProfileFrames[$s] != gDownwellProfileTargetFrames || gDownwellProfileFields[$s] == 0
    printf "Invalid profiling window\n"
    quit 1
end
printf "PROFILE scene=%lu PROBES (instrumented, not shipping FPS): frames=%lu fields=%lu max_gap=%lu\n", $s, gDownwellProfileFrames[$s], gDownwellProfileFields[$s], gDownwellProfileMaxGap[$s]
printf "Phases: 0=other 1=vblank_wait 2=input 3=simulation 4=title 5=sprite 6=scroll 7=copper 8=publish 9=style_copy 10=style_actors 11=style_text 12=style_drain 13=style_mirror\n"
printf "phase  calls  total_ms  ms/frame  max_bracket_ms\n"
set $i = 0
while $i < 14
    printf "%u %lu %.3f %.3f %.3f\n", $i, gDownwellProfileCalls[$s][$i], gDownwellProfileTicks[$s][$i]*20.0/80128.0, gDownwellProfileTicks[$s][$i]*20.0/80128.0/gDownwellProfileFrames[$s], gDownwellProfileMax[$s][$i]*20.0/80128.0
    set $i = $i + 1
end
printf "max_frame_ms=%.3f clock_clamps=%lu\n", gDownwellProfileFrameMax[$s]*20.0/80128.0, gDownwellProfileClockClamps
printf "DOWNWELL_PROFILE_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
