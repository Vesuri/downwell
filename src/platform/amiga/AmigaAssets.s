	.section .rodata
	.balign 4
	.globl amiga_hud_pack
amiga_hud_pack:
	.incbin "../generated/amiga-hud.dwap"
	.globl amiga_hud_pack_end
amiga_hud_pack_end:

	.balign 4
	.globl amiga_game_pack
amiga_game_pack:
	.incbin "../generated/amiga-game.dwap"
	.globl amiga_game_pack_end
amiga_game_pack_end:

	.balign 4
	.globl amiga_exit_mask
amiga_exit_mask:
	.incbin "../generated/amiga-game.exit-mask"
	.globl amiga_exit_mask_end
amiga_exit_mask_end:

	.balign 4
	.globl amiga_selection_mask
amiga_selection_mask:
	.incbin "../generated/amiga-game.selection-mask"
	.globl amiga_selection_mask_end
amiga_selection_mask_end:

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

	.balign 4
	.globl amiga_clear_pack
amiga_clear_pack:
	.incbin "../generated/amiga-clear.dwap"
	.globl amiga_clear_pack_end
amiga_clear_pack_end:

	.balign 4
	.globl amiga_room_pack
amiga_room_pack:
	.incbin "../generated/downwell-rooms.dwrm"
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

	.balign 4
	.globl amiga_builder_pack
amiga_builder_pack:
	.incbin "../generated/downwell-builder-patterns.dwbp"
	.globl amiga_builder_pack_end
amiga_builder_pack_end:
