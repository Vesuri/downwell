set pagination off
set confirm off
target remote 127.0.0.1:2345
break *main
continue
set $return_pc = *(unsigned long*)$sp
# Only the verifier's disposable mounted copy is hidden, never source/output.
shell mv .run/dh1/Downwell.patterns .run/dh1/Downwell.patterns.missing-test
# The freestanding LTO image has no reliable outer-frame unwind information.
# At main's exact entry the 68000 stack contains its caller's return address.
tbreak *$return_pc
continue
shell mv .run/dh1/Downwell.patterns.missing-test .run/dh1/Downwell.patterns
if $d0 != 20
    printf "Missing builder bank returned %lu instead of RETURN_FAIL\n", $d0
    quit 1
end
if gDownwellBuilderPatternBytes != 0 || gDownwellBuilderPatternLoads != 0 || gDownwellInitialRoomPreparations != 0
    printf "Missing builder bank published a partial gameplay lease\n"
    quit 1
end
printf "DOWNWELL_PATTERN_FAILURE_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
