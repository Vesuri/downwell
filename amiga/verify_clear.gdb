set pagination off
set confirm off
if $_isvoid($encounter_clear)
    set $encounter_clear = 0
    target remote 127.0.0.1:2345
end
break *main
continue
set $clear_return_pc=*(unsigned long*)$sp
tbreak *$clear_return_pc
commands
    silent
    printf "Executable returned %lu before completing level transitions\n", $d0
    quit 1
end
break downwellClearProbe
commands
silent
printf "Clear phase=%lu frames=%lu choices=%lu failures=%lu reentries=%lu\n", gDownwellClearPhase, gDownwellClearFrames, gDownwellClearChoices, gDownwellClearFailures, gDownwellClearReentries
printf "Gameplay tick=%lu field frames=%lu pixel failures=%lu\n", gDownwellClearTick, gDownwellClearFieldFrames, gDownwellGameBufferFailures
if gDownwellUpgradeSourceBytes != 0 && (gDownwellTerrainRasterResidentBytes != 0 || gDownwellFieldColumnResidentBytes != 0 || gDownwellGameArtResidentBytes != 0 || gDownwellGamePlaneResidentBytes != 0)
    printf "Gameplay resources retained during upgrade presentation\n"
    quit 1
end
if gDownwellClearFailures != 0
    dump binary memory .run/clear-actual-hud.bin gDownwellClearActualHud gDownwellClearActualHud+27264
    dump binary memory .run/clear-expected-hud.bin gDownwellClearExpectedHud gDownwellClearExpectedHud+27264
    dump binary memory .run/clear-actual-game.bin gDownwellClearActualGame gDownwellClearActualGame+30720
    dump binary memory .run/clear-expected-game.bin gDownwellClearExpectedGame gDownwellClearExpectedGame+30720
    quit 1
end
if gDownwellClearPhase >= 100
    quit 1
end
continue
end
break downwellClearReady
continue
if gDownwellClearFrames < 400 || gDownwellClearChoices < 60 || gDownwellClearFailures != 0
    printf "Clear presentation failed: frames=%lu choices=%lu pixel failures=%lu\n", gDownwellClearFrames, gDownwellClearChoices, gDownwellClearFailures
    quit 1
end
if gDownwellClearReentries != 2 || gDownwellClearGameplayUpdates != 60
    printf "Clear departure did not resume gameplay\n"
    quit 1
end
if gDownwellUpgradeSourceBytes != 0
    printf "Upgrade source still resident after returning to gameplay\n"
    quit 1
end
if gDownwellClearOracleBytes != 0
    printf "Upgrade pixel-oracle buffers retained in gameplay\n"
    quit 1
end
if gDownwellSelectionMaskLoads != 2
    printf "Selection mask did not load exactly once per upgrade scene\n"
    quit 1
end
if gDownwellTerrainRasterResidentBytes != 1081344
    printf "Gameplay raster not reacquired after upgrade departure\n"
    quit 1
end
if gDownwellFieldColumnResidentBytes != 70816 || gDownwellFieldColumnLoads != 3
    printf "Gameplay field columns did not reacquire across both upgrade departures\n"
    quit 1
end
if gDownwellGameArtLoads != 3 || gDownwellGameArtReleases != 2 || gDownwellGameArtPreparations != 3 || gDownwellGameArtResidentBytes != gDownwellGameArtExpectedBytes || gDownwellGamePlaneLoads != 3 || gDownwellGamePlaneReleases != 2 || gDownwellGamePlaneResidentBytes != gDownwellGamePlaneExpectedBytes
    printf "Gameplay art did not retire and reacquire across both upgrade departures\n"
    quit 1
end
if gDownwellClearCursorMasks[0] != 7 || gDownwellClearCursorMasks[1] != 7
    printf "Upgrade navigation missed choices: first=%lu second=%lu\n", gDownwellClearCursorMasks[0], gDownwellClearCursorMasks[1]
    quit 1
end
printf "All three upgrade cursors covered on both room entries\n"
if gDownwellClearFieldFrames < 180 || gDownwellGameBufferFailures != 0
    printf "Exit field coverage failed: frames=%lu pixel failures=%lu\n", gDownwellClearFieldFrames, gDownwellGameBufferFailures
    quit 1
end
printf "Exit field: %lu retained planar frames compared\n", gDownwellClearFieldFrames
if gDownwellStackGuardReady != 1
    quit 1
end
set $guard = (unsigned long *)SysBase->ThisTask->tc_SPLower
set $word = 0
while $word < 64
    if $guard[$word] != 0x57ac6a4d
        printf "Clear presenter exhausted stack margin\n"
        quit 1
    end
    set $word = $word + 1
end
printf "Two clears and 60 third-level updates passed; %lu planar frames compared\n", gDownwellClearFrames
if $encounter_clear
    set $owner=(downwell::GameplayCollisionOwner*)&'_ZZL11runDownwellvE16collisionStorage.lto_priv.0'
    set $world=(GameplayWorld*)&'_ZL18portRuntimeStorage.lto_priv.0'
    if !$owner->native_ || !$owner->collisionConsumersAdmitted_ || !$owner->nativeCollisionStorage_ || $owner->owner_.control_.bank_ || $owner->terrain_.records_
        printf "Level transitions did not exercise admitted native ownership\n"
        quit 1
    end
    if !$world->encounterUpdateOwner_ || !$world->collisionPass_ || !$world->run_.effectsSucceeded
        printf "Encounter update scope missing after level transition\n"
        quit 1
    end
    if gDownwellEncounterRunLoads != 1 || gDownwellEncounterSceneLoads != 1 || gDownwellEncounterSceneReleases != 0 || !downwell::AmigaPaulaAudio::current_
        printf "Paula mixer or collision lease did not survive level transitions\n"
        quit 1
    end
    printf "Native encounter scope and the Paula mixer retained through both upgrades\n"
end
if gDownwellSoundBankFailures || !gDownwellSoundReady || gDownwellSoundResidentBank != 1 || gDownwellSoundSceneBank != 0 || gDownwellSoundBankLoads < 3
    printf "SOUND_BANKS_FAILED ready=%lu resident=%lu scene=%lu loads=%lu failures=%lu\n", gDownwellSoundReady, gDownwellSoundResidentBank, gDownwellSoundSceneBank, gDownwellSoundBankLoads, gDownwellSoundBankFailures
    quit 1
end
printf "W2.5 sound banks: run resident, the clear bank loaded for each clear and freed before the next level: loads=%lu chip=%lu\n", gDownwellSoundBankLoads, gDownwellSoundChipBytes
printf "DOWNWELL_CLEAR_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
