set pagination off
set confirm off
target remote 127.0.0.1:2345
break downwellStartupFailed
commands
    silent
    printf "Startup failed at check %lu: Chip free=%lu largest=%lu assets=%lu\n", gDownwellStartupFailure, gDownwellStartupChipFree, gDownwellStartupChipLargest, gDownwellStartupAssetBytes
    quit 1
end
break downwellStartupReady
continue
printf "Startup passed: resident assets=%lu, Chip remaining=%lu bytes\n", gDownwellStartupAssetBytes, gDownwellStartupChipRemaining
printf "DOWNWELL_MEMORY_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
