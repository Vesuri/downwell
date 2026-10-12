# Build with TRACE_FRAMES=165, PROFILE_FRAMES=600 and no-input gameplay.
# No debugger stops occur inside the measured 164-update fall.
set pagination off
set confirm off
target remote 127.0.0.1:2345
break downwellTraceReady
continue
set $sWorld=(GameplayWorld*)&'_ZL18portRuntimeStorage.lto_priv.0'
if gDownwellProfileFrames[3] != 164 || !$sWorld->player_.grounded_ || gDownwellProfileFields[3] == 0
printf "DOWNWELL_QUIET_INVALID_CADENCE\n"
quit 1
end
printf "QUIET_CADENCE frames=%lu fields=%lu fps=%.3f worst_gap=%lu worst_frame=%lu\n", gDownwellProfileFrames[3],gDownwellProfileFields[3],50.0*gDownwellProfileFrames[3]/gDownwellProfileFields[3],gDownwellProfileMaxGap[3],gDownwellProfileMaxGapFrame[3]
printf "QUIET_LANDING tick=%u x=%ld y=%ld health=%d ammo=%d enemies=%u rng=%u\n", $sWorld->tick_,$sWorld->player_.x_,$sWorld->player_.y_,$sWorld->health_,$sWorld->player_.ammo_,$sWorld->enemyCount_,(unsigned)$sWorld->enemyRandom_.drawCount_
printf "DOWNWELL_QUIET_CADENCE_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
