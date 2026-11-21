set pagination off
set confirm off
target remote 127.0.0.1:2345
break *main
continue
set $return_pc = *(unsigned long*)$sp
tbreak *$return_pc
continue
if $d0 != 0 || gPaulaProbePassed != 1 || gPaulaProbeFailed != 0
    printf "Paula probe failed: return=%lu reason=%u fields=%u late=%u plays=%u steals=%u drops=%u missing=%u\n", $d0, gPaulaProbeFailed, gPaulaProbeFields, gPaulaProbeLate, gPaulaProbePlays, gPaulaProbeSteals, gPaulaProbeDrops, gPaulaProbeMissing
    quit 1
end
printf "PAULA_MIXER: fields=%u late=%u plays=%u steals=%u drops=%u missing=%u; VBlank service <= %u lines, main frame <= %u lines; bank %u bytes in Chip\n", gPaulaProbeFields, gPaulaProbeLate, gPaulaProbePlays, gPaulaProbeSteals, gPaulaProbeDrops, gPaulaProbeMissing, gPaulaProbeServiceLines, gPaulaProbeFrameLines, gPaulaProbeChipBank
printf "PAULA_APPLY: 16 plays %u lines, 16 gains %u lines\n", gPaulaProbePlay16Lines, gPaulaProbeGain16Lines
printf "PAULA_BENCH (16 each): reset %u, reset+free play %u, stealing play %u, publish+stealing play %u, gain %u lines\n", gPaulaProbeBench[0], gPaulaProbeBench[1], gPaulaProbeBench[2], gPaulaProbeBench[3], gPaulaProbeBench[4]
printf "PAULA_COST: schedule step <= %u lines (field %u), frame() <= %u lines (field %u)\n", gPaulaProbeStepLines, gPaulaProbeStepField, gPaulaProbeMixLines, gPaulaProbeMixField
printf "DOWNWELL_PAULA_MIXER_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
