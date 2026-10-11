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
if gDownwellSceneAdmissionFailure != 0
    printf "Scene ownership guard failed at startup\n"
    quit 1
end
if gDownwellHudLoads != 0
    printf "Gameplay HUD loaded before its scene admission\n"
    quit 1
end
if gDownwellEntryMaskLoads != 0 || gDownwellEntryMaskResidentBytes != 0
    printf "Gameplay entry mask loaded before its scene admission\n"
    quit 1
end
if gDownwellBuilderPatternBytes != 0 || gDownwellBuilderPatternLoads != 0
    printf "Builder patterns loaded before gameplay entry\n"
    quit 1
end
if gDownwellSceneBankLoads != 1 || gDownwellSplashPreparations != 1 || gDownwellGardenPreparations != 0
    printf "Normal startup loaded/prepared a scene beyond the splash sequence\n"
    quit 1
end
if gDownwellInitialRoomPreparations != 0
    printf "Normal startup constructed the gameplay room before scene entry\n"
    quit 1
end
set $owner = (downwell::GameplayCollisionOwner*)&'_ZZL11runDownwellvE16collisionStorage.lto_priv.0'
if $owner->owner_.control_.bank_ != 0
    printf "Unused scalar control bank resident during startup\n"
    quit 1
end
printf "Startup scalar control: optional_bytes=%lu resident=0\n", sizeof(*$owner->owner_.control_.bank_)
if $owner->terrain_.records_ != 0
    printf "Unused scalar terrain reference backing resident during startup\n"
    quit 1
end
printf "Startup scalar terrain: resident=0\n"
if $owner->geometry_ != 0
    printf "Unused prepared collision geometry resident during startup\n"
    quit 1
end
printf "Startup collision geometry: optional_bytes=%lu resident=0\n", sizeof(*$owner->geometry_)
if $owner->sprites_.prepared_ != 0
    printf "Unadmitted native-endian sprite catalogue resident during startup\n"
    quit 1
end
printf "Startup sprite descriptor cache: resident=0\n"
if $owner->nativeCollisionStorage_ != 0
    printf "Unused collision queue/mask backing resident during startup\n"
    quit 1
end
printf "Startup collision backing: optional_bytes=%lu resident=0 owner_bytes=%lu\n", sizeof(*$owner->nativeCollisionStorage_), sizeof(*$owner)
if $owner->owner_.index_.tree_.tree_ != 0
    printf "Derived spatial tree unexpectedly resident during normal startup\n"
    quit 1
end
if $owner->owner_.index_.population_ != 0
    printf "Unused generic collision index populated during normal startup\n"
    quit 1
end
printf "Startup spatial tree: unmaterialized, no backing allocation\n"
if $owner->owner_.index_.operations_.bank_ != 0
    printf "Unused mutation journal resident during normal startup\n"
    quit 1
end
printf "Startup journal: optional_bytes=%lu resident=0\n", sizeof(*$owner->owner_.index_.operations_.bank_)
if gDownwellFieldBandLoads != 0
    printf "Gameplay field images loaded before gameplay admission\n"
    quit 1
end
if gDownwellFieldColumnLoads != 0 || gDownwellFieldColumnResidentBytes != 0
    printf "Gameplay CPU field columns resident before gameplay admission\n"
    quit 1
end
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
if gDownwellExitMaskLoads != 0 || gDownwellSelectionMaskLoads != 0
    printf "Transition-only masks loaded at normal startup\n"
    quit 1
end
printf "DOWNWELL_MEMORY_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
