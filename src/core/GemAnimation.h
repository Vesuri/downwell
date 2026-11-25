#ifndef DOWNWELL_CORE_GEM_ANIMATION_H
#define DOWNWELL_CORE_GEM_ANIMATION_H
#include "Binary32Timeline.h"
namespace downwell {
// Default unfrozen objCurrency animation. Sprites 133/134 have four unit keys
// and playback rate 1. Keep the runner's binary32 addition and wrap rounding:
// exact decimal tenths select the wrong frame at +0.3 on the tenth update.
// Scheduler eligibility and mutable time-stop image_speed remain separate.
class GemAnimation {
    unsigned choice_ = 3;
    std::int32_t phase27_ = 0; // the binary32 phase in exact Q27
    static std::int32_t speed27(unsigned choice) {
        // -0.5, -0.3, 0.3, 0.5 (speedBits) in Q27.
        static const std::int32_t speeds[] = {-0x4000000, -0x2666668, 0x2666668, 0x4000000};
        return speeds[choice];
    }
public:
    template<class Random>
    void initialize(Random& random) {
        choice_ = random.chooseIndexSmall(4);
        phase27_ = 0;
    }
    unsigned speedBits() const {
        static const unsigned speeds[] = {0xbf000000u, 0xbe99999au, 0x3e99999au, 0x3f000000u};
        return speeds[choice_];
    }
    // SequenceTimeline's binary32 advance over four frames, bit for bit
    // (Binary32Timeline): the phase starts at 0 and every speed is exact.
    void advance() {
        Binary32Timeline::advance(phase27_, speed27(choice_), 4 * Binary32Timeline::one);
    }
    unsigned phaseBits() const { return Binary32Timeline::toBits(phase27_); }
    unsigned frame() const { return unsigned(phase27_ >> Binary32Timeline::fraction); }
};
} // namespace downwell
#endif
