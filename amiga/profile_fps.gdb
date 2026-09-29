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
printf "Worst PAL gap: frame=%lu fields=%lu (first maximum; measured gameplay frame 1 starts at tick 0)\n", gDownwellProfileMaxGapFrame[$s], gDownwellProfileMaxGap[$s]
if gDownwellProfileMaxGap[$s] > 0 && (gDownwellProfileMaxGapFrame[$s] == 0 || gDownwellProfileMaxGapFrame[$s] > gDownwellProfileFrames[$s])
    printf "Worst PAL gap frame outside measurement window\n"
    quit 1
end
printf "Damage number births: %lu\n", gDownwellProfileDamageBirths
if $s == 3
    if gDownwellProfileGameSamples != gDownwellProfileFrames[$s]
        printf "Gameplay coverage sample count mismatch: samples=%lu frames=%lu\n", gDownwellProfileGameSamples, gDownwellProfileFrames[$s]
        quit 1
    end
    printf "COVERAGE pre-Step: samples=%lu ticks=%lu..%lu view_y=%ld..%ld max_view_y=%ld max_player_y_q8=%ld final_health=%ld dead_samples=%lu max_enemy_slots=%lu\n", gDownwellProfileGameSamples, gDownwellProfileFirstTick, gDownwellProfileLastTick, gDownwellProfileFirstViewY, gDownwellProfileLastViewY, gDownwellProfileMaxViewY, gDownwellProfileMaxPlayerYQ8, gDownwellProfileLastHealth, gDownwellProfileDeadSamples, gDownwellProfileMaxEnemySlots
    if gDownwellProfileGenerationTiming
        if gDownwellProfileGenerationCalls == 0 || gDownwellProfileGenerationMaxFrame == 0 || gDownwellProfileGenerationMaxFrame > gDownwellProfileFrames[3]
            printf "Invalid coarse phase coverage\n"
            quit 1
        end
        if gDownwellProfileGenerationPhase == 32 && gDownwellProfileGenerationCalls != gDownwellProfileFrames[3]
            printf "Generation call count differs from gameplay frame count\n"
            quit 1
        end
        set $tick_ms = 20.0/(313*256)
        printf "PHASE coarse: phase=%lu calls=%lu total_ms=%.3f mean_call_ms=%.3f max_frame_ms=%.3f max_frame=%lu worst_gap_phase_ms=%.3f clock_clamps=%lu\n", gDownwellProfileGenerationPhase, gDownwellProfileGenerationCalls, gDownwellProfileGenerationTicks*$tick_ms, gDownwellProfileGenerationTicks*$tick_ms/gDownwellProfileGenerationCalls, gDownwellProfileGenerationMax*$tick_ms, gDownwellProfileGenerationMaxFrame, gDownwellProfileGenerationWorstGapTicks*$tick_ms, gDownwellProfileClockClamps
        if gDownwellProfileGenerationWorstGapTicks > (gDownwellProfileMaxGap[3]+1)*313*256
            printf "Generation bracket exceeds worst frame's PAL interval\n"
            quit 1
        end
    end
end
if gDownwellProfileRequireDamage && gDownwellProfileDamageBirths == 0
    printf "Damage profiling tape did not reach a damaging contact\n"
    quit 1
end
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
