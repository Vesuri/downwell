# No-input diagnostic: the most recent collision pass at selected updates.
# Build with scene_collision_candidate.mk and scene_collision_census.mk,
# FPSCOUNT=1 PROFILE_AUTORUN=1 DETERMINISTIC=1 TRACE_FRAMES=165
# PROFILE_FRAMES=600 PROFILE_IDLE=1 SKIP_TO_GAME=1 STRIP=1.
# State lines must match the uninstrumented build; timings are NOT comparable.
set pagination off
set confirm off
set max-value-size unlimited
target remote 127.0.0.1:2345
break downwellTraceFrameMark
set $samples=0
commands
silent
set $w=(GameplayWorld*)&'_ZL18portRuntimeStorage.lto_priv.0'
printf "STATE %u %u %ld %ld %ld %ld %d %d %u %d %d %u %u %u\n",gDownwellProfileFrames[3],$w->tick_,$w->player_.x_,$w->player_.y_,$w->player_.speedX_,$w->player_.speedY_,$w->cameraPixelX_,$w->cameraPixelY_,$w->player_.grounded_,$w->health_,$w->player_.ammo_,$w->enemyCount_,$w->bulletSlotEnd_,(unsigned)$w->enemyRandom_.drawCount_
if gDownwellProfileFrames[3] == 58 || gDownwellProfileFrames[3] == 136 || gDownwellProfileFrames[3] == 137 || gDownwellProfileFrames[3] == 141
printf "CENSUS frame=%u\n",gDownwellProfileFrames[3]
set $i=0
while $i<386
if downwell::collisionQueryCensus[$i][0]
printf "QUERY object=%u queries=%lu candidates=%lu pairs=%lu empty=%lu\n",$i,downwell::collisionQueryCensus[$i][0],downwell::collisionQueryCensus[$i][1],downwell::collisionQueryCensus[$i][2],downwell::collisionQueryCensus[$i][3]
end
set $i=$i+1
end
end
set $samples=$samples+1
if $samples<165
continue
end
end
continue
printf "ENCOUNTER_CENSUS_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
