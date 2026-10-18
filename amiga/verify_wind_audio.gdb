set pagination off
set confirm off
target remote 127.0.0.1:2345
break *main
continue
set $return_pc = *(unsigned long*)$sp
tbreak *$return_pc
continue
dump binary memory .run/wind-audio-trace.bin &gWindProbeTrace &gWindProbeTrace+1
if $d0 != 0 || gWindProbePassed != 1 || gWindProbeFailed != 0
    printf "Wind probe failed: return=%lu passed=%u reason=%u rows=%u irqs=%u\n", $d0, gWindProbePassed, gWindProbeFailed, gWindProbeRows, gWindProbeIrqs
    quit 1
end
printf "WIND_AUDIO: rows=%u interrupts=%u; seven complete PCM streams, memory and device retirement checked\n", gWindProbeRows, gWindProbeIrqs
printf "DOWNWELL_WIND_AUDIO_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
