set pagination off
set confirm off
target remote 127.0.0.1:2345
break downwellProfileReady
continue
set $s = gDownwellProfileTargetScene
if gDownwellProfileFrozen != 1 || gDownwellProfileFrames[$s] != gDownwellProfileTargetFrames || gDownwellProfileFields[$s] == 0
    printf "Invalid profiling window\n"
    quit 1
end
printf "PROFILE scene=%lu PROBES (instrumented, not shipping FPS): frames=%lu fields=%lu max_gap=%lu\n", $s, gDownwellProfileFrames[$s], gDownwellProfileFields[$s], gDownwellProfileMaxGap[$s]
printf "Phases: 0=other 1=vblank_wait 2=input 3=simulation 4=title 5=sprite 6=scroll 7=copper 8=publish 9=style_copy 10=style_actors 11=style_text 12=style_drain 13=style_mirror\n"
printf "phase  calls  total_ms  ms/frame  max_bracket_ms\n"
printf "Additional phases: 14=terrain_collision 15=game_dirty_copy 16=player 17=bullets 18=enemy_alarms 19=enemy_motion 20=enemy_contact 21=gems 22=terrain_walls 23=terrain_tiles 24=terrain_decorations 25=terrain_asset 26=wall_frame\n"
set $i = 0
printf "Ammo phases: 27=base_and_fill 28=rows 29=dividers 30=rim_and_label\n"
printf "Runtime phases: 31=camera 32=generation 33=lifecycle 34=hud_state\n"
printf "Generation phases: 35=object_registration 36=terrain_index_and_tiles\n"
printf "HUD detail phases: 37=damage_scan 38=fill_geometry 39=changed_word_publish\n"
while $i < sizeof(gDownwellProfileTicks[0])/sizeof(gDownwellProfileTicks[0][0])
    printf "%u %lu %.3f %.3f %.3f\n", $i, gDownwellProfileCalls[$s][$i], gDownwellProfileTicks[$s][$i]*20.0/80128.0, gDownwellProfileTicks[$s][$i]*20.0/80128.0/gDownwellProfileFrames[$s], gDownwellProfileMax[$s][$i]*20.0/80128.0
    set $i = $i + 1
end
printf "max_frame_ms=%.3f clock_clamps=%lu\n", gDownwellProfileFrameMax[$s]*20.0/80128.0, gDownwellProfileClockClamps
printf "Collision routes: solid_indexed=%lu solid_fallback=%lu landing_indexed=%lu landing_fallback=%lu breakable_fallback=%lu\n", gDownwellProfileCounters[0], gDownwellProfileCounters[1], gDownwellProfileCounters[2], gDownwellProfileCounters[3], gDownwellProfileCounters[4]
printf "DOWNWELL_PROFILE_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
