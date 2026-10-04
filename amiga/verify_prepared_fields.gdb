set pagination off
set confirm off
target remote 127.0.0.1:2345
break downwellPreparedFieldMismatch
commands
    silent
    printf "Prepared field blitter pixels differ from the independent oracle\n"
    printf "operation=%u check=%u offset=%u actual=%u expected=%u\n", gDownwellPreparedFieldOperation, gDownwellPreparedFieldChecks, gDownwellPreparedFieldMismatchOffset, gDownwellPreparedFieldActualByte, gDownwellPreparedFieldExpectedByte
    print gDownwellPreparedFieldRect
    quit 1
end
break downwellProfileReady
continue
if gDownwellProfileFrozen != 1 || gDownwellProfileGameSamples != gDownwellProfileTargetFrames
    printf "Prepared field verification did not finish the gameplay tape\n"
    quit 1
end
if gDownwellPreparedFieldChecks < 700 || gDownwellPreparedFieldFixtureChecks < 512 || gDownwellPreparedFieldCompositeChecks < 437 || gDownwellPreparedFieldFusedChecks < 150 || gDownwellPreparedFieldFailures != 0
    printf "Prepared field verification has insufficient coverage or failures\n"
    quit 1
end
printf "PROFILE prepared field pixels: %u full-buffer comparisons in %lu gameplay updates; failures=%u\n", gDownwellPreparedFieldChecks, gDownwellProfileGameSamples, gDownwellPreparedFieldFailures
printf "PROFILE terrain-mask phases: %u full-buffer comparisons\n", gDownwellPreparedFieldFixtureChecks
printf "PROFILE overlapping field composites: %u full-buffer comparisons\n", gDownwellPreparedFieldCompositeChecks
printf "PROFILE fused field transfers: %u full-buffer comparisons\n", gDownwellPreparedFieldFusedChecks
printf "PROFILE Chip remaining after allocation: %lu bytes\n", gDownwellStartupChipRemaining
printf "DOWNWELL_PROFILE_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
