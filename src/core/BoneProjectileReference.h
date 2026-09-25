#ifndef DOWNWELL_CORE_BONE_PROJECTILE_REFERENCE_H
#define DOWNWELL_CORE_BONE_PROJECTILE_REFERENCE_H
#include "GameMakerRounding.h"

namespace downwell {
// Host-side numeric reference for object 269, NOT the deployed 68000 motion
// representation. Preserve VM doubles and operation order before selecting a
// fixed-point implementation. Compile with FP contraction disabled: VM
// multiply/add instructions must not become a host fused multiply-add.
// The event owner supplies collision dispatch,
// deferred destruction, effects and animation advancement.
struct BoneProjectileReference {
    double xx = 0, yy = 0, xsp = 0, ysp = 0, imgSp = 0, imageSpeed = 0;
    long x = 0, y = 0;
    int health = 1, mask = -1;
    bool active = true, hit = false, hitEffect = false;
    template<class Random>
    void create(long px, long py, Random& random) {
        *this = BoneProjectileReference{}; // parentShootableThing Create has no RNG.
        x = px; y = py; xx = px; yy = py;
        xsp = 0.5 + random.random(1.5);
        ysp = -4 + random.random(3);
        imgSp = 0.05 + random.random(0.45);
        // Create sets imgSp but does not assign image_speed.
        imageSpeed = 1; // GameMaker instance default.
    }
    template<class Events>
    void step(bool stopped, double viewX, double viewY, double width,
              double height, Events& events) {
        if (!stopped) {
            imageSpeed = imgSp;
            ysp += 0.1;
            xx += xsp; yy += ysp;
            x = roundGameMakerCoordinate(xx); y = roundGameMakerCoordinate(yy);
            if (health <= 0 && active) {
                imageSpeed = 0; mask = 582; xsp = ysp = 0; active = false;
                events.destroy();
                events.effect(85, 0.3);
            }
        } else imageSpeed = 0;
        // scrInView uses strict anchor bounds, not sprite overlap; runs even
        // during time-stop and after an earlier instance_destroy in this event.
        if (!(x > viewX && x < viewX + width && y > viewY && y < viewY + height))
            events.destroy();
    }
    template<class Events> void collideWall(Events& events) {
        events.destroy(); events.effect(85, 0.3);
    }
    template<class Events> void collidePlayer(bool stopped, Events& events) {
        if (!active) return;
        if (!stopped) events.damage(1, 3, 2);
        events.effect(85, 0.3); events.destroy();
    }
};
} // namespace downwell
#endif
