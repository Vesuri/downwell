#ifndef DOWNWELL_CORE_CRAWLER_MOTION_H
#define DOWNWELL_CORE_CRAWLER_MOTION_H

#include "SolidCollision.h"

namespace downwell {

// enmCrawler Step/Alarm 0. Coordinates use fifths of Q8, preserving .2
// gravity and fractional positions through every wall-following alarm.
// Math selects how a coordinate is held; FixedCollisionMath is the oracle.
template<class Math>
struct BasicCrawlerMotion {
    using Number = typename Math::Number;
    Number x{}, y{}, vx = Math::pixels(1), vy{};
    unsigned direction = 0; // 0 right, 1 up, 2 left, 3 down
    unsigned imageDirection = 0; // Quarter turns, published by Step only.
    bool latched = false;
    bool initialized = false;

    void initialize(int px, int py) {
        *this = {};
        x = Math::pixels(px); y = Math::pixels(py);
        initialized = true;
    }
    int pixelX() const { return Math::roundPixel(x); }
    int pixelY() const { return Math::roundPixel(y); }
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
    static bool meetsTiles(Number px, Number py, Solid solid) {
        const int ix = Math::roundPixel(px);
        const int iy = Math::roundPixel(py);
        return solidCorners(solid, ix - 5, iy - 5, ix + 4, iy + 4);
    }
    template<class Meeting>
    void step(bool clockwise, bool visible, Meeting meeting) {
        if (!latched && visible) {
            vy += Math::fixed(256); // global.grav = .2; crawler source has no speed cap.
            direction = clockwise ? 0 : 2;
            vx = Number{};
            const auto collision = resolveSolidCollision<Math>(x, y, vx, vy, meeting);
            if (collision.y == 1) { vy = Number{}; latched = true; }
            x += vx; y += vy;
        }
        // enmCrawler Step publishes even outside the falling branch. Alarm 0
        // changes movedir, not image_angle. Retain raw 360/450 degree values.
        imageDirection = direction + (clockwise ? 0u : 2u);
    }
    template<class Meeting>
    void alarm(bool clockwise, bool visible, Meeting meeting) {
        if (!latched) return;
        static const int dx[] = {1, 0, -1, 0};
        static const int dy[] = {0, -1, 0, 1};
        const int side = clockwise ? 1 : -1;
        if (!meeting(x - Math::pixels(side * dy[direction]),
                     y + Math::pixels(side * dx[direction])))
            direction = (direction + (clockwise ? 3 : 1)) % 4;
        else if (meeting(x + Math::pixels(dx[direction]), y + Math::pixels(dy[direction])))
            direction = (direction + (clockwise ? 1 : 3)) % 4;
        vx = Math::pixels(dx[direction]); vy = Math::pixels(dy[direction]);
        const Number one = Math::pixels(1);
        if (!meeting(x + one, y + one) && !meeting(x - one, y - one) &&
            !meeting(x + one, y - one) && !meeting(x - one, y + one))
            latched = false;
        // Source still integrates on the alarm that releases its latch.
        if (visible) { x += vx; y += vy; }
    }
};
using CrawlerMotion = BasicCrawlerMotion<FixedCollisionMath>;
} // namespace downwell
#endif
