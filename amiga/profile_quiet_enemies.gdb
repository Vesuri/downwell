# Enemy states of the no-input run through the first landing and on, for
# comparing native enemy updates with REFERENCE_ENEMY_UPDATES=1. Never time
# this run. Build with TRACE_FRAMES and PROFILE_FRAMES=600 (or more). Prints
# only fields both motion representations hold alike.
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
set $sEnemy=0
while $sEnemy < $sWorld->enemyCount_
set $e=&$sWorld->enemies_[$sEnemy]
printf "ENEMY %u %u %u %d %d %d %ld %ld %ld %ld %d %d %d %u %u %d %d %d %d %d %d %d %u %u %d %d %d %d %d %d %d %d %d\n", $sWorld->tick_,$sEnemy,$e->lifetimeId,$e->objectId,$e->x,$e->y,$e->preciseX,$e->preciseY,$e->speedX,$e->speedY,$e->facing,$e->alarm0,$e->alarm1,$e->imagePhase,$e->imageFrame,$e->spriteId,$e->grounded,$e->awake,$e->active,$e->destroyRequested,$e->motionState,$e->health,$e->crawler.direction,$e->crawler.imageDirection,$e->crawler.latched,$e->snail.facing,$e->snail.verticalFlip,$e->snail.attached,$e->walker.facing,$e->walker.publishedX,$e->walker.publishedY,$e->walker.grounded,$e->gravityRemainder+256*$e->gravityPositionRemainder
set $sEnemy=$sEnemy+1
end
set $sSamples=$sSamples+1
if $sSamples < 400
continue
end
end
continue
printf "DOWNWELL_QUIET_ENEMIES\n"
shell touch .run/gdb-ready.marker
detach
quit
