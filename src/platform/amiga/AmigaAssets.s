	.section .rodata
	.balign 4
	.globl amiga_hud_pack
amiga_hud_pack:
	.incbin "../generated/amiga-hud.dwap"
	.globl amiga_hud_pack_end
amiga_hud_pack_end:
