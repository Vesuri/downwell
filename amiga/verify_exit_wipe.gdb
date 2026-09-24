set pagination off
set confirm off
target remote 127.0.0.1:2345
break downwellExitWipeReady
continue
if gDownwellExitWipeFailures != 0 || gDownwellExitWipeFrames < 40
    printf "Exit wipe failed: frames=%lu failures=%lu\n", gDownwellExitWipeFrames, gDownwellExitWipeFailures
    quit 1
end
printf "Exit wipe: %lu presented frames, both cover DMA pointers and bank bytes verified\n", gDownwellExitWipeFrames
printf "DOWNWELL_EXIT_WIPE_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
