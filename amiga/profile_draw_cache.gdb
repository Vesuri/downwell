set pagination off
set confirm off
target remote 127.0.0.1:2345
break downwellProfileReady
continue
if gDownwellProfileFrozen != 1 || gDownwellProfileFrames[3] != gDownwellProfileTargetFrames
    printf "Invalid Draw-cache audit window\n"
    quit 1
end
printf "DRAW_CACHE buckets: 0=all 1=645 2=701 3=844 4=1..300 5=600..660\n"
set $b = 0
while $b < 6
    printf "DRAW_CACHE bucket=%u hits=%lu identical_rebuilds=%lu changed_rebuilds=%lu failed=%lu actors=%lu capture_invalidations=%lu layer_invalidations=%lu identical_actors=%lu\n", $b, gDownwellDrawCacheAudit[$b][0], gDownwellDrawCacheAudit[$b][1], gDownwellDrawCacheAudit[$b][2], gDownwellDrawCacheAudit[$b][3], gDownwellDrawCacheAudit[$b][4], gDownwellDrawCacheAudit[$b][5], gDownwellDrawCacheAudit[$b][6], gDownwellDrawCacheAudit[$b][7]
    if gDownwellDrawCacheAudit[$b][3] != 0
        printf "Failed Draw capture during audit\n"
        quit 1
    end
    set $b = $b+1
end
printf "DRAW_CACHE frames=%lu PAL_fields=%lu (instrumented; not performance acceptance)\n", gDownwellProfileFrames[3],gDownwellProfileFields[3]
printf "DOWNWELL_PROFILE_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
