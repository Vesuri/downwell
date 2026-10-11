set pagination off
set confirm off
target remote 127.0.0.1:2345
source chip_memory_watch.gdb
break downwellStartupFailed
commands
    silent
    printf "Startup failed at check %lu: Chip free=%lu largest=%lu assets=%lu\n", gDownwellStartupFailure, gDownwellStartupChipFree, gDownwellStartupChipLargest, gDownwellStartupAssetBytes
    quit 1
end
break downwellStartupReady
continue
printf "Startup passed: resident assets=%lu, Chip remaining=%lu bytes\n", gDownwellStartupAssetBytes, gDownwellStartupChipRemaining
# This is the normal-startup audit; explicit plane/replay oracle builds have
# their own verifiers and must not weaken the front-end residency guard.
if gDownwellGamePlaneResidentBytes != 0 || gDownwellGamePlaneLoads != 0
    printf "Gameplay DMA unexpectedly resident during normal startup\n"
    quit 1
end
if gDownwellGameArtResidentBytes != 0 || gDownwellGameArtLoads != 0 || gDownwellGameArtPreparations != 0
    printf "Gameplay CPU art unexpectedly loaded/prepared during normal startup\n"
    quit 1
end
downwell-chip-snapshot
if gDownwellTerrainRasterResidentBytes != 0
    printf "Gameplay terrain raster unexpectedly resident during normal startup\n"
    quit 1
end
if $chip_changes == 0 || $chip_minimum > gDownwellStartupChipRemaining
    printf "CHIP_AUDIT_ERROR: watcher missed startup allocations\n"
    quit 1
end
printf "DOWNWELL_MEMORY_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
