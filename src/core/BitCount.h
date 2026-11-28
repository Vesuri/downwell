#ifndef DOWNWELL_CORE_BIT_COUNT_H
#define DOWNWELL_CORE_BIT_COUNT_H
#include <cstdint>
namespace downwell {
// Leading zero bits of a 32-bit word (32 for zero). Shared by the binary32,
// Q16 and binary64 codecs, so the narrower formats need not include
// MotionBinary64 for it.
inline unsigned leadingZeros32(std::uint32_t value) {
    if (!value) return 32;
    unsigned count = 0;
    if (!(value & 0xffff0000u)) { value <<= 16; count += 16; }
    if (!(value & 0xff000000u)) { value <<= 8; count += 8; }
    if (!(value & 0xf0000000u)) { value <<= 4; count += 4; }
    if (!(value & 0xc0000000u)) { value <<= 2; count += 2; }
    if (!(value & 0x80000000u)) ++count;
    return count;
}
}
#endif
