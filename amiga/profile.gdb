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
printf "Player detail phases: 40=movement_without_nested_terrain 41=shops_and_keepers\n"
printf "Actor detail phases: 42=collection 43=channel_selection 44=hardware_sprite_update\n"
printf "Laser phases: 45=endpoint_trace 46=retention_checks 47=CPU_ray_draw 48=ray_draw_blitter_wait 49=planar_actor_draw 50=ray_restore 51=ray_prepare_blitter_wait\n"
printf "Generation detail: 67=command_create 68=command_initialize 69=terrain_cell 70=landing_index 71=tile_list 72=breakable_register 73=enemy_register 74=shop_create\n"
printf "Damage detail: 75=contact_direction 76=number_create 77=number_step 78=number_draw_including_drain\n"
printf "Controller detail: 79=currency_smoke_shade_composition (excluded from 42=actor_collection)\n"
printf "Collection detail: 80=owned_layer_walk 81=actor_preparation; both excluded from 42, preparation excluded from 80\n"
printf "Birth detail: 82=instance_birth 83=bounds_refresh 84=bounds_trig 85=layer_attach 86=script_effect_birth; all phases exclusive, nested costs excluded from their callers\n"
printf "Bounds detail: 87=sprite_geometry 88=cache_and_spatial_publication; excluded from 83 where nested\n"
printf "Animation detail: 89=room_instance_animation 90=standalone_effect_animation; excluded from 3, includes nested-scope exclusions\n"
printf "Actor animation: 91=enemy_bullet_script_debris_animation; excluded from 89; no-animation entries have no inner probe\n"
printf "Spatial publication: 92=insert 93=remove 94=partition 95=choose_child; exclusive nested costs, excluded from 88 where nested\n"
printf "Draw entry: 96=destroyed_instance_cleanup; excluded from caller (4 in gameplay), includes only pending sweeps\n"
printf "Birth breakdown: 97=owner_registration 98=resource_binding 99=room_order_insertion; excluded from 82\n"
printf "Bullet breakdown: 100=enemy_query_and_impact 101=solid_query_and_impact 102=angle_position_publication; excluded from 17\n"
printf "Damage number births: %lu (zero means this tape did not measure a damaging contact)\n", gDownwellProfileDamageBirths
if gDownwellProfileRequireDamage && gDownwellProfileDamageBirths == 0
    printf "Damage profiling tape did not reach a damaging contact\n"
    quit 1
end
while $i < sizeof(gDownwellProfileTicks[0])/sizeof(gDownwellProfileTicks[0][0])
    printf "%u %lu %.3f %.3f %.3f\n", $i, gDownwellProfileCalls[$s][$i], gDownwellProfileTicks[$s][$i]*20.0/80128.0, gDownwellProfileTicks[$s][$i]*20.0/80128.0/gDownwellProfileFrames[$s], gDownwellProfileMax[$s][$i]*20.0/80128.0
    set $i = $i + 1
end
printf "max_frame_ms=%.3f clock_clamps=%lu\n", gDownwellProfileFrameMax[$s]*20.0/80128.0, gDownwellProfileClockClamps
printf "Worst whole frame: %lu; exclusive phase milliseconds (includes instrumentation and wait)\n", gDownwellProfileWorstFrame[$s]
set $sum = 0
set $i = 0
while $i < sizeof(gDownwellProfileWorstTicks[0])/sizeof(gDownwellProfileWorstTicks[0][0])
    set $ticks = gDownwellProfileWorstTicks[$s][$i]
    set $sum = $sum + $ticks
    if $ticks != 0
        printf "worst_phase %u %.3f\n", $i, $ticks*20.0/80128.0
    end
    set $i = $i + 1
end
if $sum != gDownwellProfileFrameMax[$s] || gDownwellProfileWorstFrame[$s] == 0
    printf "Worst-frame phase accounting mismatch\n"
    quit 1
end
printf "Collision routes: solid_indexed=%lu solid_fallback=%lu landing_indexed=%lu landing_fallback=%lu breakable_fallback=%lu\n", gDownwellProfileCounters[0], gDownwellProfileCounters[1], gDownwellProfileCounters[2], gDownwellProfileCounters[3], gDownwellProfileCounters[4]
printf "Worst-frame terrain spans: %lu (first 16 retained); kind 0=cache 1=dirty 2=forced 3=uncached; cause 0=exposure/camera/prefetch 1=generation 2=shop 3=broken block 4=cold side staging\n", gDownwellProfileWorstSpanCount[$s]
set $i = 0
while $i < gDownwellProfileWorstSpanCount[$s] && $i < 16
    set $span = &gDownwellProfileWorstSpans[$s][$i]
    printf "terrain_span view_x=%ld first_y=%ld height=%ld clip_x=%ld width=%ld kind=%ld cause=%ld\n", $span->viewX, $span->firstY, $span->height, $span->clipX, $span->width, $span->kind, $span->cause
    set $i = $i + 1
end
printf "Terrain segments: indexed=%lu cells=%lu fallback=%lu\n", gDownwellProfileCounters[5], gDownwellProfileCounters[6], gDownwellProfileCounters[7]
printf "Hardware sprite updates: header_only=%lu payload_copies=%lu payload_words=%lu\n", gDownwellProfileCounters[8], gDownwellProfileCounters[9], gDownwellProfileCounters[10]
printf "DOWNWELL_PROFILE_READY\n"
shell touch .run/gdb-ready.marker
detach
quit
