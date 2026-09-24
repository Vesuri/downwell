#ifndef DOWNWELL_CORE_SNAIL_MOTION_H
#define DOWNWELL_CORE_SNAIL_MOTION_H
#include "SolidCollision.h"
#include "EnemyContact.h"
namespace downwell {
struct SnailMotion {
    long x = 0, y = 0, vy = 128; // fifth-Q8: exactly 0.1 pixel
    int facing = 1, verticalFlip = 1;
    bool initialized = false, attached = false;
    void initialize(int px, int py, bool descending) {
        *this = {};
        x = px * 1280L; y = py * 1280L;
        vy = descending ? 128 : -128;
        initialized = true;
    }
    static int pixel(long p) { return static_cast<int>(p / 1280 - (p % 1280 < 0)); }
    template<class Solid>
    bool step(Solid solid, bool timeStopped = false) {
        const auto meeting = [this, &solid](long px, long py) {
            const int ix = static_cast<int>(FixedCollisionMath::round(px) / 1280);
            const int iy = static_cast<int>(FixedCollisionMath::round(py) / 1280);
            const auto b = contactWorldBounds({ix * 256L, iy * 256L, facing, verticalFlip, 177});
            // Current area-one wall cells are 16x16; this mask is smaller.
            return solid(b.left, b.top) || solid(b.right - 1, b.top) ||
                   solid(b.left, b.bottom - 1) || solid(b.right - 1, b.bottom - 1);
        };
        if (!attached) {
            if (meeting(x, y)) return false;
            if (meeting(x + 8 * 1280L, y)) facing = 1;
            else if (meeting(x - 8 * 1280L, y)) facing = -1;
            else return false;
            attached = true;
        }
        if (timeStopped) return true;
        const int sign = (vy > 0) - (vy < 0);
        const auto collision = resolveSolidCollision<FixedCollisionMath>(x, y, 0, vy, meeting);
        if (collision.y != 0 || !solid(pixel(x) + facing * 16, pixel(y + sign * 8 * 1280L)))
            vy = -vy;
        y += vy;
        if (vy != 0) verticalFlip = vy < 0 ? 1 : -1;
        return true;
    }
};
} // namespace downwell
#endif
