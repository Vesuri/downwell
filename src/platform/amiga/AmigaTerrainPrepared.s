	.section .rodata
	.balign 4
	.globl amiga_terrain_phase6
amiga_terrain_phase6:
	.incbin "../generated/amiga-terrain-phase6.bin"
	.globl amiga_terrain_phase6_end
amiga_terrain_phase6_end:
	.balign 4
	.globl amiga_initial_terrain
amiga_initial_terrain:
	.incbin "../generated/amiga-initial-terrain.bin"
	.globl amiga_initial_terrain_end
amiga_initial_terrain_end:
