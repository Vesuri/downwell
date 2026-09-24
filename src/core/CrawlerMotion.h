#ifndef DOWNWELL_CORE_CRAWLER_MOTION_H
#define DOWNWELL_CORE_CRAWLER_MOTION_H

#include "SolidCollision.h"

namespace downwell {

// enmCrawler Step/Alarm 0. Coordinates use fifths of Q8, preserving .2
// gravity and fractional positions through every wall-following alarm.
struct CrawlerMotion {
    long x = 0, y = 0, vx = 1280, vy = 0;
    unsigned direction = 0; // 0 right, 1 up, 2 left, 3 down
    bool latched = false;
    bool initialized = false;

    void initialize(int px, int py) {
        *this = {};
        x = px * 1280L; y = py * 1280L;
        initialized = true;
    }
    int pixelX() const { return static_cast<int>(FixedCollisionMath::round(x) / 1280); }
    int pixelY() const { return static_cast<int>(FixedCollisionMath::round(y) / 1280); }
    static bool inView(int publishedX, int publishedY,
                      long leftQ12, long topQ12, long rightQ12, long bottomQ12,
                      int bottomMargin = 0) {
        const long px = publishedX * 4096L, py = publishedY * 4096L;
        return px > leftQ12 && px < rightQ12 && py > topQ12 &&
               py < bottomQ12 + bottomMargin * 4096L;
    }
    // Mask 278 is a 10x10 AxisAlignedRect. The current area-one wall
    // adapters contain aligned 16x16 cells: four corners suffice to query
    // their union. A different terrain representation must supply meeting.
    template<class Solid>
    static bool meetsTiles(long px, long py, Solid solid) {
        const int ix = static_cast<int>(FixedCollisionMath::round(px) / 1280);
        const int iy = static_cast<int>(FixedCollisionMath::round(py) / 1280);
        return solid(ix - 5, iy - 5) || solid(ix + 4, iy - 5) ||
               solid(ix - 5, iy + 4) || solid(ix + 4, iy + 4);
    }
    template<class Meeting>
    void step(bool clockwise, bool visible, Meeting meeting) {
        if (latched || !visible) return;
        vy += 256; // global.grav = .2; crawler source has no speed cap.
        direction = clockwise ? 0 : 2;
        vx = 0;
        const auto collision = resolveSolidCollision<FixedCollisionMath>(x, y, vx, vy, meeting);
        if (collision.y == 1) { vy = 0; latched = true; }
        x += vx; y += vy;
    }
    template<class Meeting>
    void alarm(bool clockwise, bool visible, Meeting meeting) {
        if (!latched) return;
        static const int dx[] = {1, 0, -1, 0};
        static const int dy[] = {0, -1, 0, 1};
        const int side = clockwise ? 1 : -1;
        if (!meeting(x - side * dy[direction] * 1280L,
                     y + side * dx[direction] * 1280L))
            direction = (direction + (clockwise ? 3 : 1)) % 4;
        else if (meeting(x + dx[direction] * 1280L, y + dy[direction] * 1280L))
            direction = (direction + (clockwise ? 1 : 3)) % 4;
        vx = dx[direction] * 1280L; vy = dy[direction] * 1280L;
        if (!meeting(x + 1280, y + 1280) && !meeting(x - 1280, y - 1280) &&
            !meeting(x + 1280, y - 1280) && !meeting(x - 1280, y + 1280))
            latched = false;
        // Source still integrates on the alarm that releases its latch.
        if (visible) { x += vx; y += vy; }
    }
};
} // namespace downwell
#endif
