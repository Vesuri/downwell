set pagination off
set confirm off
target remote 127.0.0.1:2345
break downwellStartupFailed
break downwellPlaneBlitsReady
continue
printf "PLANE_DMA checks=%u failures=%u startup_failure=%lu\n", gDownwellPlaneBlitChecks, gDownwellPlaneBlitFailures, gDownwellStartupFailure
if gDownwellStartupFailure != 0 || gDownwellPlaneBlitFailures != 0 || gDownwellPlaneBlitChecks == 0
quit 1
end
printf "DOWNWELL_PLANE_DMA_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
