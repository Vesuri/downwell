#ifndef DOWNWELL_CORE_SMALL_UNSIGNED_DIVISION_H
#define DOWNWELL_CORE_SMALL_UNSIGNED_DIVISION_H
#include <cassert>
#include <cstdint>
namespace downwell {
// Exact quotient AND remainder for a 32-bit dividend and nonzero 16-bit
// divisor. Keep the portable implementation as the target twin's oracle.
struct SmallUnsignedDivision {
    struct Result {std::uint32_t quotient,remainder;};
    static Result reference(std::uint32_t value,std::uint32_t divisor){
        assert(divisor&&divisor<=0xffffu);
        return {value/divisor,value%divisor};
    }
    static Result full(std::uint32_t value,std::uint32_t divisor){
        assert(divisor&&divisor<=0xffffu);
#ifdef __m68k__
        if((value>>16)<divisor)return bounded(value,divisor);
        const auto upper=bounded(value>>16,divisor);
        const auto lower=bounded((upper.remainder<<16)|(value&0xffffu),divisor);
        return {(upper.quotient<<16)|lower.quotient,lower.remainder};
#else
        return reference(value,divisor);
#endif
    }
private:
#ifdef __m68k__
    static Result bounded(std::uint32_t value,std::uint32_t divisor){
        // DIVU's quotient must fit in 16 bits. Both callers prove this via
        // a high-word bound or the preceding division's remainder.
        assert((value>>16)<divisor);
        __asm__("divu.w %1,%0" : "+d"(value) : "d"(divisor) : "cc");
        return {std::uint16_t(value),value>>16};
    }
#endif
};
}
#endif
