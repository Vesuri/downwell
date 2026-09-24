#ifndef DOWNWELL_CORE_STANDARD_BULLET_STEP_H
#define DOWNWELL_CORE_STANDARD_BULLET_STEP_H

#include "EnemyContact.h"

namespace downwell {
// scrBulletOov uses strict comparisons to the published view position.
// Default camMain publishes rounded pixels; callers must not substitute its
// private easing accumulator. Q12 also supports explicit synthetic fixtures.
inline bool standardBulletOutsideView(long yQ12, long viewTopQ12,
                                     int viewHeight = 284) {
    return yQ12 > viewTopQ12 + (viewHeight + 32L) * 4096L ||
           yQ12 < viewTopQ12 - 100L * 4096L;
}

// Vertical usual-gun subset. scrBulCheckSolid suppresses wall processing
// when the complete motion segment intersects an enemy. Spread, other guns
// and exact enemy collision-event scheduling remain separate work. The
// callbacks apply the current adapters' hit effects and return whether the
// ordinary non-piercing bullet was consumed. This is not a general VM Step.
struct StandardBulletPath {
    int x, fromY, toY;
    bool intersects(int left, int right, int top, int bottom) const {
        return left < right && top < bottom &&
               x >= left && x < right && fromY < bottom && toY >= top;
    }
    bool intersects(const ContactActor& actor) const {
        const ContactBounds bounds = contactWorldBounds(actor);
        return intersects(bounds.left, bounds.right, bounds.top, bounds.bottom);
    }
};

template<class Enemy, class Wall>
bool stepStandardBullet(int x, int& y, Enemy enemy, Wall wall) {
    const StandardBulletPath path{x, y, y + 8};
    y = path.toY;
    if (enemy(path)) return false;
    return !wall(path);
}
} // namespace downwell
#endif
