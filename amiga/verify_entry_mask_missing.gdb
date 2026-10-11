set pagination off
set confirm off
target remote 127.0.0.1:2345
break *main
continue
set $return_pc = *(unsigned long*)$sp
# Hide only the disposable mounted copy, using a SKIP_TO_GAME build.
shell mv .run/dh1/Downwell.entrymask .run/dh1/Downwell.entrymask.missing-test
tbreak *$return_pc
continue
shell mv .run/dh1/Downwell.entrymask.missing-test .run/dh1/Downwell.entrymask
if $d0 != 20 || gDownwellEntryMaskLoads != 0 || gDownwellEntryMaskResidentBytes != 0
    printf "Missing entry mask did not fail before publishing its lease\n"
    quit 1
end
printf "DOWNWELL_ENTRY_MASK_FAILURE_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
