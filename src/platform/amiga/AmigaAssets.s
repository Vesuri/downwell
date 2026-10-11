	.section .rodata
	.ifdef DOWNWELL_REFERENCE_SCENE_BANKS
	.balign 4
	.globl amiga_hud_pack
amiga_hud_pack:
	.include "../generated/amiga-health-source.inc"
	.globl amiga_hud_pack_end
amiga_hud_pack_end:
	.endif

	.balign 4
	.globl amiga_effect_stamps
amiga_effect_stamps:
	.incbin "../generated/amiga-effect-stamps.dwef"
	.globl amiga_effect_stamps_end
amiga_effect_stamps_end:

	.ifdef DOWNWELL_EXIT_WIPE_VERIFY
	.balign 4
	.globl amiga_exit_mask
amiga_exit_mask:
	.incbin "../generated/amiga-game.exit-mask"
	.globl amiga_exit_mask_end
amiga_exit_mask_end:
	.endif

	.balign 4
	.globl amiga_entry_mask
amiga_entry_mask:
	.incbin "../generated/amiga-game.entry-mask"
	.globl amiga_entry_mask_end
amiga_entry_mask_end:

	.ifdef DOWNWELL_REFERENCE_SCENE_BANKS
	.balign 4
	.globl amiga_results_pack
amiga_results_pack:
	.incbin "../generated/amiga-results.dwap"
	.globl amiga_results_pack_end
amiga_results_pack_end:

	.balign 4
	.globl amiga_shop_pack
amiga_shop_pack:
	.incbin "../generated/amiga-shop.dwap"
	.globl amiga_shop_pack_end
amiga_shop_pack_end:

	.endif

	.balign 4
	.globl amiga_room_pack
amiga_room_pack:
	.incbin "../generated/downwell-runtime-rooms.dwrm"
	.globl amiga_room_pack_end
amiga_room_pack_end:

	.balign 4
	.globl amiga_object_pack
amiga_object_pack:
	.incbin "../generated/downwell-objects.dwob"
	.globl amiga_object_pack_end
amiga_object_pack_end:

	.balign 4
	.globl amiga_collision_pack
amiga_collision_pack:
	.incbin "../generated/downwell-sprites.dwsc"
	.globl amiga_collision_pack_end
amiga_collision_pack_end:

	.ifdef DOWNWELL_REFERENCE_COLLISION_MASKS
	.balign 4
	.globl amiga_collision_masks
amiga_collision_masks:
	.incbin "../generated/downwell-sprite-masks.dwsm"
	.globl amiga_collision_masks_end
amiga_collision_masks_end:
	.endif
