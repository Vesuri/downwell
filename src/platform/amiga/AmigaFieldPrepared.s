	.section .rodata
	.balign 4
	.globl amiga_field_images
amiga_field_images:
	.incbin "../generated/amiga-field-images.bin"
	.globl amiga_field_images_end
amiga_field_images_end:
