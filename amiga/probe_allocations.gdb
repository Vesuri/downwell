# WR.5: every exec AllocMem/FreeMem the program makes until the quiet-fall
# trace point (a profile.sh quiet-fall build). Each line names the size, the
# flags and the calling function (FREE lines give the freed address).
# GDB_SOURCE=probe_allocations.gdb READY_MARKER=DOWNWELL_ALLOCATIONS_READY
# bash verify_startup.sh (VERIFY_PROGRAM defaults to out/Downwell).
set pagination off
set confirm off
target remote 127.0.0.1:2345
set $sExec=*(unsigned long*)4
set $sAlloc=*(unsigned long*)($sExec-198+2)
set $sFree=*(unsigned long*)($sExec-210+2)
break *$sAlloc
commands
silent
printf "ALLOC %lu %lx ", $d0, $d1
info symbol *(unsigned long*)$sp
continue
end
break *$sFree
commands
silent
printf "FREE %lu %lx ", $d0, $a1
info symbol *(unsigned long*)$sp
continue
end
break downwellTraceReady
continue
delete
printf "DOWNWELL_ALLOCATIONS_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
