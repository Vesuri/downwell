#ifndef DOWNWELL_CORE_GOAL_FIELD_OUTLINE_H
#define DOWNWELL_CORE_GOAL_FIELD_OUTLINE_H
#include "GoalTimeField.h"
#include "LaserSightLine.h"
namespace downwell {
// GameMaker's default circle precision is 24; the source never overrides it.
// Shared integer raster adaptation, not a Windows D3D pixel-coverage oracle.
// Q8 radii and Q15 unit vectors avoid floating point and wide multiplies.
struct GoalFieldOutline {
    struct Point { int x, y; } points[3][24] = {};
    bool visible = false;
    void set(const GoalTimeField& field, bool present,
             int viewX, int viewY, int screenX = 0) {
        visible = false;
        const int cameraX = viewX + 80;
        if (!present || cameraX <= 160 || cameraX >= 320 ||
            field.y + 132 < viewY || field.y - 132 >= viewY + 284) return;
        auto radius = field.radius;
        radius.exponent += 8;
        const int base = static_cast<int>(radius.floor());
        if (base < 128 * 256 || base >= 130 * 256) return;
        setRasterCircle(field.x - viewX + screenX, field.y - viewY, base);
    }
    // Presentation-only Q8 radius. Collision continues to use resource masks.
    void setRasterCircle(int x, int y, int base) {
        static constexpr unsigned short cosine[6] = {32768,31651,28378,23170,16384,8481};
        visible = true;
        for (unsigned ring = 0; ring < 3; ++ring) {
            const int r = base + (ring == 1 ? -256 : ring == 2 ? 256 : 0);
            // Six unique magnitudes; the other quadrants reuse their exact
            // rounded values. Keep unsigned 16x16 multiplication on 68000.
            int c[7]; c[6] = 0;
            for (unsigned i = 0; i < 6; ++i) {
                const unsigned long magnitude = static_cast<unsigned short>(r) * cosine[i];
                c[i] = static_cast<int>((magnitude + (1L << 22)) >> 23);
            }
            for (unsigned i = 0; i < 6; ++i) {
                points[ring][i] = {x+c[i], y+c[6-i]};
                points[ring][i+6] = {x-c[6-i], y+c[i]};
                points[ring][i+12] = {x-c[i], y-c[6-i]};
                points[ring][i+18] = {x+c[6-i], y-c[i]};
            }
        }
    }
    bool same(const GoalFieldOutline& other) const {
        if (visible != other.visible) return false;
        if (!visible) return true;
        for (unsigned ring = 0; ring < 3; ++ring)
            for (unsigned i = 0; i < 24; ++i)
                if (points[ring][i].x != other.points[ring][i].x ||
                    points[ring][i].y != other.points[ring][i].y) return false;
        return true;
    }
    template<class Plot> void draw(Plot plot) const {
        if (!visible) return;
        for (unsigned ring = 0; ring < 3; ++ring)
            for (unsigned i = 0; i < 24; ++i) {
                const auto a = points[ring][i], b = points[ring][(i + 1) % 24];
                LaserSightLine{a.x,a.y,b.x,b.y}.draw([&](int x, int y) { plot(x, y, ring == 2 ? 2 : 1); });
            }
    }
    // Reject whole off-screen edges without changing endpoint/error state on
    // intersecting edges. Geometric endpoint clipping would alter Bresenham
    // coverage and therefore cannot replace this conservative rejection.
    template<class Plot> void drawClipped(int left,int top,int right,int bottom,Plot plot) const {
        if (!visible || left >= right || top >= bottom) return;
        for (unsigned ring = 0; ring < 3; ++ring)
            for (unsigned i = 0; i < 24; ++i) {
                const auto a = points[ring][i], b = points[ring][(i+1)%24];
                if ((a.x < left && b.x < left) || (a.x >= right && b.x >= right) ||
                    (a.y < top && b.y < top) || (a.y >= bottom && b.y >= bottom)) continue;
                LaserSightLine{a.x,a.y,b.x,b.y}.draw([&](int x,int y) {
                    if (x >= left && x < right && y >= top && y < bottom)
                        plot(x,y,ring == 2 ? 2 : 1);
                });
            }
    }
};
}
#endif
