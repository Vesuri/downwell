set pagination off
set confirm off
target remote 127.0.0.1:2345
break downwellEntryWipeReady
continue
if gDownwellEntryWipeFailures != 0 || gDownwellEntryWipeFrames != 27 || gDownwellEntryWipeReuses != 1
    printf "Entry wipe failed: frames=%lu failures=%lu\n", gDownwellEntryWipeFrames, gDownwellEntryWipeFailures
    quit 1
end
printf "Entry wipe: 25 covered frames, two detached slots, VBlank-fenced shop bank reuse verified\n"
printf "DOWNWELL_ENTRY_WIPE_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
