# WR.1: free memory after boot (tests/platform/amiga/FreeMemoryProbe.cpp).
# MACHINE=a1200 bash amiga/verify_startup.sh with GDB_SOURCE set to this,
# VERIFY_PROGRAM=out/FreeMemoryProbe and READY_MARKER=DOWNWELL_FREE_MEMORY_READY.
set pagination off
set confirm off
target remote 127.0.0.1:2345
break downwellFreeMemoryMeasured
continue
printf "FREE_MEMORY chip=%lu chip_largest=%lu fast=%lu fast_largest=%lu public=%lu public_largest=%lu\n", gFreeChip, gFreeChipLargest, gFreeFast, gFreeFastLargest, gFreePublic, gFreePublicLargest
printf "DOWNWELL_FREE_MEMORY_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
