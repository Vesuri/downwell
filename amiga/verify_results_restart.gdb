set pagination off
set confirm off
if $_isvoid($encounter_backend)
    set $encounter_backend = 0
    target remote 127.0.0.1:2345
end
source chip_memory_watch.gdb
break *main
continue
set $results_return_pc = *(unsigned long*)$sp
tbreak *$results_return_pc
commands
    silent
    printf "Executable returned %lu before completing results reentry (gameplay failure line %lu, sound banks resident=%lu scene=%lu failures=%lu)\n", $d0, gDownwellGameplayFailLine, gDownwellSoundResidentBank, gDownwellSoundSceneBank, gDownwellSoundBankFailures
    quit 1
end
break downwellStartupFailed
commands
    silent
    printf "Startup failed before results verification: reason=%lu Chip free=%lu largest=%lu remaining=%lu\n", gDownwellStartupFailure, gDownwellStartupChipFree, gDownwellStartupChipLargest, gDownwellStartupChipRemaining
    quit 1
end
break downwellSceneStackFailed
commands
    silent
    printf "Scene stack guard failed before results verification\n"
    quit 1
end
break downwellResultsRestartReady
continue
if gDownwellSceneAdmissionFailure != 0
    printf "Scene ownership guard failed across results re-entry\n"
    quit 1
end
set $owner = (downwell::GameplayCollisionOwner*)&'_ZZL11runDownwellvE16collisionStorage.lto_priv.0'
if $owner->collisionConsumersAdmitted_ != $encounter_backend || !$owner->native_ || !$owner->sceneryLayers_
    printf "Results re-entry is not exercising shipping native scene admission\n"
    quit 1
end
if $owner->owner_.control_.bank_ != 0 || $owner->terrain_.records_ != 0
    printf "Reference control/terrain backing resident after native re-entry\n"
    quit 1
end
if $encounter_backend
    if !$owner->geometry_ || !$owner->nativeCollisionStorage_ || !$owner->owner_.index_.tree_.tree_
        printf "Admitted collision backing missing after reentry\n"
        quit 1
    end
    if gDownwellEncounterRunLoads != 1 || gDownwellEncounterSceneLoads != 3 || gDownwellEncounterSceneReleases != 2
        printf "Encounter leases did not retain Retry and reload for style/surface\n"
        quit 1
    end
    if !downwell::AmigaWindAudio::current_
        printf "Production audio device missing on gameplay reentry\n"
        quit 1
    end
    printf "Native encounter: one run history, three scene leases, two retirements\n"
else
if $owner->geometry_ != 0 || $owner->nativeCollisionStorage_ != 0
    printf "Unadmitted collision backing resident after native re-entry\n"
    quit 1
end
if $owner->owner_.index_.tree_.tree_ != 0 || $owner->owner_.index_.operations_.bank_ != 0 || $owner->owner_.index_.population_ != 0
    printf "Unadmitted spatial service resident or populated after native re-entry\n"
    quit 1
end
printf "Native re-entry: external backend retained; no reference control, terrain, geometry, queue, tree or journal allocation\n"
end
printf "Initial Chip allocation: free=%lu largest=%lu remaining=%lu\n", gDownwellStartupChipFree, gDownwellStartupChipLargest, gDownwellStartupChipRemaining
if gDownwellResultsRestartCount != 3 || gDownwellResultsRestartMask != 7
    printf "Not all results destinations executed\n"
    quit 1
end
if gDownwellResultsHudChecks != 4 || gDownwellResultsHudFailures != 0
    printf "HUD residency failed across results destinations: checks=%lu failures=%lu\n", gDownwellResultsHudChecks, gDownwellResultsHudFailures
    quit 1
end
printf "HUD residency: four presentation entries match the offline bank byte-for-byte\n"
if gDownwellResultsBankChecks != 4 || gDownwellResultsBankFailures != 0
    printf "Unrelated overlay bank resident on gameplay entry: checks=%lu failures=%lu\n", gDownwellResultsBankChecks, gDownwellResultsBankFailures
    quit 1
