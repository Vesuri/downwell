#ifndef DOWNWELL_CORE_GOAL_TIME_FIELD_H
#define DOWNWELL_CORE_GOAL_TIME_FIELD_H
#include "GameMakerRandom.h"
#include "HudReal.h"
#include "EnemyContact.h"
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
        return meetsRelativeBounds({px-x-4,px-x+4,py-y-4,py-y+8});
    }
    // Half-open integer pixel bounds relative to the field's centre. The
    // caller resolves the source actor's transform/bbox before invoking this.
    // Clip before translating so even extreme rectangle bounds are safe.
    static bool meetsRelativeBounds(ContactBounds bounds) {
        if(bounds.left>=bounds.right || bounds.top>=bounds.bottom ||
           bounds.right<=-128 || bounds.left>=128 ||
           bounds.bottom<=-128 || bounds.top>=128)return false;
        const int left=(bounds.left < -128 ? -128 : bounds.left)+128;
        const int right=(bounds.right > 128 ? 128 : bounds.right)+128;
        const int top=(bounds.top < -128 ? -128 : bounds.top)+128;
        const int bottom=(bounds.bottom > 128 ? 128 : bounds.bottom)+128;
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
