#ifndef DOWNWELL_CORE_AMMO_FILL_H
#define DOWNWELL_CORE_AMMO_FILL_H
#include "HudReal.h"
namespace downwell {
struct AmmoFill {
    HudReal top, height;
    // scrDrawHud4x3: sprDot at (chargeMeterx+5, min(chargeMetery+5,232)),
    // scale (10, 5-stammoMeter). Current recharge jiggle is nonnegative,
    // so the anchor is always 232. Keep both signs, including very low charge.
    static AmmoFill geometry(unsigned ammo, unsigned capacity) {
        const auto anchor = HudReal::integer(232);
        if (!ammo || !capacity) return {anchor, {}};
        // No rounding occurs in the original divide/multiply sequence here:
        // powers of two through eight divide 184 exactly. Avoid reconstructing
        // binary64 arithmetic for the usual gun's integral rectangle.
        if (capacity <= 8 && !(capacity & (capacity-1)) && ammo <= capacity) {
            const unsigned length = ammo * (184 / capacity);
            return {HudReal::integer(237-length), HudReal::integer(length-5)};
        }
        const auto meter = HudReal::ratio(ammo, capacity).times184();
        const auto five = HudReal::integer(5);
        if (meter.less(five)) return {anchor, five.subtract(meter)};
        const auto height = meter.subtract(five);
        return {anchor.subtract(height), height};
    }
};
// Integer rectangle backend policy, not a claim about Windows GPU coverage
// at fractional edges. Integral geometry matches the source exactly; retain
// the unquantized geometry above for comparison with Windows captures.
template<class Draw>
void drawAmmoFill(unsigned ammo, unsigned capacity, int x, Draw draw) {
    const auto fill = AmmoFill::geometry(ammo, capacity);
    const unsigned height = fill.height.floor();
    if (height) draw(x, fill.top.floor(), 10, height);
}
}
#endif
