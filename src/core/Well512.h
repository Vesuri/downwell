#ifndef DOWNWELL_CORE_WELL512_H
#define DOWNWELL_CORE_WELL512_H
#include <cstdint>
#include <cstddef>
namespace downwell {
struct Well512 {
    // The caller owns stream counting and seed semantics. Index must be <16
    // and stored separately from the sixteen state words.
    static std::uint32_t reference(std::uint32_t* state,std::size_t& index){
        const auto a=state[index];auto c=state[(index+13u)&15u];
        const auto b=a^c^(a<<16u)^(c<<15u);
        c=state[(index+9u)&15u];c^=c>>11u;
        const auto mixed=b^c;state[index]=mixed;
        const auto d=mixed^((mixed<<5u)&0xda442d24u);
        index=(index+15u)&15u;
        const auto prior=state[index];
        return state[index]=prior^b^d^(prior<<2u)^(b<<18u)^(c<<28u);
    }
    static std::uint32_t step(std::uint32_t* state,std::size_t& index){
#if defined(__m68k__)
        std::uint32_t position=index,b,c,scratch,result;
        // Keep ring addressing inside the kernel. Only five data registers
        // are live; load prior after publishing mixed, not across all mixing.
        __asm__(
            "lsl.l #2,%1\n\tmove.l (%[base],%1.l),%2\n\t"
            "move.l %1,%4\n\taddi.w #52,%4\n\tandi.w #60,%4\n\tmove.l (%[base],%4.l),%3\n\t"
            "move.l %2,%0\n\tswap %0\n\tclr.w %0\n\teor.l %3,%2\n\teor.l %0,%2\n\t"
            "move.l %3,%0\n\tswap %0\n\tror.l #1,%0\n\tandi.l #0xffff8000,%0\n\teor.l %0,%2\n\t"
            "move.l %1,%4\n\taddi.w #36,%4\n\tandi.w #60,%4\n\tmove.l (%[base],%4.l),%3\n\t"
            "move.l %3,%0\n\tlsr.l #8,%0\n\tlsr.l #3,%0\n\teor.l %0,%3\n\t"
            "move.l %2,%0\n\teor.l %3,%0\n\tmove.l %0,(%[base],%1.l)\n\t"
            "move.l %0,%4\n\tlsl.l #5,%4\n\tandi.l #0xda442d24,%4\n\teor.l %0,%4\n\t"
            "subq.w #4,%1\n\tandi.w #60,%1\n\tmove.l (%[base],%1.l),%0\n\t"
            "eor.l %0,%4\n\tlsl.l #2,%0\n\teor.l %0,%4\n\teor.l %2,%4\n\t"
            "swap %2\n\tlsl.l #2,%2\n\tandi.l #0xfffc0000,%2\n\teor.l %2,%4\n\t"
            "ror.l #4,%3\n\tandi.l #0xf0000000,%3\n\teor.l %3,%4\n\t"
            "move.l %4,(%[base],%1.l)\n\tlsr.w #2,%1\n\tmove.l %4,%0"
            : "=&d"(result), "+&d"(position), "=&d"(b), "=&d"(c), "=&d"(scratch),
              "+m"(*reinterpret_cast<std::uint32_t(*)[16]>(state))
            : [base] "a"(state) : "cc");
        index=position;return result;
#else
        return reference(state,index);
#endif
    }
};
}
#endif
