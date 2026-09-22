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
