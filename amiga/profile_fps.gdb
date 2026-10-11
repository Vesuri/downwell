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
if gDownwellProfileIdle
    printf "IDLE_INPUT: nonzero_samples=%lu bullet_slots=%lu\n", gDownwellIdleInputViolations, gDownwellIdleBulletSlots
    if gDownwellIdleInputViolations != 0 || gDownwellIdleBulletSlots != 0 || gDownwellProfileDamageBirths != 0 || gDownwellProfileDeadSamples != 0 || gDownwellProfileLastHealth != 4
        printf "Idle route is not an undamaged no-input/no-projectile baseline\n"
        quit 1
    end
end
printf "ROOM_PLAN preparation: PAL_fields=%lu approximately_ms=%lu (20 ms resolution, before first presented frame)\n", gDownwellRoomPlanFields, gDownwellRoomPlanFields*20
printf "PREPARATION terrain: calls=%lu PAL_fields=%lu approximately_ms=%lu (20 ms counter resolution, before takeover)\n", gDownwellTerrainPreparationCalls, gDownwellTerrainPreparationFields, gDownwellTerrainPreparationFields*20
if gDownwellTerrainPreparationCalls != gDownwellTerrainPreparationExpectedCalls
    printf "Unexpected terrain preparation coverage\n"
    quit 1
end
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
printf "Transient scene residency: loads=%lu upgrade_source_bytes=%lu results_bytes=%lu shop_bytes=%lu\n", gDownwellTransientLoads, gDownwellUpgradeSourceBytes, gDownwellResultsAssetBytes, gDownwellShopAssetBytes
printf "Front-end residency: startup_scene=%lu bank_loads=%lu splash_preparations=%lu garden_preparations=%lu\n", gDownwellStartupScene, gDownwellSceneBankLoads, gDownwellSplashPreparations, gDownwellGardenPreparations
if gDownwellStartupScene == 3 && gDownwellProfileRequireDamage && gDownwellProfileDeadSamples == 0
    if gDownwellSceneBankLoads != 0 || gDownwellSplashPreparations != 0 || gDownwellGardenPreparations != 0
        printf "Gameplay-only contact route admitted front-end assets or preparation\n"
        quit 1
    end
end
# The living world-1 contact tape never enters a shop, results or upgrade room.
# New scene content must not introduce bank loading/preparation to this route.
if gDownwellProfileRequireDamage && gDownwellProfileDeadSamples == 0
    if gDownwellInitialRoomPreparations != 1
        printf "Gameplay room was not prepared exactly once at entry\n"
        quit 1
    end
    if gDownwellGameArtLoads != 1 || gDownwellGameArtPreparations != 1 || gDownwellGameArtReleases != 0 || gDownwellGameArtResidentBytes != gDownwellGameArtExpectedBytes
        printf "Gameplay art reloaded/prepared during the live contact tape\n"
        quit 1
    end
    printf "Gameplay CPU art: one entry load/preparation, %lu resident bytes\n", gDownwellGameArtResidentBytes
    if gDownwellTerrainRasterResidentBytes != 1081344
        printf "Gameplay terrain raster lease missing or wrong size\n"
        quit 1
    end
    printf "Gameplay terrain raster: %lu ordinary-memory bytes\n", gDownwellTerrainRasterResidentBytes
    if gDownwellTransientLoads != 0 || gDownwellUpgradeSourceBytes != 0 || gDownwellResultsAssetBytes != 0 || gDownwellShopAssetBytes != 0
        printf "Unrelated transient scene work on the contact tape\n"
        quit 1
    end
end
set $i = 0
printf "Staged camera transfers=%ld last_shortfall=%ld rows\n", gDownwellProfileStagedHits, gDownwellProfileStageShortfall
while $i < gDownwellProfileMissCount && $i < 32
    printf "Missed deadline: frame=%lu fields=%lu scroll=%ld camera_x_change=%ld\n", gDownwellProfileMissFrame[$i], gDownwellProfileMissFields[$i], gDownwellProfileMissScroll[$i], gDownwellProfileMissCamera[$i]
    set $i = $i + 1
end
if gDownwellSampleFocus != 0
    set $i = 0
    set $total = 0
    set $focused = 0
    while $i < gDownwellSamplePhaseCount
        set $total = $total + gDownwellPhaseSamples[$i]
        set $focused = $focused + gDownwellFocusSamples[$i]
        if gDownwellPhaseSamples[$i] || gDownwellFocusSamples[$i]
            printf "SAMPLE phase=%u fields=%lu focus_fields=%lu\n", $i, gDownwellPhaseSamples[$i], gDownwellFocusSamples[$i]
        end
        set $i = $i + 1
    end
    printf "SAMPLE total=%lu focus_frame=%lu focus_total=%lu (statistical, not phase timings)\n", $total, gDownwellSampleFocus, $focused
    if $total == 0 || $focused == 0
        printf "Sampling window was not reached\n"
        quit 1
    end
end
# Read storage directly: calling nodeCount()/height() would drain the journal
# and change the very residency/work state this diagnostic is measuring.
set $owner = (downwell::GameplayCollisionOwner*)&'_ZZL11runDownwellvE16collisionStorage.lto_priv.0'
set $tree = $owner->owner_.index_.tree_.tree_
printf "Spatial storage: owner_bytes=%lu tree_capacity_bytes=%lu journal_bytes=%lu pending=%lu population=%lu deferred=%u resident=%u\n", sizeof(*$owner), sizeof(*$tree), sizeof($owner->owner_.index_.operations_), $owner->owner_.index_.count_, $owner->owner_.index_.population_, $owner->owner_.index_.deferred_, $tree != 0
if $tree != 0
    printf "Spatial materialized tree: used=%lu live=%lu\n", $tree->used_, $tree->live_
end
printf "DOWNWELL_PROFILE_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
