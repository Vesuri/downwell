#ifndef DOWNWELL_CORE_SKELETON_DECISIONS_H
#define DOWNWELL_CORE_SKELETON_DECISIONS_H
#include "SkeletonAnimation.h"
namespace downwell {
// enmSkeleton Step decisions around motion/death, not a substitute for them.
// Owner derives activation/facing comparisons from precise global coordinates.
struct SkeletonDecisions : SkeletonAnimation {
    bool active = true, hitPending = false;
    int facing = 1;
    // Original finite real/integer CMP uses (lhs-rhs) against +/-epsilon,
    // not an exact ordered comparison. See runner-frame-comparisons.md.
    bool frameCompared(unsigned threshold, bool below) const {
        auto delta = imageIndex;
        if (!delta.add(MotionBinary64::fromInteger(-static_cast<int>(threshold)))) return false;
        if (bool(delta.high & 0x80000000u) != below) return false;
        const auto magnitude = delta.high & 0x7fffffffu;
        return magnitude > 0x3ee4f8b5u ||
            (magnitude == 0x3ee4f8b5u && delta.low > 0x88e368f1u);
    }
    bool frameBelow(unsigned threshold) const { return frameCompared(threshold, true); }
    bool frameAbove(unsigned threshold) const { return frameCompared(threshold, false); }
    // throwBone must complete child Create, sound, RNG and velocity overrides
    // synchronously (SkeletonThrow). Failed capacity is an explicit owner error.
    template<class Ops>
    bool stepDecisions(bool stopped, bool playerBelowActivation, bool playerToLeft,
                       int horizontalSpeedSign, Ops& ops) {
        beginStep(stopped);
        if (stopped) return true;
        if (playerBelowActivation) active = true;
        if (hitPending) {
            active = true; hitStun = true; alarms[1] += 5; imageIndex = {};
            ops.hurtSound(133); // Source sound observes the pre-stop desired rate.
            rate = {}; alarms[3] = 30; hitPending = false;
        }
        if (!active) {
            if (!setSprite(208)) return false;
            rate = {0x3fc99999u,0x9999999au}; // VM double 0.2
        } else {
            if (sprite == 208) { imageIndex = {}; rate = throwRate(); }
            if (!setSprite(207)) return false;
            if (frameBelow(3)) facing = playerToLeft ? -1 : 1;
            if (frameAbove(4) && !shot) {
                if (!ops.throwBone(facing)) return false;
                shot = true;
            }
            if (frameAbove(6)) finishThrow();
        }
        if (horizontalSpeedSign) facing = horizontalSpeedSign;
        return true;
    }
    // Call only after motion and death/effect processing, including time-stop.
    bool finishPresentation() {
        if (!hitStun) return true;
        imageIndex = {};
        return setSprite(205);
    }
};
}
#endif
