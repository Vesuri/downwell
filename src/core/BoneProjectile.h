#ifndef DOWNWELL_CORE_BONE_PROJECTILE_H
#define DOWNWELL_CORE_BONE_PROJECTILE_H
#include "BoneMotion.h"
#include "RandomBinary64.h"
#include "UnitSequenceFrames.h"
namespace downwell {
// Object 269's Create and event state. The owner must call create synchronously
// before installing the skeleton's velocity overrides.
// It supplies collision dispatch, precise view bounds and effect/damage sinks.
// Requests never free the instance during an executing event.
struct BoneProjectile {
    enum class Animation { instanceDefault, sampledRate, stopped };
    BoneMotion motion;
    MotionBinary64 sampledAnimationRate;
    RunnerImageValue timeline{}, imageIndex{}, imageSpeed{0x3f800000u};
    int health = 1, mask = -1;
    bool active = true, destroyRequested = false;
    bool hit = false, hitEffect = false;
    Animation animation = Animation::instanceDefault;
    // Sprite 460: four consecutive unit sequence keys. The owner advances
    // animation before Begin Step/alarms, using the speed from the prior Step.
    // Bone events do not write image_index or change the sprite.
    bool advanceImage(bool& crossed) {
        return UnitSequenceFrames::advance(timeline,imageSpeed,4,imageIndex,crossed);
    }
    unsigned drawFrame() const {
        // Valid sequence evaluation is nonnegative and strictly below four.
        const unsigned exponent=(imageIndex.bits>>23)&255u;
        if (exponent<127u) return 0;
        return ((imageIndex.bits&0x7fffffu)|0x800000u)>>(150u-exponent);
    }
    template<class Random>
    bool create(std::int32_t x, std::int32_t y, Random& random) {
        BoneProjectile next;
        next.motion.x = x; next.motion.y = y;
        next.motion.xx = MotionBinary64::fromInteger(x);
        next.motion.yy = MotionBinary64::fromInteger(y);
        // Inherited parent Create sets flags only. Keep all three range draws
        // even when SkeletonThrow immediately overwrites the first two values.
        if (!randomBinary64(random.nextU32(),{0x3ff80000u,0},next.motion.xsp) ||
            !next.motion.xsp.add({0x3fe00000u,0}) ||
            !randomBinary64(random.nextU32(),{0x40080000u,0},next.motion.ysp) ||
            !next.motion.ysp.add({0xc0100000u,0}) ||
            !randomBinary64(random.nextU32(),{0x3fdcccccu,0xcccccccdu},next.sampledAnimationRate) ||
            !next.sampledAnimationRate.add({0x3fa99999u,0x9999999au})) return false;
        *this = next;
        return true;
    }
    template<class Events> void requestDestroy(Events& events) {
        destroyRequested = true;
        events.destroy();
    }
    template<class InView, class Events>
    bool step(bool timeStopped, InView inView, Events& events) {
        if (!timeStopped) {
            // GML stores the sampled double into the builtin binary32 field
            // before motion; the newly selected speed affects next animation.
            if (!imageSpeed.assign(sampledAnimationRate)) return false;
            animation = Animation::sampledRate;
            if (!motion.step(false)) return false;
            if (health <= 0 && active) {
                animation = Animation::stopped;
                imageSpeed = {};
                mask = 582;
                motion.xsp = {}; motion.ysp = {};
                active = false;
                requestDestroy(events);
                events.effect(85, 3); // scrFxNol(85, 0.3), speed in tenths.
            }
        } else { animation = Animation::stopped; imageSpeed = {}; }
        // Source executes this after health destruction too. Do not early-out
        // on destroyRequested; the owner retires storage at the event boundary.
        if (!inView(motion.x, motion.y)) requestDestroy(events);
        return true;
    }
    template<class Events> void collideWall(Events& events) {
        requestDestroy(events); events.effect(85, 3);
    }
    template<class Events> void collidePlayer(bool timeStopped, Events& events) {
        if (!active) return;
        if (!timeStopped) events.damage(1, 3, 2);
        events.effect(85, 3); requestDestroy(events);
    }
};
}
#endif