end
printf "Results artwork: four gameplay entries have no unrelated overlay bank\n"
if gDownwellStackGuardReady != 1
    quit 1
end
if gDownwellOverlayCopperChecks != 640 || gDownwellOverlayCopperFailures != 0
    printf "Retained overlay copper patch verification failed\n"
    quit 1
end
set $stack_guard = (unsigned long *)SysBase->ThisTask->tc_SPLower
set $word = 0
while $word < 64
    if $stack_guard[$word] != 0x57ac6a4d
        printf "Restart exhausted the stack safety margin\n"
        quit 1
    end
    set $word = $word + 1
end
if gDownwellLiveGenerationFailed != 0 || gDownwellTerrainRasterFailures != 0 || gDownwellGameBufferFailures != 0 || gDownwellGameplayBobFailures != 0 || gDownwellStyleReferenceFailures != 0 || gDownwellMenuCopperPatchFailures != 0
    printf "Scene reentry renderer or generation failure\n"
    quit 1
end
# This target builds SKIP_TO_GAME=1: no boot/front bank at startup.
# Style select and surface each load front once; Retry retains gameplay.
if gDownwellStartupScene != 3 || gDownwellSplashPreparations != 0
    printf "Results verifier did not start directly in gameplay\n"
    quit 1
end
if gDownwellSceneBankLoads != 2
    printf "Unexpected scene bank loads: %lu\n", gDownwellSceneBankLoads
    quit 1
end
printf "Scene bank loads: %lu\n", gDownwellSceneBankLoads
if gDownwellGamePlaneLoads != 3 || gDownwellGamePlaneReleases != 2 || gDownwellGamePlaneResidentBytes != gDownwellGamePlaneExpectedBytes || gDownwellGamePlaneExpectedBytes == 0
    printf "Gameplay DMA lease failed: loads=%u releases=%u resident=%u\n", gDownwellGamePlaneLoads, gDownwellGamePlaneReleases, gDownwellGamePlaneResidentBytes
    quit 1
end
printf "Gameplay DMA lease: Retry retained; style/surface released and reloaded\n"
if gDownwellFieldColumnLoads != 3 || gDownwellFieldColumnResidentBytes != 70816
    printf "Field-column lease did not follow gameplay Retry/re-entry\n"
    quit 1
end
if gDownwellGameArtLoads != 3 || gDownwellGameArtReleases != 2 || gDownwellGameArtPreparations != 3 || gDownwellGameArtResidentBytes != gDownwellGameArtExpectedBytes || gDownwellGameArtExpectedBytes == 0
    printf "Gameplay CPU art lease failed: loads=%u releases=%u preparations=%u resident=%u\n", gDownwellGameArtLoads, gDownwellGameArtReleases, gDownwellGameArtPreparations, gDownwellGameArtResidentBytes
    quit 1
end
if gDownwellEntryMaskLoads != gDownwellGameArtLoads || gDownwellEntryMaskResidentBytes != 31104
    printf "Entry mask did not follow the gameplay lease across re-entry\n"
    quit 1
end
printf "Entry mask: three gameplay leases, no reload for retained Retry\n"
printf "Gameplay CPU art lease: zero startup preparation, Retry retained, re-entry rebound\n"
if gDownwellBuilderPatternLoads != 3 || gDownwellBuilderPatternReleases != 2 || gDownwellBuilderPatternBytes != 70779
    printf "Builder pattern lease failed: loads=%u releases=%u resident=%u\n", gDownwellBuilderPatternLoads, gDownwellBuilderPatternReleases, gDownwellBuilderPatternBytes
    quit 1
end
printf "Builder pattern lease: Retry retained; retired terrain allowed style/surface unload and re-entry\n"
printf "Three results destinations executed; final gameplay reached 200 updates\n"
printf "DOWNWELL_RESULTS_RESTART_READY\n"
downwell-chip-snapshot
shell touch .run/gdb-ready.marker
detach
quit
