#ifndef DOWNWELL_CORE_FROG_MOTION_H
#define DOWNWELL_CORE_FROG_MOTION_H
#include "FrogHorizontalMotion.h"
#include "FrogVerticalMotion.h"
#include "FrogHitState.h"
namespace downwell {
// One shared ordering for both adapters. The terrain callback translates
// coordinates only; it must not implement an alternative movement policy.
// Collision probes remain provisional (not the complete source mask solver).
template<class Solid>
bool stepFrogMotion(FrogHorizontalMotion& horizontal,
                    FrogVerticalMotion& vertical, FrogHitState& hitState,
                    bool integrate, Solid solid) {
    const bool hit = hitState.consume();
    if (hit) horizontal.velocity = FrogHitState::dampHorizontal(horizontal.velocity);
    vertical.step([&](int y) { return solid(horizontal.pixel(), y); }, integrate, hit);
    if (!vertical.grounded)
        horizontal.step([&](int x) { return solid(x, vertical.pixel()); }, integrate);
    return hit;
}
} // namespace downwell
#endif
