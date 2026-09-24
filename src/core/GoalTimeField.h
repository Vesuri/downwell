#ifndef DOWNWELL_CORE_GOAL_TIME_FIELD_H
#define DOWNWELL_CORE_GOAL_TIME_FIELD_H
#include "GameMakerRandom.h"
#include "HudReal.h"
namespace downwell {
// goalTimeField Create/Draw, audited against the VM assembly. Call draw once
// per original Draw event, not once per dirty rectangle or buffer repair.
struct GoalTimeField {
    int x = 0, y = 0;
    HudReal radius;
    static HudReal nextRadius(GameMakerRandom& random) {
        // 128 + random(2) = 128 + uint32 / 2^31, exactly representable
        // as binary64. Fixed shifts avoid target floating-point helpers.
        return {(std::uint64_t{1} << 52) |
                (static_cast<std::uint64_t>(random.nextU32()) << 14), 7};
    }
    void create(int positionX, int positionY, GameMakerRandom& random) {
        x = positionX; y = positionY;
        radius = nextRadius(random);
    }
    template<class Circle>
    void draw(bool timeStopped, int cameraX, int roomWidth,
              GameMakerRandom& random, Circle circle) {
        // Source updates radius before checking the camera bounds.
        if (!timeStopped) radius = nextRadius(random);
        if (cameraX <= 160 || cameraX >= roomWidth - 160) return;
        circle(x, y, radius, false);
        circle(x, y, radius.subtract(HudReal::integer(1)), false);
        circle(x, y, radius.add(HudReal::integer(1)), true);
    }
};
}
#endif
