#include "core/LaserSightRay.h"
#include <cassert>
#include <cstdio>
#include <cmath>
#include <initializer_list>
using namespace downwell;
int main() {
    using C = LaserSightRay::Coordinate;
    int wallCalls = 0, enemyCalls = 0;
    auto emptyWall = [&](const ProjectileSegment&) { ++wallCalls; return false; };
    auto emptyEnemy = [&](const ProjectileSegment&) { ++enemyCalls; return false; };
    auto ray = LaserSightRay::trace(C{10,0}, C{20,0}, C{36,0}, 0, emptyWall, emptyEnemy);
    assert(ray.x.pixel == 10 && ray.x.fraction == 0 && ray.y.pixel == 44);
    assert(wallCalls == 3 && enemyCalls == 3); // 20,28,36; no query at 44.
    wallCalls = enemyCalls = 0;
    ray = LaserSightRay::trace(C{10,0}, C{20,0}, C{36,0}, 0,
        [&](const ProjectileSegment& s) { ++wallCalls; return s.intersects(9,11,20,21); }, emptyEnemy);
    assert(ray.y.pixel == 20 && wallCalls == 1 && enemyCalls == 0);
    ray = LaserSightRay::trace(C{10,0}, C{20,0}, C{36,0}, 0, emptyWall,
        [](const ProjectileSegment& s) { return s.intersects(9,11,23,24); });
    assert(ray.y.pixel == 28); // Not exact collision y=23.
    // Horizontal preview responds to player speed, not zero-spread bullets.
    const std::uint32_t oneDegree = 0x100000000ull / 360;
    auto left = LaserSightRay::trace(C{10,0}, C{20,0}, C{36,0}, oneDegree, emptyWall, emptyEnemy);
    auto right = LaserSightRay::trace(C{10,0}, C{20,0}, C{36,0}, 0u-oneDegree, emptyWall, emptyEnemy);
    assert(left.x.pixel == 9 && right.x.pixel == 10);
    assert(left.x.fraction + right.x.fraction == MachineGunTrajectory::unit);
    // Nonzero aim changes both components, but speed changes only X. The
    // independent trig oracle bounds the existing Q20 approximation; endpoint
    // advancement and strict bottom behavior are exact in that representation.
    const auto turns = [](int degrees) {
        return static_cast<std::uint32_t>((std::int64_t(degrees) * 0x100000000LL) / 360);
    };
    for (int aim : {-30, -15, 0, 15, 30}) for (int speed : {-5, 0, 5}) {
        const auto dx = MachineGunTrajectory::sineVelocity(turns(aim)-turns(speed));
        const auto dy = MachineGunTrajectory::sineVelocity(0x40000000u+turns(aim));
        const double radians = std::acos(-1.0) / 180;
        assert(std::abs(double(dx)/MachineGunTrajectory::unit - 8*std::sin((aim-speed)*radians)) < 0.001);
        assert(std::abs(double(dy)/MachineGunTrajectory::unit - 8*std::cos(aim*radians)) < 0.001);
        for (int top = -10; top < 90; ++top) {
            C expectedX{10, 1}, expectedY{-5, 524288};
            while (!(expectedY.pixel >= top && -5 < top+1)) {
                expectedX.advance(dx); expectedY.advance(dy);
                if (expectedY.pixel > 70 || (expectedY.pixel == 70 && expectedY.fraction > 524288)) break;
            }
            ray = LaserSightRay::trace(C{10,1}, C{-5,524288}, C{70,524288}, turns(speed),
                [&](const ProjectileSegment& s) { return s.intersects(-1000,1000,top,top+1); },
                [](const ProjectileSegment&) { return false; }, turns(aim));
            assert(ray.x.pixel == expectedX.pixel && ray.x.fraction == expectedX.fraction &&
                ray.y.pixel == expectedY.pixel && ray.y.fraction == expectedY.fraction);
        }
    }
    // Exhaustive vertical wall placements, including fractional starts/bottoms:
    // compare incremental queries to an independently evaluated cumulative line.
    for (int fraction : {0, 1, 524288, 1048575}) {
        for (int top = -20; top < 100; ++top) {
            const double start = -5 + double(fraction) / MachineGunTrajectory::unit;
            double expected = start;
            while (!(expected >= top && start < top+1)) {
                expected += 8;
                if (expected > 70.5) break;
            }
            ray = LaserSightRay::trace(C{10,0}, C{-5,static_cast<unsigned>(fraction)},
                C{70,524288}, 0,
                [&](const ProjectileSegment& s) { return s.intersects(9,11,top,top+1); }, emptyEnemy);
            assert(ray.y.pixel + double(ray.y.fraction)/MachineGunTrajectory::unit == expected);
        }
    }
    std::puts("Laser Sight stepped endpoint tests passed");
}
