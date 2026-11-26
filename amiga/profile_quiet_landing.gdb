# State-only no-input trajectory through the first landing. Never time this run.
# Build with PROFILE_FRAMES=600 (or more). FS-UAE silently drops guest writes.
set pagination off
set confirm off
set max-value-size unlimited
set $sPatternVerification = 0
target remote 127.0.0.1:2345
break downwellTraceFrameMark
set $sSamples=0
commands
silent
if $sSamples == 0
if gDownwellProfileTargetFrames < 600
printf "DOWNWELL_QUIET_INVALID_LIMIT\n"
quit 1
end
end
set $sWorld=(GameplayWorld*)&'_ZL18portRuntimeStorage.lto_priv.0'
printf "QUIET %u %u %ld %ld %ld %ld %d %d %u %d %d %u %u %u\n", gDownwellProfileFrames[3],$sWorld->tick_,$sWorld->player_.x_,$sWorld->player_.y_,$sWorld->player_.speedX_,$sWorld->player_.speedY_,$sWorld->cameraPixelX_,$sWorld->cameraPixelY_,$sWorld->player_.grounded_,$sWorld->health_,$sWorld->player_.ammo_,$sWorld->enemyCount_,$sWorld->bulletSlotEnd_,(unsigned)$sWorld->enemyRandom_.drawCount_
set $sSamples=$sSamples+1
if !$sWorld->player_.grounded_ && $sSamples < 600
continue
end
end
continue
if !$sWorld->player_.grounded_
printf "DOWNWELL_QUIET_NO_LANDING\n"
quit 1
end
if $sPatternVerification
printf "QUIET_PATTERN_ROWS checks=%lu failures=%lu\n",gDownwellPatternTransferChecks,gDownwellTerrainBatchFailures
if gDownwellPatternTransferChecks == 0 || gDownwellTerrainBatchFailures != 0
quit 1
end
end
printf "DOWNWELL_QUIET_LANDED\n"
shell touch .run/gdb-ready.marker
detach
quit
