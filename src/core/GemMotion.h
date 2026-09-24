#ifndef DOWNWELL_CORE_GEM_MOTION_H
#define DOWNWELL_CORE_GEM_MOTION_H
#include "GemHoming.h"
#include "GemLifetime.h"
#include "GemAnimation.h"
#include "EnemyContact.h"
namespace downwell {
// Positions in 1/12800 pixels keep .035 gravity exact, with no 64-bit math.
// Terrain probes remain provisional, not the source sprite-mask solver.
struct GemMotion {
    long x = 0, y = 0, vx = 0, vy = 0;
    GemHoming homing;
    GemLifetime lifetime;
    GemAnimation animation;
    bool attracted = false;
    static long q8(long value) { return value < 0 ? -((-value + 25) / 50) : (value + 25) / 50; }
    static int pixel(long value) { return static_cast<int>(value / 12800 - (value % 12800 < 0)); }
    static long rounded(long value) {
        const long whole = value / 12800, fraction = value % 12800;
        if (fraction > 6400 || (fraction == 6400 && whole % 2 != 0)) return whole + 1;
        if (fraction < -6400 || (fraction == -6400 && whole % 2 != 0)) return whole - 1;
        return whole;
    }
    // Serialized AxisAlignedRect masks, with rounded instance positions.
    // Round directly from authoritative units, not through a Q8 midpoint.
    bool collectible(long playerX, long playerY, bool large = false) const {
        const ContactBounds player = contactWorldBounds({playerX, playerY, 1, 1, 31});
        const ContactBounds gem = contactWorldBounds({rounded(x) * 256, rounded(y) * 256,
                                                     1, 1, large ? 134 : 133});
        return lifetime.obtainable() && player.left < gem.right && gem.left < player.right &&
            player.top < gem.bottom && gem.top < player.bottom;
    }
    template<class Random>
    void initialize(int px, int py, bool attract, Random& random) {
        lifetime = {};
        x = px * 12800L; y = py * 12800L; attracted = attract;
        vx = static_cast<long>(random.randomIntegerSmall(51200)) - 25600;
        vy = static_cast<long>(random.randomIntegerSmall(51200)) - 25600;
        animation.initialize(random);
        homing.initialize(random);
    }
    template<class Solid, class Random>
    bool step(long targetX, long targetY, Solid solid, Random& random) {
        if (!lifetime.advance(attracted)) return false;
        animation.advance();
        const long dx = targetX - q8(x), dy = targetY - q8(y);
        if (attracted) {
            long sx, sy;
            homing.step(dx, dy, sx, sy);
            vx = sx * 50; vy = sy * 50;
            x += vx; y += vy;
            return true;
        }
        if (vy < 4 * 12800L) vy += 448;
        else vy = 4 * 12800L;
        if (solid(pixel(x + vx), pixel(y))) vx = -vx * 7 / 10;
        else x += vx;
        if (vy > 0 && solid(pixel(x), pixel(y + vy) + 4)) {
            // Existing bounce approximation; source irandom_range with
            // fractional negative endpoints still needs runner verification.
            vy = -6400 - static_cast<long>(random.randomIntegerSmall(8960));
            vx = vx * 7 / 10;
        } else y += vy;
        if (vx > -640 && vx < 640) vx = vx < 0 ? -640 : vx > 0 ? 640 : 0;
        if (lifetime.suckable() && dx > -24L * 256 && dx < 24L * 256 &&
            dy > -24L * 256 && dy < 24L * 256) {
            attracted = true;
            homing.acquire(dx, dy);
        }
        return true;
    }
};
} // namespace downwell
#endif
