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
printf "PROFILE scene=%lu: frames=%lu PAL_fields=%lu FPS=%.2f max_gap=%lu fields\n", $s, gDownwellProfileFrames[$s], gDownwellProfileFields[$s], 50.0*gDownwellProfileFrames[$s]/gDownwellProfileFields[$s], gDownwellProfileMaxGap[$s]
set $i = 0
printf "Staged camera transfers=%ld last_shortfall=%ld rows\n", gDownwellProfileStagedHits, gDownwellProfileStageShortfall
while $i < gDownwellProfileMissCount && $i < 32
    printf "Missed deadline: frame=%lu fields=%lu scroll=%ld camera_x_change=%ld\n", gDownwellProfileMissFrame[$i], gDownwellProfileMissFields[$i], gDownwellProfileMissScroll[$i], gDownwellProfileMissCamera[$i]
    set $i = $i + 1
end
printf "DOWNWELL_PROFILE_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
