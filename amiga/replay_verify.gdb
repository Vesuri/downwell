set pagination off
set confirm off
set remotetimeout 30
tbreak downwellReplayComplete
continue
printf "DOWNWELL_REPLAY_RESULT %u %08x %08x 400 %08x %08x %02x\n", gReplayFrames, gReplayDigestHigh, gReplayDigestLow, gGemReplayDigestHigh, gGemReplayDigestLow, gGemReplayCoverage
shell touch .run/gdb-ready.marker
detach
quit
