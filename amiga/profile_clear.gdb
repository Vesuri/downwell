set pagination off
set confirm off
target remote 127.0.0.1:2345
break downwellProfileReady
continue
if gDownwellProfileFrozen != 1 || gDownwellProfileFrames[3] != 480
    printf "Invalid clear profile window\n"
    quit 1
end
printf "CLEAR PROFILE: 0=approach 1=entry 2=reveal 3=idle 4=navigation 5=departure 6=reentry\n"
printf "Phases: 0=other 1=wait 2=input 3=simulation 52=clear_entry 53=clear_step 54=clear_bank 55=draw_list_and_init 56=border 57=overlay 58=sprites 59=copper 60=drain 61=publish\n"
printf "Detail phases: 62=border_paint 63=overlay_paint 64=trig 65=goal_field_prepare 66=goal_field_draw; parent phases exclude nested phases\n"
set $s = 0
while $s < 7
    set $n = gDownwellSectionFrames[$s]
    if $n > 0
        printf "section %u frames=%lu fields=%lu max_gap=%lu FPS=%.3f\n", $s, $n, gDownwellSectionFields[$s], gDownwellSectionMaxGap[$s], 50.0*$n/gDownwellSectionFields[$s]
        if gDownwellProfileCalls[3][1] > 0
        set $p = 0
        while $p < sizeof(gDownwellSectionTicks[0])/sizeof(gDownwellSectionTicks[0][0])
            if gDownwellSectionTicks[$s][$p] > 0
                printf "phase %u avg_ms=%.4f max_frame_ms=%.4f\n", $p, gDownwellSectionTicks[$s][$p]*20.0/80128.0/$n, gDownwellSectionMax[$s][$p]*20.0/80128.0
            end
            set $p = $p + 1
        end
        end
    end
    set $s = $s + 1
end
if gDownwellProfileCalls[3][1] > 0
    printf "clock_clamps=%lu\n", gDownwellProfileClockClamps
end
printf "DOWNWELL_PROFILE_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
