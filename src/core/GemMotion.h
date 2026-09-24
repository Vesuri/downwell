#ifndef DOWNWELL_CORE_GEM_MOTION_H
#define DOWNWELL_CORE_GEM_MOTION_H
#include "GemHoming.h"
#include "GemLifetime.h"
namespace downwell {
// Positions in 1/12800 pixels keep .035 gravity exact, with no 64-bit math.
// Terrain probes remain provisional, not the source sprite-mask solver.
struct GemMotion {
    long x = 0, y = 0, vx = 0, vy = 0;
    GemHoming homing;
    GemLifetime lifetime;
    bool attracted = false;
    static long q8(long value) { return value < 0 ? -((-value + 25) / 50) : (value + 25) / 50; }
    static int pixel(long value) { return static_cast<int>(value / 12800 - (value % 12800 < 0)); }
    // Existing provisional collection rectangle; source collision masks
    // remain a separate task. Both adapters use rounded instance positions.
    bool collectible(long playerX, long playerY) const {
        const long dx = roundGameMakerQ8(playerX) - roundGameMakerQ8(q8(x));
        const long dy = roundGameMakerQ8(playerY) - roundGameMakerQ8(q8(y));
        return lifetime.obtainable() && dx >= -6 && dx <= 6 && dy >= -8 && dy <= 8;
    }
    template<class Random>
    void initialize(int px, int py, bool attract, Random& random) {
        lifetime = {};
        x = px * 12800L; y = py * 12800L; attracted = attract;
        vx = static_cast<long>(random.randomIntegerSmall(51200)) - 25600;
        vy = static_cast<long>(random.randomIntegerSmall(51200)) - 25600;
        homing.initialize(random);
    }
    template<class Solid, class Random>
    bool step(long targetX, long targetY, Solid solid, Random& random) {
        if (!lifetime.advance(attracted)) return false;
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
