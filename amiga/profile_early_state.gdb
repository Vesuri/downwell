# Read-only observable-state comparison for saved TRACE_FRAMES executables.
# Use the per-frame marker, not downwellTraceReady (selected windows only).
# These debugger-stopped samples are not timing measurements or full digests.
set pagination off
set confirm off
set max-value-size unlimited
target remote 127.0.0.1:2345
break downwellTraceFrameMark
set $samples=0
commands
silent
# The concrete linked symbol avoids the LTO abstract debug declaration,
# which GDB reports as non-addressable in the historical executable.
set $w=(GameplayWorld*)&'_ZL18portRuntimeStorage.lto_priv.0'
printf "EARLY %u %u %ld %ld %ld %ld %d %d %d %d %u %u %u\n", gDownwellProfileFrames[3],$w->tick_,$w->player_.x_,$w->player_.y_,$w->player_.speedX_,$w->player_.speedY_,$w->cameraPixelX_,$w->cameraPixelY_,$w->health_,$w->player_.ammo_,$w->enemyCount_,$w->bulletSlotEnd_,(unsigned)$w->enemyRandom_.drawCount_
set $samples=$samples+1
if $samples < 80
continue
end
end
continue
printf "DOWNWELL_PROFILE_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
