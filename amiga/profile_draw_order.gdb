set pagination off
set confirm off
target remote 127.0.0.1:2345
break downwellProfileReady
continue
if gDownwellProfileFrozen != 1 || gDownwellProfileFrames[3] != gDownwellProfileTargetFrames
    printf "Invalid Draw-order audit window\n"
    quit 1
end
if gDownwellActorOrderFailures != 0 || gDownwellActorOrderChecks < gDownwellProfileFrames[3]
    printf "Draw-order comparison failed or coverage incomplete\n"
    quit 1
end
printf "DRAW_ORDER frames=%lu checks=%u failures=%u (instrumented; not performance acceptance)\n", gDownwellProfileFrames[3],gDownwellActorOrderChecks,gDownwellActorOrderFailures
printf "DOWNWELL_PROFILE_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
