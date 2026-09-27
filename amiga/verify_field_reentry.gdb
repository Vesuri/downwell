set pagination off
set confirm off
target remote 127.0.0.1:2345
break downwellFieldReentryReady
continue
printf "Field tape: checks=%u stage=%u gameplay=%u upgrade=%u generations=%u/%u/%u\n", gDownwellFieldChecks, gDownwellFieldStage, gDownwellFieldGameSteps, gDownwellFieldClearSteps, gDownwellFieldFirst, gDownwellFieldLater, gDownwellFieldRetry
if gDownwellFieldChecks != 255 || gDownwellFieldStage != 8 || gDownwellFieldGameSteps != 108 || gDownwellFieldClearSteps != 211
    quit 1
end
if gDownwellFieldFirst >= gDownwellFieldLater || gDownwellFieldLater >= gDownwellFieldRetry
    quit 1
end
if gDownwellStackGuardReady != 1 || gDownwellSceneStackFailureStage != 0
    quit 1
end
set $guard = (unsigned long *)SysBase->ThisTask->tc_SPLower
set $i = 0
while $i < 64
    if $guard[$i] != 0x57ac6a4d
        printf "Field tape exhausted stack margin\n"
        quit 1
    end
    set $i = $i + 1
end
printf "DOWNWELL_FIELD_REENTRY_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
