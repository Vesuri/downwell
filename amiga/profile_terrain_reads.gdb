# Shape probe only: no timing claim from this instrumented build.
set pagination off
set confirm off
target remote 127.0.0.1:2345
break downwellTraceReady
continue
set $sWorld=(GameplayWorld*)&'_ZL18portRuntimeStorage.lto_priv.0'
if frame != 165 || gDownwellTerrainReadFrames != 165 || gDownwellProfileFrames[3] != 164 || gDownwellTerrainReadActive || gDownwellTerrainConstructionDepth
printf "DOWNWELL_TERRAIN_READS_INVALID_WINDOW\n"
quit 1
end
if $sWorld->tick_ != 164 || $sWorld->player_.y_ != 262144 || !$sWorld->player_.grounded_ || $sWorld->health_ != 4 || $sWorld->player_.ammo_ != 8 || $sWorld->enemyCount_ != 3 || $sWorld->enemyRandom_.drawCount_ != 1371
printf "DOWNWELL_TERRAIN_READS_INVALID_LANDING\n"
quit 1
end
if $sWorld->player_.x_ != 0 || $sWorld->player_.speedX_ != 0 || $sWorld->player_.speedY_ != 0 || $sWorld->cameraPixelX_ != 0 || $sWorld->cameraPixelY_ != 1024 || $sWorld->bulletSlotEnd_ != 0
printf "DOWNWELL_TERRAIN_READS_INVALID_MOTION\n"
quit 1
end
dump binary memory .run/terrain-read-census.bin gDownwellTerrainReadSamples ((char*)gDownwellTerrainReadSamples+sizeof(gDownwellTerrainReadSamples))
printf "TERRAIN_READS updates=164 samples=%lu construction_depth=%u\n",gDownwellTerrainReadFrames,gDownwellTerrainConstructionDepth
printf "DOWNWELL_TERRAIN_READS_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
