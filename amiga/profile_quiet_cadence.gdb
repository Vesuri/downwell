# Build with TRACE_FRAMES=165, PROFILE_FRAMES=600 and no-input gameplay.
# No debugger stops occur inside the measured 164-update fall.
set pagination off
set confirm off
target remote 127.0.0.1:2345
break downwellTraceReady
continue
set $sWorld=(GameplayWorld*)&'_ZL18portRuntimeStorage.lto_priv.0'
if frame != 165 || gDownwellProfileTargetScene != 3 || gDownwellProfileTargetFrames < 600 || gDownwellProfileFrames[3] != 164 || !$sWorld->player_.grounded_ || gDownwellProfileFields[3] == 0
printf "DOWNWELL_QUIET_INVALID_CADENCE\n"
quit 1
end
printf "QUIET_CADENCE frames=%lu fields=%lu fps=%.3f worst_gap=%lu worst_frame=%lu\n", gDownwellProfileFrames[3],gDownwellProfileFields[3],50.0*gDownwellProfileFrames[3]/gDownwellProfileFields[3],gDownwellProfileMaxGap[3],gDownwellProfileMaxGapFrame[3]
printf "QUIET_LANDING tick=%u x=%ld y=%ld health=%d ammo=%d enemies=%u rng=%u\n", $sWorld->tick_,$sWorld->player_.x_,$sWorld->player_.y_,$sWorld->health_,$sWorld->player_.ammo_,$sWorld->enemyCount_,(unsigned)$sWorld->enemyRandom_.drawCount_
printf "QUIET_MOTION vx=%ld vy=%ld camera_x=%d camera_y=%d grounded=%u bullets=%u\n",$sWorld->player_.speedX_,$sWorld->player_.speedY_,$sWorld->cameraPixelX_,$sWorld->cameraPixelY_,$sWorld->player_.grounded_,$sWorld->bulletSlotEnd_
# Read the existing bounded missed-field ledger only after the run. Do not
# introduce stops or instrumentation into the measured fall. An overflow is
# partial evidence, not a complete distribution of missed fields.
set $sMissCapacity=sizeof(gDownwellProfileMissFrame)/sizeof(gDownwellProfileMissFrame[0])
set $sMissSaved=gDownwellProfileMissCount
if $sMissSaved > $sMissCapacity
set $sMissSaved=$sMissCapacity
end
printf "QUIET_MISSES total=%lu saved=%lu\n",gDownwellProfileMissCount,$sMissSaved
set $sMissIndex=0
while $sMissIndex < $sMissSaved
printf "QUIET_MISS frame=%lu fields=%lu\n",gDownwellProfileMissFrame[$sMissIndex],gDownwellProfileMissFields[$sMissIndex]
set $sMissIndex=$sMissIndex+1
end
printf "DOWNWELL_QUIET_CADENCE_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
