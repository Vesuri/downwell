set pagination off
set confirm off
target remote 127.0.0.1:2345
break downwellOverflowMismatch
break downwellProfileReady
break downwellStartupFailed
break AmigaPlatform::restore
break downwellOverflowProgress
commands
    silent
    printf "OVERFLOW_PROGRESS frames=%lu\n", gDownwellOverflowChecks
    continue
end
continue
printf "OVERFLOW_WITNESS frames=%lu failure_frame=%lu group=%lu actual=%08lx%08lx\n", gDownwellOverflowChecks, gDownwellOverflowFailureFrame, gDownwellOverflowFailureGroup, gDownwellOverflowActual[0], gDownwellOverflowActual[1]
if gDownwellOverflowFailureFrame != 0 || gDownwellOverflowChecks != 1800 || gDownwellProfileFrozen != 1 || gDownwellProfileFrames[3] != 1800
    if gDownwellOverflowFailureGroup == 2
        set $sWorld=(GameplayWorld*)&'_ZL18portRuntimeStorage.lto_priv.0'
        set $sIndex=0
        while $sIndex < $sWorld->enemyCount_
            set $sEnemy=&$sWorld->enemies_[$sIndex]
            printf "ENEMY %u %u %u %d %d %d %d %d %d %ld %ld %ld %ld %d %d %u %u %u %u\n", $sWorld->tick_,$sIndex,$sEnemy->lifetimeId,$sEnemy->objectId,$sEnemy->spriteId,$sEnemy->maskId,$sEnemy->x,$sEnemy->y,$sEnemy->health,$sEnemy->preciseX,$sEnemy->preciseY,$sEnemy->speedX,$sEnemy->speedY,$sEnemy->alarm0,$sEnemy->alarm1,$sEnemy->imagePhase,$sEnemy->imageFrame,$sEnemy->active,$sEnemy->destroyRequested
            set $sIndex=$sIndex+1
        end
    end
    bt 4
    quit 1
end
printf "DOWNWELL_PROFILE_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
