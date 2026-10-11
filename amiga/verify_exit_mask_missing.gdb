set pagination off
set confirm off
target remote 127.0.0.1:2345
break *main
continue
set $return_pc = *(unsigned long*)$sp
# Hide only the disposable mounted copy, not the generated or packaged bank.
shell mv .run/dh1/Downwell.exitmask .run/dh1/Downwell.exitmask.missing-test
tbreak *$return_pc
continue
shell mv .run/dh1/Downwell.exitmask.missing-test .run/dh1/Downwell.exitmask
if $d0 != 20 || gDownwellExitMaskLoads != 0
    printf "Missing exit mask did not fail without publishing a lease\n"
    quit 1
end
printf "DOWNWELL_EXIT_MASK_FAILURE_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
