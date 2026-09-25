#include "core/ProjectileSegment.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <utility>
namespace {
using Segment = downwell::ProjectileSegment;
void require(bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
double position(Segment::Coordinate p) { return p.pixel + p.fraction / 1048576.0; }
// Independent floating-point slab oracle. Inputs are dyadic rationals so
// endpoint coordinates are represented exactly; strict interval ends model
// excluded right/bottom edges without an arbitrary epsilon.
bool oracle(const Segment& line, int left, int right, int top, int bottom) {
    if (left >= right || top >= bottom) return false;
    double first = 0, last = 1;
    bool firstOpen = false, lastOpen = false;
    auto clip = [&](double p, double d, int low, int high) {
        if (!d) return p >= low && p < high;
        double enter = (low - p) / d, leave = (high - p) / d;
        bool enterOpen = false, leaveOpen = true;
        if (d < 0) { std::swap(enter, leave); std::swap(enterOpen, leaveOpen); }
        if (enter > first) { first = enter; firstOpen = enterOpen; }
        else if (enter == first) firstOpen |= enterOpen;
        if (leave < last) { last = leave; lastOpen = leaveOpen; }
        else if (leave == last) lastOpen |= leaveOpen;
        return first < last || (first == last && !firstOpen && !lastOpen);
    };
    return clip(position(line.x), line.dx / 1048576.0, left, right) &&
           clip(position(line.y), line.dy / 1048576.0, top, bottom);
}
}
int main() {
    std::uint32_t random = 0x238491abu;
    auto next = [&]() { random = random * 1664525u + 1013904223u; return random; };
    for (unsigned i = 0; i < 100000; ++i) {
        Segment::Fraction a{next(), next(), false}, b{next(), next(), true};
        if (i % 2) b.denominator = a.denominator;
        if (i % 7 == 0) a.denominator = b.denominator = 1;
        if (i % 11 == 0) a.numerator = b.numerator;
        if (i % 17 == 0) a.denominator = b.denominator = 0;
        const std::uint64_t left = std::uint64_t(a.numerator) * b.denominator;
        const std::uint64_t right = std::uint64_t(b.numerator) * a.denominator;
        require(Segment::compare(a, b) == (left == right ? 0 : left < right ? -1 : 1),
                "fraction shortcut matches full-width cross products");
    }
    constexpr int unit = downwell::MachineGunTrajectory::unit;
    unsigned checks = 0;
    for (int x = -3; x <= 3; ++x) for (int y = -3; y <= 3; ++y)
        for (unsigned fractionX : {0u, 1u, 524288u, 1048575u})
          for (unsigned fractionY : {0u, 1u, 524288u, 1048575u})
            for (int dx : {-8*unit, -unit/3, 0, unit/3, 8*unit})
                for (int dy : {-8*unit, -unit/3, 0, unit/3, 8*unit}) {
                    const Segment segment({x, fractionX}, {y, fractionY}, dx, dy);
                    for (int left = -4; left <= 4; left += 2)
                        for (int top = -4; top <= 4; top += 2) {
                            require(segment.intersects(left, left+2, top, top+2) ==
                                    oracle(segment, left, left+2, top, top+2),
                                    "split-Q20 segment matches independent slab oracle");
                            ++checks;
                        }
                    require(!segment.intersects(100000, 100016, -100000, -99984),
                            "far rectangles reject before relative fixed-point arithmetic");
                }
    require(Segment({0,0}, {0,0}, unit, unit).intersects(1,2,1,2), "included corner endpoint intersects");
    require(!Segment({0,0}, {0,0}, unit, -unit).intersects(1,2,0,1), "excluded corner touch does not intersect");
    require(Segment({2,0}, {1,0}, -unit, 0).intersects(1,2,0,2), "starting at excluded edge can move inside");
    require(!Segment({2,0}, {1,0}, unit, 0).intersects(1,2,0,2), "starting at excluded edge and leaving misses");
    const Segment shifted({100000,524288}, {-100000,524288}, unit/3, 8*unit);
    require(shifted.intersects(100000,100001,-99999,-99998), "large signed world coordinates retain subpixel intersections");
    const downwell::ContactActor snake{20*256L,20*256L,1,1,163};
    require(Segment({20,0},{10,0},0,8*unit).intersects(snake), "shared segment uses recovered contact masks");
    std::printf("Projectile segment tests passed: %u oracle comparisons\n", checks);
}
