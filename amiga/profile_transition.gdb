set pagination off
set confirm off
target remote 127.0.0.1:2345
# The compile-time stop condition runs through garden/well and 60 style frames.
# No target writes or intermediate breakpoints inside the traversal.
break downwellProfileReady
continue
printf "Transition coverage: frozen=%lu target=%lu frames=%lu splash=%lu garden=%lu style=%lu gameplay=%lu\n", gDownwellProfileFrozen, gDownwellProfileTargetScene, gDownwellProfileTargetFrames, gDownwellProfileFrames[0], gDownwellProfileFrames[1], gDownwellProfileFrames[2], gDownwellProfileFrames[3]
if gDownwellProfileFrozen != 1 || gDownwellProfileFrames[2] != 60 || gDownwellProfileFrames[0] == 0 || gDownwellProfileFrames[1] == 0
    printf "Transition profiling did not traverse splash, garden/well and style reveal\n"
    quit 1
end
printf "PROFILE transition PROBES (instrumented, not shipping FPS); scenes 0=splash 1=garden/well 2=style; clock_clamps=%lu\n", gDownwellProfileClockClamps
set $s = 0
while $s < 3
    printf "SCENE %u frames=%lu fields=%lu max_gap=%lu max_frame_ms=%.3f worst_frame=%lu\n", $s, gDownwellProfileFrames[$s], gDownwellProfileFields[$s], gDownwellProfileMaxGap[$s], gDownwellProfileFrameMax[$s]*20.0/80128.0, gDownwellProfileWorstFrame[$s]
    set $i = 0
    set $sum = 0
    while $i < sizeof(gDownwellProfileTicks[0])/sizeof(gDownwellProfileTicks[0][0])
        if gDownwellProfileMax[$s][$i] > gDownwellProfileFrameMax[$s]
            printf "Phase bracket exceeds measured frame window\n"
            quit 1
        end
        if gDownwellProfileCalls[$s][$i] != 0
            printf "phase %u %u calls=%lu total_ms=%.3f per_frame_ms=%.3f max_bracket_ms=%.3f worst_frame_ms=%.3f\n", $s, $i, gDownwellProfileCalls[$s][$i], gDownwellProfileTicks[$s][$i]*20.0/80128.0, gDownwellProfileTicks[$s][$i]*20.0/80128.0/gDownwellProfileFrames[$s], gDownwellProfileMax[$s][$i]*20.0/80128.0, gDownwellProfileWorstTicks[$s][$i]*20.0/80128.0
        end
        set $sum = $sum + gDownwellProfileWorstTicks[$s][$i]
        set $i = $i + 1
    end
    if $sum != gDownwellProfileFrameMax[$s] || gDownwellProfileWorstFrame[$s] == 0
        printf "Transition worst-frame accounting mismatch\n"
        quit 1
    end
    set $s = $s + 1
end
printf "DOWNWELL_PROFILE_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
