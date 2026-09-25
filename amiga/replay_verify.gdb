set pagination off
set confirm off
set remotetimeout 30
tbreak downwellReplayComplete
continue
if gDownwellStackGuardReady != 1
    printf "Target could not install the replay task-stack guard\n"
    quit 1
end
set $stack_guard = (unsigned long *)SysBase->ThisTask->tc_SPLower
set $guard_word = 0
while $guard_word < 64
    if $stack_guard[$guard_word] != 0x57ac6a4d
        printf "Replay exhausted the 256-byte task-stack safety margin\n"
        quit 1
    end
    set $guard_word = $guard_word + 1
end
printf "Replay task stack retained its 256-byte safety margin\n"
printf "DOWNWELL_REPLAY_RESULT %u %08x %08x 400 %08x %08x %02x 480 %08x %08x %03x 64 %08x %08x %02x\n", gReplayFrames, gReplayDigestHigh, gReplayDigestLow, gGemReplayDigestHigh, gGemReplayDigestLow, gGemReplayCoverage, gDeathReplayDigestHigh, gDeathReplayDigestLow, gDeathReplayCoverage, gWeaponReplayDigestHigh, gWeaponReplayDigestLow, gWeaponReplayCoverage
printf "DOWNWELL_REPLAY_RESULT MOTION %u %08x %u %08x %u\n", gMotionReplayUpdates, gMotionReplayDigest, gMotionReplayFailures, gMotionBenchDigest, gMotionBenchFailures
printf "Motion benchmark: 1000 updates, %u PAL fields (20 ms/field, endpoint uncertainty <1 field)\n", gMotionBenchFields
printf "DOWNWELL_REPLAY_RESULT BONE %u %08x %u %08x %u\n", gBoneReplayUpdates, gBoneReplayDigest, gBoneReplayFailures, gBoneBenchDigest, gBoneBenchFailures
printf "Bone motion benchmark: 1000 two-axis updates, %u PAL fields\n", gBoneBenchFields
printf "DOWNWELL_REPLAY_RESULT BONE_CREATE %u %08x %u\n", gBoneCreateCount, gBoneCreateDigest, gBoneCreateFailures
shell touch .run/gdb-ready.marker
detach
quit
