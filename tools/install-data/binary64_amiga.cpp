/* The Amiga converter's binary64 multiply and add (binary64_amiga.s): which
   multiply path it takes, its portable fallback, and a self-test of both
   twins against their portable oracles. */
#include "core/MultiplyBinary64.h"
#include <exec/execbase.h>

using downwell::MotionBinary64;

extern "C" {

/* Set by install_cpu_init: the hardware 32x32->64 multiply is used only on a
   68020 or 68030. Kickstart before 68060.library reports a 68060 as a 68040,
   and a 68060 lacks that instruction, so neither flag takes the fast path. */
unsigned char install_wide_multiply;

void install_cpu_init(void)
{
    struct ExecBase* sys;
    __asm__ volatile("move.l 4.w,%0" : "=a"(sys));
    const UWORD flags = sys->AttnFlags;
    install_wide_multiply = (flags & AFF_68020) && !(flags & (AFF_68040 | (1u << 7))) ? 1 : 0;
}

bool installMultiplyBinary64Portable(MotionBinary64 a, MotionBinary64 b, MotionBinary64& out)
{
    return downwell::multiplyBinary64Portable(a, b, out);
}

#ifdef INSTALL_MULTIPLY_TEST
/* amiga-multiply-test: the twin against the oracle over edge mantissas and
   pseudo-random operands; GDB reads the counts at multiply_test_done. */
volatile unsigned long multiply_test_cases, multiply_test_failures, multiply_test_admitted;
volatile unsigned long multiply_test_first[4];
volatile unsigned long add_test_failures, add_test_admitted, add_test_first[4];
__attribute__((noinline)) void multiply_test_done(void) { __asm__ volatile("nop"); }

static unsigned long long state = 0x9e3779b97f4a7c15ull;
static unsigned long long next(void)
{
    state ^= state << 13; state ^= state >> 7; state ^= state << 17;
    return state;
}

static void check(unsigned long long x, unsigned long long y)
{
    const MotionBinary64 a{(unsigned long)(x >> 32), (unsigned long)x}, b{(unsigned long)(y >> 32), (unsigned long)y};
    MotionBinary64 fast{0x12345678u, 0x9abcdef0u}, portable{0x12345678u, 0x9abcdef0u};
    const bool f = downwellMultiplyBinary64Hook(a, b, fast), p = downwell::multiplyBinary64Portable(a, b, portable);
    ++multiply_test_cases;
    if (p) ++multiply_test_admitted;
    if (f != p || fast.high != portable.high || fast.low != portable.low) {
        if (!multiply_test_failures) {
            multiply_test_first[0] = a.high; multiply_test_first[1] = a.low;
            multiply_test_first[2] = b.high; multiply_test_first[3] = b.low;
        }
        ++multiply_test_failures;
    }
    MotionBinary64 sum = a, reference = a;
    const bool g = downwellAddBinary64Hook(&sum, b), q = reference.addPortable(b);
    if (q) ++add_test_admitted;
    if (g != q || sum.high != reference.high || sum.low != reference.low) {
        if (!add_test_failures) {
            add_test_first[0] = a.high; add_test_first[1] = a.low;
            add_test_first[2] = b.high; add_test_first[3] = b.low;
        }
        ++add_test_failures;
    }
}

void install_multiply_test(unsigned long randoms)
{
    static const unsigned long long mantissas[] = {
        0, 0xfffffffffffffull, 1, 0x8000000000000ull, 0x7ffffffffffffull, 0x5555555555555ull,
        0xaaaaaaaaaaaaaull, 0xffff0000ffffull, 0x0000ffff0000full, 0x800000001ull, 0xfffff00000000ull};
    static const unsigned exponents[] = {0, 1, 2, 511, 1022, 1023, 1024, 1535, 2045, 2046, 2047};
    for (unsigned long long ma : mantissas)
        for (unsigned long long mb : mantissas)
            for (unsigned ea : exponents)
                for (unsigned eb : exponents)
                    for (unsigned s = 0; s < 4; ++s)
                        check(((unsigned long long)(s & 1) << 63) | ((unsigned long long)ea << 52) | ma,
                              ((unsigned long long)(s >> 1) << 63) | ((unsigned long long)eb << 52) | mb);
    for (unsigned long i = 0; i < randoms; ++i) {
        unsigned long long x = next(), y = next();
        if ((i & 3) == 3) {
            /* Near cancellation: the negated operand with its low bits
               replaced, at an exponent up to two apart. */
            y = (x ^ 0x8000000000000000ull) ^ (y & 0xfffffull);
            if (i & 4) y += (unsigned long long)((i >> 3) % 3u) << 52;
        } else if (i & 1) {
            x = (x & 0x800fffffffffffffull) | ((unsigned long long)(983 + (unsigned long)(next() >> 32) % 80u) << 52);
            y = (y & 0x800fffffffffffffull) | ((unsigned long long)(983 + (unsigned long)(next() >> 32) % 80u) << 52);
        }
        check(x, y);
    }
    multiply_test_done();
}
#endif

}
