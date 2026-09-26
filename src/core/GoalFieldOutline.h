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
        static constexpr int cosine[24] = {
            32768,31651,28378,23170,16384,8481,0,-8481,-16384,-23170,-28378,-31651,
            -32768,-31651,-28378,-23170,-16384,-8481,0,8481,16384,23170,28378,31651};
        visible = true;
        for (unsigned ring = 0; ring < 3; ++ring) {
            const int r = base + (ring == 1 ? -256 : ring == 2 ? 256 : 0);
            const auto component = [r](int unit) {
                const unsigned long magnitude = static_cast<unsigned short>(r) *
                    static_cast<unsigned short>(unit < 0 ? -unit : unit);
                const int rounded = static_cast<int>((magnitude + (1L << 22)) >> 23);
                return unit < 0 ? -rounded : rounded;
            };
            for (unsigned i = 0; i < 24; ++i)
                points[ring][i] = {x + component(cosine[i]),
                    y + component(cosine[(i + 18) % 24])};
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
};
}
#endif
