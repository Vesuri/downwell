| downwellMultiplyBinary64Hook: src/core's multiplyBinary64 for the Amiga
| converter (DOWNWELL_MULTIPLY_BINARY64_HOOK). On a 68020 or 68030 it forms
| the 106-bit significand product with four 32x32->64 mulu.l, step for step
| what multiplyBinary64Portable computes with sixteen 16-bit limb products.
| Elsewhere (install_wide_multiply clear: a 68040 or 68060, whose 64-bit
| forms of mulu.l may be missing) it jumps to the portable function, which
| is also its oracle (binary64_amiga.cpp's self-test).
|
| C++ call: bool (MotionBinary64 a, MotionBinary64 b, MotionBinary64& out),
| arguments on the stack (a.high, a.low, b.high, b.low, &out), result in d0.
| Normal and zero operands only; a subnormal, infinite or NaN operand and a
| result outside the normal range return false and leave out unchanged.

    .text
    .globl downwellMultiplyBinary64Hook
downwellMultiplyBinary64Hook:
    tst.b install_wide_multiply
    bne.s 1f
    jmp installMultiplyBinary64Portable
1:  movem.l %d2-%d7/%a2-%a3,-(%sp)
    move.l 36(%sp),%d0              | a.high
    move.l 44(%sp),%d1              | b.high
    bfextu %d0{#1:#11},%d2          | a's exponent
    bfextu %d1{#1:#11},%d3          | b's exponent
    cmp.w #2047,%d2
    beq.w .Lfail
    cmp.w #2047,%d3
    beq.w .Lfail
    tst.l %d2                       | exponent 0: valid only as a zero
    bne.s 2f
    move.l %d0,%d4
    and.l #0xfffff,%d4
    or.l 40(%sp),%d4
    bne.w .Lfail
2:  tst.l %d3
    bne.s 3f
    move.l %d1,%d4
    and.l #0xfffff,%d4
    or.l 48(%sp),%d4
    bne.w .Lfail
3:  move.l %d0,%d4                  | the sign
    eor.l %d1,%d4
    and.l #0x80000000,%d4
    move.l %d4,%a2
    tst.l %d2                       | a zero operand: a signed zero
    beq.w .Lzero
    tst.l %d3
    beq.w .Lzero
    add.l %d3,%d2                   | exponent before normalisation
    sub.l #1023,%d2
    and.l #0xfffff,%d0              | AH, BH: the high 21 bits with the
    bset #20,%d0                    | implicit one
    and.l #0xfffff,%d1
    bset #20,%d1
    move.l 40(%sp),%d5              | AL * BL -> d6:d5
    mulu.l 48(%sp),%d6:%d5
    move.l 48(%sp),%d7              | AH * BL -> d3:d7
    mulu.l %d0,%d3:%d7
    add.l %d7,%d6
    moveq #0,%d7                    | moveq keeps X
    addx.l %d7,%d3
    move.l 40(%sp),%d7              | AL * BH -> d4:d7
    mulu.l %d1,%d4:%d7
    add.l %d7,%d6                   | d6 = w1
    moveq #0,%d7
    addx.l %d7,%d4
    add.l %d4,%d3                   | X1 + Y1 < 2^23: no carry
    mulu.l %d1,%d7:%d0              | AH * BH -> d7:d0
    add.l %d0,%d3                   | d3 = w2
    moveq #0,%d0
    addx.l %d0,%d7                  | d7 = w3; d5 = w0
    moveq #20,%d4                   | s = 20, or 21 when bit 105 is set
    btst #9,%d7
    beq.s 4f
    moveq #21,%d4
    addq.l #1,%d2
4:  moveq #32,%d1                   | k = 32 - s
    sub.l %d4,%d1
    move.l %d6,%d0                  | mantissa = (w3:w2:w1) >> s
    lsr.l %d4,%d0
    lsl.l %d1,%d6                   | d6: the bits below, guard first
    move.l %d3,%a3
    lsl.l %d1,%d3
    or.l %d0,%d3                    | d3 = mantissa low
    move.l %a3,%d0
    lsr.l %d4,%d0
    lsl.l %d1,%d7
    or.l %d0,%d7                    | d7 = mantissa high
    tst.l %d6                       | nearest-even: guard, then sticky
    bpl.s 6f                        | (the rest and w0) or an odd mantissa
    add.l %d6,%d6
    or.l %d5,%d6
    bne.s 5f
    btst #0,%d3
    beq.s 6f
5:  addq.l #1,%d3
    moveq #0,%d0
    addx.l %d0,%d7
6:  btst #21,%d7                    | carried to 2^53: renormalise
    beq.s 7f
    lsr.l #1,%d7
    roxr.l #1,%d3
    addq.l #1,%d2
7:  tst.l %d2
    ble.s .Lfail
    cmp.l #2047,%d2
    bge.s .Lfail
    moveq #20,%d0
    lsl.l %d0,%d2
    and.l #0xfffff,%d7
    or.l %d7,%d2
    move.l %a2,%d0
    or.l %d0,%d2
    move.l 52(%sp),%a0
    move.l %d2,(%a0)
    move.l %d3,4(%a0)
    moveq #1,%d0
    movem.l (%sp)+,%d2-%d7/%a2-%a3
    rts
.Lzero:
    move.l 52(%sp),%a0
    move.l %a2,(%a0)
    clr.l 4(%a0)
    moveq #1,%d0
    movem.l (%sp)+,%d2-%d7/%a2-%a3
    rts
.Lfail:
    moveq #0,%d0
    movem.l (%sp)+,%d2-%d7/%a2-%a3
    rts

| downwellAddBinary64Hook: src/core's MotionBinary64::add for the Amiga
| converter (DOWNWELL_ADD_BINARY64_HOOK), step for step addPortable, which is
| its oracle. No 64-bit multiply: every CPU from the 68020 takes this path.
|
| C++ call: bool (MotionBinary64* self, MotionBinary64 other), arguments on
| the stack (self, other.high, other.low), result in d0; *self holds the sum.
| A subnormal, infinite or NaN operand and a result outside the normal range
| return false and leave *self unchanged.

    .globl downwellAddBinary64Hook
downwellAddBinary64Hook:
    movem.l %d2-%d7/%a2-%a3,-(%sp)
    move.l 36(%sp),%a1              | self
    move.l (%a1),%d0                | a = *self
    move.l 4(%a1),%d1
    move.l 40(%sp),%d2              | b = other
    move.l 44(%sp),%d3
    bfextu %d0{#1:#11},%d4          | validity, as valid()
    cmp.w #2047,%d4
    beq.w .Lafail
    tst.l %d4
    bne.s 1f
    move.l %d0,%d6
    and.l #0xfffff,%d6
    or.l %d1,%d6
    bne.w .Lafail
1:  bfextu %d2{#1:#11},%d5
    cmp.w #2047,%d5
    beq.w .Lafail
    tst.l %d5
    bne.s 2f
    move.l %d2,%d6
    and.l #0xfffff,%d6
    or.l %d3,%d6
    bne.w .Lafail
2:  move.l %d0,%d6                  | a zero: the other operand, or the
    and.l #0x7fffffff,%d6           | zeros' signs ANDed (-0 + -0 = -0)
    or.l %d1,%d6
    bne.s 3f
    move.l %d2,%d6
    and.l #0x7fffffff,%d6
    or.l %d3,%d6
    beq.s 21f
    move.l %d2,(%a1)
    move.l %d3,4(%a1)
    bra.w .Laok
21: and.l %d2,(%a1)
    bra.w .Laok
3:  move.l %d2,%d6                  | b zero: a unchanged
    and.l #0x7fffffff,%d6
    or.l %d3,%d6
    beq.w .Laok
    move.l %d0,%d6                  | a is the larger magnitude
    and.l #0x7fffffff,%d6
    move.l %d2,%d7
    and.l #0x7fffffff,%d7
    cmp.l %d7,%d6
    bhi.s 4f
    bcs.s 31f
    cmp.l %d3,%d1
    bcc.s 4f
31: exg %d0,%d2
    exg %d1,%d3
    exg %d4,%d5
4:  move.l %d0,%a2                  | the result's sign
    move.l %d0,%a3                  | the signs differ: bit 31 of a3
    move.l %d0,%d6
    eor.l %d2,%d6
    move.l %d6,%a3
    sub.l %d4,%d5                   | d5: eb - ea; then the alignment count
    neg.l %d5
    and.l #0xfffff,%d0              | ma = (implicit one, 52 bits) << 3
    bset #20,%d0
    lsl.l #3,%d0
    move.l %d1,%d6
    moveq #29,%d7
    lsr.l %d7,%d6
    or.l %d6,%d0
    lsl.l #3,%d1
    and.l #0xfffff,%d2              | mb likewise
    bset #20,%d2
    lsl.l #3,%d2
    move.l %d3,%d6
    lsr.l %d7,%d6
    or.l %d6,%d2
    lsl.l #3,%d3
    tst.l %d5                       | mb = mb.rightJam(count)
    beq.s 7f
    cmp.l #64,%d5
    bcs.s 5f
    or.l %d2,%d3
    sne %d3
    and.l #1,%d3
    moveq #0,%d2
    bra.s 7f
5:  cmp.l #32,%d5
    bcs.s 6f
    sub.l #32,%d5                   | n = count - 32
    moveq #0,%d6                    | sticky: low, or high's bits below n
    tst.l %d3
    sne %d6
    tst.l %d5
    beq.s 51f
    moveq #32,%d7
    sub.l %d5,%d7
    move.l %d2,%d3
    lsl.l %d7,%d3
    beq.s 51f
    moveq #1,%d6
51: and.l #1,%d6
    move.l %d2,%d3
    lsr.l %d5,%d3
    or.l %d6,%d3
    moveq #0,%d2
    bra.s 7f
6:  moveq #32,%d7                   | count < 32
    sub.l %d5,%d7
    move.l %d3,%d6
    lsl.l %d7,%d6
    sne %d6
    and.l #1,%d6
    lsr.l %d5,%d3
    move.l %d2,%a0
    lsl.l %d7,%d2
    or.l %d2,%d3
    or.l %d6,%d3
    move.l %a0,%d2
    lsr.l %d5,%d2
7:  move.l %a3,%d6                  | subtract or add the significands
    tst.l %d6
    bpl.s 9f
    sub.l %d3,%d1
    subx.l %d2,%d0
    move.l %d0,%d6
    or.l %d1,%d6
    bne.s 8f
    clr.l (%a1)                     | exact cancellation: +0
    clr.l 4(%a1)
    bra.w .Laok
8:  tst.l %d0                       | renormalise: shift up to 55 places
    beq.s 81f
    bfffo %d0{#0:#32},%d6
    subq.l #8,%d6
    bra.s 82f
81: bfffo %d1{#0:#32},%d6
    add.l #24,%d6
82: sub.l %d6,%d4
    tst.l %d6                       | ma = ma.left(shift)
    beq.s 10f
    cmp.l #32,%d6
    bcs.s 83f
    sub.l #32,%d6
    move.l %d1,%d0
    lsl.l %d6,%d0
    moveq #0,%d1
    bra.s 10f
83: lsl.l %d6,%d0
    moveq #32,%d7
    sub.l %d6,%d7
    move.l %d1,%d5
    lsr.l %d7,%d5
    or.l %d5,%d0
    lsl.l %d6,%d1
    bra.s 10f
9:  add.l %d3,%d1
    addx.l %d2,%d0
    btst #24,%d0                    | carried to bit 56: rightJam(1)
    beq.s 10f
    lsr.l #1,%d0
    roxr.l #1,%d1
    bcc.s 91f
    bset #0,%d1
91: addq.l #1,%d4
10: moveq #0,%d7                    | nearest-even on the three low bits
    move.l %d1,%d6
    and.l #7,%d6
    cmp.l #4,%d6
    bcs.s 11f
    bhi.s 101f
    btst #3,%d1
    beq.s 11f
101: moveq #1,%d7
11: lsr.l #1,%d0                    | ma >>= 3
    roxr.l #1,%d1
    lsr.l #1,%d0
    roxr.l #1,%d1
    lsr.l #1,%d0
    roxr.l #1,%d1
    tst.l %d7
    beq.s 12f
    addq.l #1,%d1
    moveq #0,%d6
    addx.l %d6,%d0
12: btst #21,%d0                    | carried to 2^53: rightJam(1)
    beq.s 13f
    lsr.l #1,%d0
    roxr.l #1,%d1
    bcc.s 121f
    bset #0,%d1
121: addq.l #1,%d4
13: tst.l %d4
    ble.s .Lafail
    cmp.l #2047,%d4
    bge.s .Lafail
    moveq #20,%d6
    lsl.l %d6,%d4
    and.l #0xfffff,%d0
    or.l %d0,%d4
    move.l %a2,%d6
    and.l #0x80000000,%d6
    or.l %d6,%d4
    move.l %d4,(%a1)
    move.l %d1,4(%a1)
.Laok:
    moveq #1,%d0
    movem.l (%sp)+,%d2-%d7/%a2-%a3
    rts
.Lafail:
    moveq #0,%d0
    movem.l (%sp)+,%d2-%d7/%a2-%a3
    rts
