#ifndef DOWNWELL_CORE_INTERACTION_PROMPT_H
#define DOWNWELL_CORE_INTERACTION_PROMPT_H
#include "HudReal.h"
namespace downwell {
// scrCheckThisSignStep uses binary64 arithmetic, not a decimal tenths counter.
struct InteractionPrompt {
    HudReal phase;
    void step(bool eligible) {
        const auto two = HudReal::integer(2);
        if (eligible) {
            if (phase.less(two)) phase = two;
            const HudReal limit{0x14666666666666ULL, 2}; // 5.1
            if (!limit.less(phase)) phase = phase.add({0x13333333333333ULL, -2}); // .3
        } else {
            phase = phase.add({0x1999999999999aULL, -4}); // .1
            if (two.less(phase)) phase = phase.subtract(two); // once, not modulo
        }
    }
    unsigned frame() const { return phase.floor(); }
};
}
#endif
