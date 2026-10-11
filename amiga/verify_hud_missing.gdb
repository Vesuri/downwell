set pagination off
set confirm off
target remote 127.0.0.1:2345
break *main
continue
set $return_pc = *(unsigned long*)$sp
# Only hide the disposable mounted copy. Use a SKIP_TO_GAME build.
shell mv .run/dh1/Downwell.hud .run/dh1/Downwell.hud.missing-test
tbreak *$return_pc
continue
shell mv .run/dh1/Downwell.hud.missing-test .run/dh1/Downwell.hud
if $d0 != 20 || gDownwellHudLoads != 0
    printf "Missing HUD did not fail before publishing its lease\n"
    quit 1
end
printf "DOWNWELL_HUD_FAILURE_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
