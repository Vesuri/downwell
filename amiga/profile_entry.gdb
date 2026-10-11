set pagination off
set confirm off
target remote 127.0.0.1:2345
# No debugger writes or intermediate stops: input-only traversal, cold frame included.
break downwellProfileReady
continue
if gDownwellProfileFrozen != 1 || gDownwellProfileTargetScene != 3 || gDownwellProfileFrames[3] != 120 || gDownwellProfileFrames[0] == 0 || gDownwellProfileFrames[1] == 0 || gDownwellProfileFrames[2] == 0
    printf "Natural entry tape failed to traverse every startup scene and gameplay\n"
    quit 1
end
printf "PROFILE natural entry PROBES (instrumented); cold gameplay frame retained; clock_clamps=%lu\n", gDownwellProfileClockClamps
printf "ROOM_PLAN startup bind: ticks=%lu ms=%.3f\n", gDownwellRoomPlanTicks, gDownwellRoomPlanTicks*20.0/80128.0
printf "ENTRY scene banks: transient_loads=%lu discarded_for_entry=%lu results_frames=%lu field_loads=%u\n", gDownwellTransientLoads, gDownwellEntryResultsDiscarded, gDownwellEntryResultsFrames, gDownwellFieldBandLoads
printf "ENTRY attribution: room_admission_phase=%u game_art_admission_phase=%u (exclusive phase ticks; nested phases reported separately)\n", profileEntryRoomAdmission, profileEntryGameArtAdmission
printf "ENTRY room detail: pattern_lease_phase=%u metadata_phase=%u creates_phase=%u\n", profileEntryPatternLease, profileEntryRoomMetadata, profileEntryRoomCreates
if @VERIFY_AMMO@
    printf "ENTRY ammo pixels: checks=%lu failures=%lu prepared=%lu\n", gDownwellAmmoChecks, gDownwellAmmoFailures, gDownwellAmmoFactoredUses
    if gDownwellAmmoChecks == 0 || gDownwellAmmoFailures != 0 || gDownwellAmmoFactoredUses == 0
        quit 1
    end
end
set $s = 0
while $s < 4
    printf "SCENE %u frames=%lu fields=%lu max_gap=%lu max_frame_ms=%.3f worst_frame=%lu\n", $s, gDownwellProfileFrames[$s], gDownwellProfileFields[$s], gDownwellProfileMaxGap[$s], gDownwellProfileFrameMax[$s]*20.0/80128.0, gDownwellProfileWorstFrame[$s]
    set $i = 0
    set $sum = 0
    while $i < sizeof(gDownwellProfileTicks[0])/sizeof(gDownwellProfileTicks[0][0])
        if gDownwellProfileCalls[$s][$i] != 0
            printf "phase %u %u calls=%lu total_ms=%.3f max_bracket_ms=%.3f worst_frame_ms=%.3f\n", $s, $i, gDownwellProfileCalls[$s][$i], gDownwellProfileTicks[$s][$i]*20.0/80128.0, gDownwellProfileMax[$s][$i]*20.0/80128.0, gDownwellProfileWorstTicks[$s][$i]*20.0/80128.0
        end
        set $sum = $sum + gDownwellProfileWorstTicks[$s][$i]
        set $i = $i + 1
    end
    if $sum != gDownwellProfileFrameMax[$s] || gDownwellProfileWorstFrame[$s] == 0
        printf "Entry worst-frame accounting mismatch\n"
        quit 1
    end
    set $s = $s + 1
end
dump binary memory .run/entry-hud.bin gDownwellEntryHud gDownwellEntryHud+gDownwellEntryHudBytes
dump binary memory .run/entry-fields.bin gDownwellEntryFields gDownwellEntryFields+gDownwellEntryFieldBytes
if gDownwellResultsPack != 0 || gDownwellResultsAssetBytes != 0 || gDownwellTransientLoads != 0 || gDownwellUpgradeSourceBytes != 0
    printf "Entry loaded an unrelated results/upgrade bank\n"
    quit 1
end
if gDownwellFieldBandLoads != 1
    printf "Entry did not load the field bank exactly once\n"
    quit 1
end
printf "DOWNWELL_PROFILE_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
