#ifndef DOWNWELL_CORE_BONE_PROJECTILE_H
#define DOWNWELL_CORE_BONE_PROJECTILE_H
#include "BoneMotion.h"
namespace downwell {
// Object 269's post-launch event state. The owner must finish child Create
// (including random animation rate), then install the skeleton's velocities.
// It supplies collision dispatch, precise view bounds and effect/damage sinks.
// Requests never free the instance during an executing event.
struct BoneProjectile {
    enum class Animation { instanceDefault, sampledRate, stopped };
    BoneMotion motion;
    int health = 1, mask = -1;
    bool active = true, destroyRequested = false;
    Animation animation = Animation::instanceDefault;
    template<class Events> void requestDestroy(Events& events) {
        destroyRequested = true;
        events.destroy();
    }
    template<class InView, class Events>
    bool step(bool timeStopped, InView inView, Events& events) {
        if (!timeStopped) {
            if (!motion.step(false)) return false;
            animation = Animation::sampledRate;
            if (health <= 0 && active) {
                animation = Animation::stopped;
                mask = 582;
                motion.xsp = {}; motion.ysp = {};
                active = false;
                requestDestroy(events);
                events.effect(85, 3); // scrFxNol(85, 0.3), speed in tenths.
            }
        } else animation = Animation::stopped;
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
