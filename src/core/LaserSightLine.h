#ifndef DOWNWELL_CORE_LASER_SIGHT_LINE_H
#define DOWNWELL_CORE_LASER_SIGHT_LINE_H
#include "LaserSightRay.h"
namespace downwell {
// Shared integer presentation rule. Windows GPU subpixel/endpoint coverage
// still needs a capture oracle; this is not a claim of exact D3D rasterization.
struct LaserSightLine {
    int x0, y0, x1, y1;
    static int pixel(LaserSightRay::Coordinate c) {
        constexpr unsigned half = MachineGunTrajectory::unit / 2;
        return c.pixel + (c.fraction > half || (c.fraction == half && c.pixel % 2 != 0));
    }
    static LaserSightLine from(LaserSightRay::Coordinate x, LaserSightRay::Coordinate y,
                               LaserSightRay end, int viewX, int viewY) {
        return {pixel(x)-viewX, pixel(y)+8-viewY, pixel(end.x)-viewX, pixel(end.y)-viewY};
    }
    // Endpoints and differences must fit signed int, including doubled error.
    // Gameplay segments are bounded by the viewport plus the player's margin.
    template<class Plot> void draw(Plot plot) const {
        int x = x0, y = y0;
        const int dx = x1 >= x ? x1-x : x-x1, sx = x < x1 ? 1 : -1;
        const int dy = y1 >= y ? y-y1 : y1-y, sy = y < y1 ? 1 : -1;
        int error = dx+dy;
        for (;;) {
            plot(x, y);
            if (x == x1 && y == y1) break;
            const int twice = 2*error;
            if (twice >= dy) { error += dy; x += sx; }
            if (twice <= dx) { error += dx; y += sy; }
        }
    }
};
}
#endif
