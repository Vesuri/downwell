set pagination off
set confirm off
set remotetimeout 30
tbreak downwellReplayComplete
continue
printf "DOWNWELL_REPLAY_RESULT %u %08x %08x\n", gReplayFrames, gReplayDigestHigh, gReplayDigestLow
shell touch .run/gdb-ready.marker
detach
quit
