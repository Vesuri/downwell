set pagination off
set confirm off
target remote 127.0.0.1:2345
break downwellEntryWipeReady
continue
if gDownwellEntryWipeFailures != 0 || gDownwellEntryWipeFrames != 25
    printf "Entry wipe failed: frames=%lu failures=%lu\n", gDownwellEntryWipeFrames, gDownwellEntryWipeFailures
    quit 1
end
printf "Entry wipe: 25 frames, both cover DMA pointers and immutable bank verified\n"
printf "DOWNWELL_ENTRY_WIPE_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
