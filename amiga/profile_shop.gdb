set pagination off
set confirm off
target remote 127.0.0.1:2345
break downwellProfileReady
continue
if gDownwellShopProfileDone != 1 || gDownwellShopProfileFailures != 0 || gDownwellShopProfileSideFrames == 0 || gDownwellShopProfileMenuFrames == 0
    printf "Shop tape incomplete/diverged: done=%lu failures=%lu inputs=%lu/%lu side=%lu menu=%lu\n", gDownwellShopProfileDone, gDownwellShopProfileFailures, gDownwellShopProfileFrames[0], gDownwellShopProfileFrames[1], gDownwellShopProfileSideFrames, gDownwellShopProfileMenuFrames
    quit 1
end
if gDownwellProfileFrozen != 1 || gDownwellProfileFrames[3] != gDownwellProfileTargetFrames || gDownwellProfileDeadSamples != 0
    printf "Invalid shop profiling window\n"
    quit 1
end
printf "SHOP_TAPE inputs=%lu/%lu side=%lu menu=%lu final source-state/RNG exact\n", gDownwellShopProfileFrames[0], gDownwellShopProfileFrames[1], gDownwellShopProfileSideFrames, gDownwellShopProfileMenuFrames
printf "SHOP_FIELD_BANK uploads=%lu reuses=%lu\n", gDownwellFieldBankUploads, gDownwellFieldBankReuses
if gDownwellFieldBankUploads == 0 || gDownwellFieldBankUploads + gDownwellFieldBankReuses != 2
    printf "Unexpected field-bank level-entry coverage\n"
    quit 1
end
printf "SHOP_FPS frames=%lu fields=%lu fps=%.2f max_gap=%lu at=%lu\n", gDownwellProfileFrames[3], gDownwellProfileFields[3], 50.0*gDownwellProfileFrames[3]/gDownwellProfileFields[3], gDownwellProfileMaxGap[3], gDownwellProfileMaxGapFrame[3]
set $s=0
while $s<5
    printf "SHOP_SECTION %u frames=%lu fields=%lu max_gap=%lu\n", $s, gDownwellSectionFrames[$s], gDownwellSectionFields[$s], gDownwellSectionMaxGap[$s]
    if gDownwellProfileFrameMax[3] != 0
        set $p=0
        while $p<gDownwellSamplePhaseCount
            if gDownwellSectionTicks[$s][$p] != 0
                printf "SHOP_PHASE section=%u phase=%u total_ms=%.3f max_frame_ms=%.3f\n", $s, $p, gDownwellSectionTicks[$s][$p]*20.0/80128.0, gDownwellSectionMax[$s][$p]*20.0/80128.0
            end
            set $p=$p+1
        end
    end
    set $s=$s+1
end
if gDownwellProfileFrameMax[3] != 0
    printf "SHOP_WORST instrumented frame=%lu ms=%.3f clock_clamps=%lu\n", gDownwellProfileWorstFrame[3], gDownwellProfileFrameMax[3]*20.0/80128.0, gDownwellProfileClockClamps
    set $p=0
    set $sum=0
    while $p<gDownwellSamplePhaseCount
        set $sum=$sum+gDownwellProfileWorstTicks[3][$p]
        if gDownwellProfileWorstTicks[3][$p] != 0
            printf "SHOP_WORST_PHASE %u ms=%.3f\n", $p, gDownwellProfileWorstTicks[3][$p]*20.0/80128.0
        end
        set $p=$p+1
    end
    if $sum != gDownwellProfileFrameMax[3]
        printf "Shop worst-frame phase accounting mismatch\n"
        quit 1
    end
end
printf "DOWNWELL_PROFILE_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
