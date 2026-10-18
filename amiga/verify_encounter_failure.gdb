set pagination off
set confirm off
target remote 127.0.0.1:2345
break *main
continue
set $return_pc = *(unsigned long*)$sp
shell touch .run/dh1/Downwell.reject-draw
# The results route reaches style through the actual frontend. Ask its
# verification-only callback to fail after rendering, before publication.
# The debugger drops target writes; the target reads the mounted marker.
# AmigaPlatform::publish is inlined; its DWARF breakpoint locations also
# include unrelated paths, so count the actual guarded publication site.
tbreak AmigaStyleScene::render
continue
shell rm -f .run/dh1/Downwell.reject-draw
if gDownwellEncounterRejectDraw != 1
    printf "Target did not admit rejection fixture\n"
    quit 1
end
printf "ENCOUNTER_FAILURE_PROGRESS: target armed at style Draw\n"
break AmigaPlatform::restore
commands
    silent
    printf "ENCOUNTER_FAILURE_PROGRESS: restoring platform; rejected=%lu published=%lu\n", gDownwellEncounterDrawRejected, gDownwellEncounterFailurePublications
    continue
end
tbreak *$return_pc
continue
if $d0 != 20
    printf "Encounter failure returned %lu instead of RETURN_FAIL\n", $d0
    quit 1
end
if gDownwellEncounterDrawRejected != 1 || gDownwellEncounterFailurePublications != 0
    printf "Rejected Draw not observed or published: rejected=%lu published=%lu\n", gDownwellEncounterDrawRejected, gDownwellEncounterFailurePublications
    quit 1
end
printf "Encounter failure: Draw withheld, returned 20\n"
printf "DOWNWELL_ENCOUNTER_FAILURE_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
