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
    bt 4
    quit 1
end
printf "DOWNWELL_PROFILE_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
