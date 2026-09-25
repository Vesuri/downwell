#ifndef DOWNWELL_CORE_GOAL_TIME_FIELD_H
#define DOWNWELL_CORE_GOAL_TIME_FIELD_H
#include "GameMakerRandom.h"
#include "HudReal.h"
#include "../../generated/downwell-goal-mask.h"
namespace downwell {
// goalTimeField Create/Draw, audited against the VM assembly. Advance once
// per original Draw event; snapshot painting never consumes random values.
struct GoalTimeField {
    int x = 0, y = 0;
    HudReal radius;
    // sprCircleArea's precise 128x128 mask, origin (64,64), scaled 2x by
    // Create. Draw's randomized radius does not change the collision mask.
    // Player uses sprPlayerIdle's rectangle (-4..3,-4..7).
    bool meetsPlayer(int px, int py) const {
        const int left = px - 4 - (x - 128), right = left + 8;
        int top = py - 4 - (y - 128), bottom = top + 12;
        if (right <= 0 || left >= 256 || bottom <= 0 || top >= 256) return false;
        if (top < 0) top = 0;
        if (bottom > 256) bottom = 256;
        for (int row = top / 2; row <= (bottom - 1) / 2; ++row) {
            const int a = goalMaskRows[row][0] * 2;
            const int b = goalMaskRows[row][1] * 2;
            if (a < b && left < b && right > a) return true;
        }
        return false;
    }
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
    void advanceDraw(bool timeStopped, GameMakerRandom& random) {
        // Source updates radius before checking the camera bounds.
        if (!timeStopped) radius = nextRadius(random);
    }
    template<class Circle>
    void drawSnapshot(int cameraX, int roomWidth, Circle circle) const {
        if (cameraX <= 160 || cameraX >= roomWidth - 160) return;
        circle(x, y, radius, false);
        circle(x, y, radius.subtract(HudReal::integer(1)), false);
        circle(x, y, radius.add(HudReal::integer(1)), true);
    }
};
}
#endif
