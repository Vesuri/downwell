# Same build as quiet-fall; no breakpoints remain during the 75-field capture.
set pagination off
set confirm off
target remote 127.0.0.1:2345
break downwellTraceFrameMark
ignore 1 55
continue
if frame != 56 || gDownwellProfileTargetScene != 3 || gDownwellProfileTargetFrames < 600
printf "DOWNWELL_QUIET_INVALID_WINDOW\n"
quit 1
end
delete 1
printf "TRACE quiet-window start=56 fields=75\n"
monitor profile 75 "" "@OUTPUT@/trace-56.bin"
printf "DOWNWELL_PROFILE_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
