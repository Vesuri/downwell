set pagination off
set confirm off
target remote 127.0.0.1:2345
break *main
continue
set $return_pc = *(unsigned long*)$sp
tbreak downwellRejectEncounterAudio
continue
if gDownwellEncounterAudioRejected != 0
    printf "Rejection occurred before the witnessed callback\n"
    quit 1
end
set $draws = gDownwellEncounterUpdateDraws
set $publications = gDownwellEncounterUpdatePublications
set $w = (GameplayWorld*)&'_ZL18portRuntimeStorage.lto_priv.0'
if !$w->collisionPassExecuting_ || !$w->encounterUpdateOwner_
    printf "Audio callback did not run inside the bound production collision pass\n"
    quit 1
end
tbreak *$return_pc
continue
if $d0 != 20 || gDownwellEncounterAudioRejected != 1
    printf "Callback failure did not return 20 exactly once\n"
    quit 1
end
if gDownwellEncounterUpdateDraws != $draws || gDownwellEncounterUpdatePublications != $publications || $w->run_.effectsSucceeded
    printf "Failed update continued or published a frame\n"
    quit 1
end
if $w->encounterUpdateOwner_ || $w->collisionPass_ || $w->collisionPassExecuting_
    printf "Encounter callback remained bound after exit\n"
    quit 1
end
if downwell::AmigaWindAudio::current_
    printf "Audio device remained installed after exit\n"
    quit 1
end
printf "DOWNWELL_ENCOUNTER_UPDATE_FAILURE_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
