set pagination off
set confirm off
target remote 127.0.0.1:2345
# profile.sh substitutes @COUNT@, @FIELDS@ and @OUTPUT@. GDB removes the
# breakpoint while stopped, so FS-UAE records the requested PAL fields of
# every instruction without debugger stops inside a capture.
break downwellTraceReady
# A capture can run past a later requested marker. Stop at the end of the
# finite tape instead of waiting for an impossible additional breakpoint.
break downwellProfileReady
set $capture = 0
while $capture < @COUNT@
    continue
    if gDownwellProfileFrozen != 0
        printf "Trace tape ended before all markers were captured; separate overlapping windows\n"
        quit 1
    end
    set $startFrame = gDownwellProfileFrames[3] + 1
    set $startField = gDownwellProfileVbi
    printf "TRACE start frame=%lu vbi=%lu\n", $startFrame, $startField
    eval "monitor profile @FIELDS@ \"\" \"@OUTPUT@/trace-%lu.bin\"", $startFrame
    printf "TRACE end start=%lu frames_completed=%lu fields=%lu\n", $startFrame, gDownwellProfileFrames[3], gDownwellProfileVbi - $startField
    printf "TRACE coverage start=%lu ticks=%lu..%lu last_view_y=%ld max_enemy_slots=%lu health=%ld\n", $startFrame, gDownwellProfileFirstTick, gDownwellProfileLastTick, gDownwellProfileLastViewY, gDownwellProfileMaxEnemySlots, gDownwellProfileLastHealth
    if gDownwellProfileVbi == $startField
        printf "Trace capture did not advance the emulator\n"
        quit 1
    end
    set $capture = $capture + 1
end
printf "DOWNWELL_PROFILE_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
