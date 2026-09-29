#ifndef DOWNWELL_CORE_POSITIVE63_REMAINDER_H
#define DOWNWELL_CORE_POSITIVE63_REMAINDER_H
#include "SmallUnsignedRemainder.h"
namespace downwell {
// Unsigned numerator < 2^63, divisor in [1,2^63]. Limbs keep the native
// implementation independent of the compiler's 64-bit division helpers.
struct Positive63Remainder {
    struct Words { std::uint32_t high,low; };
    static bool less(Words a,Words b) {
        return a.high<b.high || (a.high==b.high && a.low<b.low);
    }
    static Words reference(Words n,Words d) {
        Words r{0,0};
        for(unsigned bit=0;bit<64;++bit) {
            r.high=(r.high<<1)|(r.low>>31);
            r.low=(r.low<<1)|(n.high>>31);
            n.high=(n.high<<1)|(n.low>>31); n.low<<=1;
            if(!less(r,d)) {
                const auto borrow=r.low<d.low;
                r.low-=d.low; r.high-=d.high+borrow;
            }
        }
        return r;
    }
    static Words full(Words n,Words d) {
        assert(n.high<0x80000000u);
        assert((d.high||d.low) && (d.high<0x80000000u ||
               (d.high==0x80000000u && !d.low)));
        if(less(n,d)) return n;
        if(!d.high) {
            if(!(d.low&(d.low-1))) return {0,n.low&(d.low-1)};
            if(d.low<=0xffffu) {
                auto r=SmallUnsignedRemainder::full(n.high,d.low);
                r=SmallUnsignedRemainder::append(r,static_cast<std::uint16_t>(n.low>>16),d.low);
                return {0,SmallUnsignedRemainder::append(r,static_cast<std::uint16_t>(n.low),d.low)};
            }
        } else if(!d.low && !(d.high&(d.high-1))) {
            return {n.high&(d.high-1),n.low};
        }
#ifdef __m68k__
        Words r{0,0};
        unsigned count=63;
        // Shift the next numerator bit through X into the remainder. Because
        // r<d<=2^63, doubling r cannot overflow the two-word accumulator.
        // CMP does not change X; SUB/SUBX propagate the low-word borrow.
        __asm__("1:\n\t"
                "add.l %[nl],%[nl]\n\taddx.l %[nh],%[nh]\n\t"
                "addx.l %[rl],%[rl]\n\taddx.l %[rh],%[rh]\n\t"
                "cmp.l %[dh],%[rh]\n\tbcs.s 3f\n\tbhi.s 2f\n\t"
                "cmp.l %[dl],%[rl]\n\tbcs.s 3f\n"
                "2:\n\tsub.l %[dl],%[rl]\n\tsubx.l %[dh],%[rh]\n"
                "3:\n\tdbra %[count],1b"
                : [nh] "+&d"(n.high), [nl] "+&d"(n.low),
                  [rh] "+&d"(r.high), [rl] "+&d"(r.low), [count] "+&d"(count)
                : [dh] "d"(d.high), [dl] "d"(d.low) : "cc");
        return r;
#else
        return reference(n,d);
#endif
    }
};
}
#endif
