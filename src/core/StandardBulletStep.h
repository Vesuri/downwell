#ifndef DOWNWELL_CORE_STANDARD_BULLET_STEP_H
#define DOWNWELL_CORE_STANDARD_BULLET_STEP_H

#include "EnemyContact.h"
#include "ProjectileSegment.h"

namespace downwell {
// scrBulletOov uses strict comparisons to the published view position.
// Default camMain publishes rounded pixels; callers must not substitute its
// private easing accumulator. Q12 also supports explicit synthetic fixtures.
inline bool standardBulletOutsideView(long yQ12, long viewTopQ12,
                                     int viewHeight = 284) {
    return yQ12 > viewTopQ12 + (viewHeight + 32L) * 4096L ||
           yQ12 < viewTopQ12 - 100L * 4096L;
}

inline bool fractionalBulletOutsideView(MachineGunTrajectory::Coordinate y,
                                       int originY, long viewTopQ12, int height = 284) {
    y.pixel += originY;
    auto compare = [&](long boundary) {
        long pixel = boundary / 4096, fraction = boundary % 4096;
        if (fraction < 0) { --pixel; fraction += 4096; }
        if (y.pixel != pixel) return y.pixel < pixel ? -1 : 1;
        const auto fixed = static_cast<std::uint32_t>(fraction) << 8;
        return y.fraction == fixed ? 0 : y.fraction < fixed ? -1 : 1;
    };
    return compare(viewTopQ12 + (height + 32L) * 4096) > 0 ||
           compare(viewTopQ12 - 100L * 4096) < 0;
}

// The adapters retain their current enemy-first impact scheduling. This
// shares the source's first-Step initialization, full segment query and
// retained fractional movement; it is not a full VM collision-event scheduler.
template<class Random, class Enemy, class Wall>
bool stepMachineGunBullet(MachineGunTrajectory& motion, Random& random,
                         Enemy enemy, Wall wall) {
    motion.initialize(random);
    const ProjectileSegment path(motion);
    const bool alive = !enemy(path) && !wall(path);
    motion.advance();
    return alive;
}

// Complete implemented projectile update. Storage adapters supply collision
// queries and their world origin, not a second animation/motion/expiry order.
// Consumed shots still finish this update; owners retire/compact them afterward.
template<class Bullet, class Random, class Enemy, class Wall>
bool stepMachineGunProjectile(Bullet& bullet, Random& random, Enemy enemy, Wall wall,
                             int originY, long viewTopQ12, int viewHeight = 284) {
    bullet.animation.advance();
    const bool alive = stepMachineGunBullet(bullet.trajectory, random, enemy, wall);
    bullet.x = bullet.trajectory.x.pixel;
    bullet.y = bullet.trajectory.y.pixel;
    return !fractionalBulletOutsideView(bullet.trajectory.y, originY, viewTopQ12, viewHeight) && alive;
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
