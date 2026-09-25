#ifndef DOWNWELL_CORE_LEGACY_IMAGE_ANIMATION_H
#define DOWNWELL_CORE_LEGACY_IMAGE_ANIMATION_H
#include "RunnerImageValue.h"
namespace downwell {
// Runner 0x50ef0c..0x50efbf, ordinary legacy sprite path. The owner decides
// when to advance and dispatches Animation End after the stored index changes.
// This does not handle skeletal sprites, sprite resizing or GMS2 playback rates.
inline bool advanceLegacyImage(RunnerImageValue& index, RunnerImageValue speed,
                               unsigned frames, bool& ended) {
    const auto valid = [](std::uint32_t bits) {
        const auto exponent = (bits >> 23) & 255u;
        return exponent != 255 && (exponent || !(bits & 0x7fffffu));
    };
    if (!frames || frames > 65535 || !valid(index.bits) || !valid(speed.bits)) return false;
    auto sum = index.promoted();
    RunnerImageValue candidate;
    if (!sum.add(speed.promoted()) || !candidate.assign(sum)) return false;
    RunnerImageValue count;
    if (!count.assign(MotionBinary64::fromInteger(frames))) return false;
    // The outer comparison skips wrapping for all negative indices. There
    // is no general negative modulo operation in this ordinary path.
    const bool wrap = !(candidate.bits & 0x80000000u) && candidate.bits >= count.bits;
    if (wrap) {
        auto subtract = count.promoted(); subtract.high |= 0x80000000u;
        sum = candidate.promoted();
        if (!sum.add(subtract) || !candidate.assign(sum)) return false;
    }
    index = candidate;
    ended = wrap;
    return true;
}
}
#endif
