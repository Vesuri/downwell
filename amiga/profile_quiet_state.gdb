# Read-only no-input fall observations. Run separately from instruction timing.
# The frame marker is before Step; ground state identifies the landing boundary.
set pagination off
set confirm off
set max-value-size unlimited
target remote 127.0.0.1:2345
break downwellTraceFrameMark
set $sSamples=0
commands
silent
set $sWorld=(GameplayWorld*)&'_ZL18portRuntimeStorage.lto_priv.0'
printf "QUIET %u %u %ld %ld %ld %ld %d %d %u %d %d %u %u %u\n", gDownwellProfileFrames[3],$sWorld->tick_,$sWorld->player_.x_,$sWorld->player_.y_,$sWorld->player_.speedX_,$sWorld->player_.speedY_,$sWorld->cameraPixelX_,$sWorld->cameraPixelY_,$sWorld->player_.grounded_,$sWorld->health_,$sWorld->player_.ammo_,$sWorld->enemyCount_,$sWorld->bulletSlotEnd_,(unsigned)$sWorld->enemyRandom_.drawCount_
set $sSamples=$sSamples+1
if $sSamples < 160
continue
end
end
continue
printf "DOWNWELL_PROFILE_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
