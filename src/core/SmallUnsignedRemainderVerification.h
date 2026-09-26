#ifndef DOWNWELL_CORE_SMALL_UNSIGNED_REMAINDER_VERIFICATION_H
#define DOWNWELL_CORE_SMALL_UNSIGNED_REMAINDER_VERIFICATION_H
#include "SmallUnsignedRemainder.h"
namespace downwell {
// Native assembly versus the portable runtime remainder, every valid divisor.
inline unsigned verifySmallUnsignedRemainder() {
    unsigned failures = 0;
    std::uint32_t random = 0x51947a23u;
    for (std::uint32_t divisor=1; divisor<=65535; ++divisor) {
        random = random * 1664525u + 1013904223u;
        failures += SmallUnsignedRemainder::full(random,divisor) != random % divisor;
        failures += SmallUnsignedRemainder::full(0xffffffffu,divisor) != 0xffffffffu % divisor;
        const std::uint32_t value = ((divisor-1)<<16) | 0xffffu;
        failures += SmallUnsignedRemainder::append(divisor-1,0xffffu,divisor) != value % divisor;
    }
    return failures;
}
}
#endif
