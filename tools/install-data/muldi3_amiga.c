/* libgcc's 64-bit multiply for the Amiga build. The converter is compiled
   for every CPU from the 68020 to the 68060 (-m68020-60), so the compiler
   avoids the 64-bit forms of mulu.l/muls.l and divu.l/divs.l, which the 68060
   leaves to 68060.library; it calls this instead. 16x16->32 products only
   (mulu.w on every 68k); the low 64 bits of the product, as libgcc's. */
typedef unsigned long long u64;
typedef unsigned long u32;

static u32 product16(u32 a, u32 b) { return (u32)(unsigned short)a * (unsigned short)b; }

/* The full 64-bit product of two 32-bit words. */
static u64 product32(u32 a, u32 b)
{
    const u32 lo = product16(a, b), cross0 = product16(a >> 16, b), cross1 = product16(a, b >> 16);
    const u32 mid = (lo >> 16) + (cross0 & 0xffffu) + (cross1 & 0xffffu);
    const u32 hi = product16(a >> 16, b >> 16) + (cross0 >> 16) + (cross1 >> 16) + (mid >> 16);
    return ((u64)hi << 32) | ((mid << 16) | (lo & 0xffffu));
}

u64 __muldi3(u64 a, u64 b)
{
    const u32 al = (u32)a, ah = (u32)(a >> 32), bl = (u32)b, bh = (u32)(b >> 32);
    /* The cross terms only reach the high word: 32-bit products suffice
       (mulu.l's 32-bit form, which the 68060 has). */
    return product32(al, bl) + ((u64)(al * bh + ah * bl) << 32);
}
