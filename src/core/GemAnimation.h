#ifndef DOWNWELL_CORE_GEM_ANIMATION_H
#define DOWNWELL_CORE_GEM_ANIMATION_H
#include "SequenceTimeline.h"
namespace downwell {
// Default unfrozen objCurrency animation. Sprites 133/134 have four unit keys
// and playback rate 1. Keep the runner's binary32 addition and wrap rounding:
// exact decimal tenths select the wrong frame at +0.3 on the tenth update.
// Scheduler eligibility and mutable time-stop image_speed remain separate.
class GemAnimation {
    unsigned choice_ = 3;
    RunnerImageValue phase_{};
public:
    template<class Random>
    void initialize(Random& random) {
        choice_ = random.chooseIndexSmall(4);
        phase_ = {};
    }
    unsigned speedBits() const {
        static const unsigned speeds[] = {0xbf000000u, 0xbe99999au, 0x3e99999au, 0x3f000000u};
        return speeds[choice_];
    }
    void advance() {
        bool crossed;
        // Private state is always in [0,4), speed is one of four finite normal
        // constants, and length is 4. None of the checked helper's unsupported
        // arithmetic domains is reachable through this restricted interface.
        (void)SequenceTimeline::advance(phase_, {speedBits()}, 4, crossed);
        phase_.bits &= 0x7fffffffu; // Unit-key evaluation canonicalizes -0.
    }
    unsigned phaseBits() const { return phase_.bits; }
    unsigned frame() const {
        const unsigned exponent = (phase_.bits >> 23) & 255u;
        return exponent < 127 ? 0 :
            ((phase_.bits & 0x7fffffu) | 0x800000u) >> (150 - exponent);
    }
};
} // namespace downwell
#endif
