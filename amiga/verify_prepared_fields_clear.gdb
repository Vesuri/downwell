set pagination off
set confirm off
target remote 127.0.0.1:2345
break downwellPreparedFieldMismatch
commands
    silent
    printf "Exit-field pixels differ: operation=%u check=%u offset=%u actual=%u expected=%u\n", gDownwellPreparedFieldOperation, gDownwellPreparedFieldChecks, gDownwellPreparedFieldMismatchOffset, gDownwellPreparedFieldActualByte, gDownwellPreparedFieldExpectedByte
    quit 1
end
break downwellProfileReady
continue
if gDownwellProfileFrozen != 1 || gDownwellProfileFrames[3] != 480
    printf "Invalid clear verification window\n"
    quit 1
end
set $fixtures = gDownwellPreparedFieldCompositeChecks / 437
set $fixture_fusions = $fixtures * 255
if gDownwellPreparedFieldFailures != 0 || $fixtures < 1 || gDownwellPreparedFieldCompositeChecks % 437 != 0 || gDownwellPreparedFieldFusedChecks < $fixture_fusions + 20
    printf "Missing actual exit-field fusion coverage or pixel failures\n"
    quit 1
end
printf "PROFILE exit field pixels: %u checks, %u fused transfers, %u fixture transfers, %u actual exit transfers; failures=%u\n", gDownwellPreparedFieldChecks, gDownwellPreparedFieldFusedChecks, $fixture_fusions, gDownwellPreparedFieldFusedChecks - $fixture_fusions, gDownwellPreparedFieldFailures
printf "PROFILE exit gameplay frames: %lu\n", gDownwellProfileFrames[3]
printf "PROFILE large-field file reads: %u\n", gDownwellFieldBandLoads
printf "DOWNWELL_PROFILE_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
