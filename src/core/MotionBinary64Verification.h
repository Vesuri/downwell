#ifndef DOWNWELL_CORE_MOTION_BINARY64_VERIFICATION_H
#define DOWNWELL_CORE_MOTION_BINARY64_VERIFICATION_H
#include "MotionBinary64.h"
namespace downwell {
struct MotionBinary64Verification {
    std::uint32_t digest = 0x811c9dc5u, updates = 0, failures = 0;
    void mix(std::uint32_t word) { digest = ((digest << 5) | (digest >> 27)) ^ word; }
};
inline MotionBinary64Verification verifyMotionBinary64() {
    MotionBinary64Verification result;
    const int starts[] = {0,32,128,704,990,2048,4096,8192};
    const MotionBinary64 speeds[] = {{0xc0080000u,0}, {0xc008ccccu,0xcccccccdu}, {0xc0099999u,0x9999999au}};
    for (int start : starts) for (auto speed : speeds) {
        auto position = MotionBinary64::fromInteger(start);
        for (unsigned i = 0; i < 1000; ++i) {
            std::int32_t pixel = 0;
            if (!speed.add({0x3fb99999u,0x9999999au}) || !position.add(speed) || !position.roundPixel(pixel))
                ++result.failures;
            result.mix(speed.high); result.mix(speed.low);
            result.mix(position.high); result.mix(position.low); result.mix(static_cast<std::uint32_t>(pixel));
            ++result.updates;
        }
    }
    return result;
}
// Isolated cost sample without per-update verification hashing. Caller consumes
// all final values to prevent deletion. Includes two adds and pixel rounding.
inline MotionBinary64Verification benchmarkMotionBinary64() {
    MotionBinary64Verification result;
    auto position = MotionBinary64::fromInteger(704);
    MotionBinary64 speed{0xc0080000u,0};
    std::int32_t pixel = 0;
    for (unsigned i = 0; i < 1000; ++i) {
        if (!speed.add({0x3fb99999u,0x9999999au}) || !position.add(speed) || !position.roundPixel(pixel)) ++result.failures;
        ++result.updates;
    }
    result.mix(position.high); result.mix(position.low);
    result.mix(speed.high); result.mix(speed.low); result.mix(static_cast<std::uint32_t>(pixel));
    return result;
}
}
#endif
