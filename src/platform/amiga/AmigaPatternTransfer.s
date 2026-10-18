    .section .text.downwellComposePatternRows,"ax"
    .balign 2
    .globl downwellComposePatternRows
    .type downwellComposePatternRows,@function
    # void (descriptors, low, high, rows), ordinary 32-bit stack arguments.
    # Each 44-byte slot starts with four pointers: wall, then three tile groups.
    # The C++ AmigaPatternRow writer remains the exact pixel oracle.
    .macro pattern_plane
    move.w (%a4)+,%d0
    andi.w #3,%d0
    or.w (%a3)+,%d0
    move.w %d0,(%a1)+
    .rept 3
    move.w (%a4)+,%d0
    or.w (%a3)+,%d0
    move.w %d0,(%a1)+
    .endr
    move.w (%a4)+,%d0
    or.w (%a5)+,%d0
    or.w (%a3)+,%d0
    move.w %d0,(%a1)+
    .rept 3
    move.w (%a5)+,%d0
    or.w (%a3)+,%d0
    move.w %d0,(%a1)+
    .endr
    move.w (%a5)+,%d0
    or.w (%a6)+,%d0
    or.w (%a3)+,%d0
    move.w %d0,(%a1)+
    move.w (%a6)+,%d0
    or.w (%a3)+,%d0
    move.w %d0,(%a1)+
    move.w (%a6)+,%d0
    andi.w #0xfffc,%d0
    or.w (%a3)+,%d0
    move.w %d0,(%a1)+
    .endm
downwellComposePatternRows:
    movem.l %d7/%a2-%a6,-(%sp)
    move.l 28(%sp),%a0
    move.l 32(%sp),%a1
    move.l 36(%sp),%a2
    move.l 40(%sp),%d7
    beq.w 2f
    subq.l #1,%d7
1:
    movem.l (%a0),%a3-%a6
    pattern_plane
    addq.l #4,%a6
    exg %a1,%a2
    pattern_plane
    exg %a1,%a2
    lea 26(%a1),%a1
    lea 26(%a2),%a2
    lea 44(%a0),%a0
    dbra %d7,1b
2:
    movem.l (%sp)+,%d7/%a2-%a6
    rts
    .size downwellComposePatternRows,.-downwellComposePatternRows

    .section .text.downwellComposePatternBand,"ax"
    .balign 2
    .globl downwellComposePatternBand
    .type downwellComposePatternBand,@function
    # Same pixels, but four starting pointers describe all rows in this band.
    # pattern_plane consumes 22 wall bytes, 10 A/B bytes and 6 C bytes.
downwellComposePatternBand:
    movem.l %d7/%a2-%a6,-(%sp)
    move.l 28(%sp),%a0
    move.l 32(%sp),%a1
    move.l 36(%sp),%a2
    move.l 40(%sp),%d7
    beq.w 4f
    subq.l #1,%d7
    movem.l (%a0),%a3-%a6
3:
    pattern_plane
    addq.l #4,%a6
    exg %a1,%a2
    pattern_plane
    addq.l #4,%a6
    exg %a1,%a2
    lea 26(%a1),%a1
    lea 26(%a2),%a2
    dbra %d7,3b
4:
    movem.l (%sp)+,%d7/%a2-%a6
    rts
    .size downwellComposePatternBand,.-downwellComposePatternBand

    .section .text.downwellPreparePatternBand,"ax"
    .balign 2
    .globl downwellPreparePatternBand
    .type downwellPreparePatternBand,@function
    # void (band, ordinary interleaved row storage, rows).
    # Same portable pixel oracle as Chip publication, but finish both planes
    # consecutively in the existing 44-byte cache row before preparing another.
downwellPreparePatternBand:
    movem.l %d7/%a3-%a6,-(%sp)
    move.l 24(%sp),%a0
    move.l 28(%sp),%a1
    move.l 32(%sp),%d7
    beq.w 6f
    subq.l #1,%d7
    movem.l (%a0),%a3-%a6
5:
    pattern_plane
    addq.l #4,%a6
    pattern_plane
    addq.l #4,%a6
    dbra %d7,5b
6:
    movem.l (%sp)+,%d7/%a3-%a6
    rts
    .size downwellPreparePatternBand,.-downwellPreparePatternBand
