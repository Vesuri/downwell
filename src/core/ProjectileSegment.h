#ifndef DOWNWELL_CORE_PROJECTILE_SEGMENT_H
#define DOWNWELL_CORE_PROJECTILE_SEGMENT_H
#include "MachineGunTrajectory.h"
#include "EnemyContact.h"
namespace downwell {
// Continuous segment versus half-open world rectangles, using the existing
// contact-mask convention [left,right) x [top,bottom). No pixel stepping,
// float, division, or 64-bit multiply is needed by the intersection query.
// The trajectory supplies normalized split-Q20 endpoints; relative edge
// distances are formed only after proving the edge lies inside this step.
// Preconditions: normalized fractions, current machine-gun step components
// bounded by eight pixels, and world pixels with room for one such step.
struct ProjectileSegment {
    using Coordinate = MachineGunTrajectory::Coordinate;
    Coordinate x, y, endX, endY;
    std::int32_t dx, dy;
    ProjectileSegment(Coordinate px, Coordinate py, std::int32_t vx, std::int32_t vy)
        : x(px), y(py), endX(px), endY(py), dx(vx), dy(vy) {
        endX.advance(dx); endY.advance(dy);
    }
    explicit ProjectileSegment(const MachineGunTrajectory& motion)
        : ProjectileSegment(motion.x, motion.y, motion.velocityX, motion.velocityY) {}

    struct Fraction { std::uint32_t numerator, denominator; bool strict; };
    static int compare(Fraction a, Fraction b) {
        const auto highA = LevelClearTrig::productHigh(a.numerator, b.denominator);
        const auto highB = LevelClearTrig::productHigh(b.numerator, a.denominator);
        if (highA != highB) return highA < highB ? -1 : 1;
        const std::uint32_t lowA = a.numerator * b.denominator;
        const std::uint32_t lowB = b.numerator * a.denominator;
        return lowA == lowB ? 0 : lowA < lowB ? -1 : 1;
    }
    struct Interval {
        Fraction first{0, 1, false}, last{1, 1, false};
        void enter(Fraction edge) {
            const int relation = compare(edge, first);
            if (relation > 0) first = edge;
            else if (relation == 0) first.strict |= edge.strict;
        }
        void leave(Fraction edge) {
            const int relation = compare(edge, last);
            if (relation < 0) last = edge;
            else if (relation == 0) last.strict |= edge.strict;
        }
        bool valid() const {
            const int relation = compare(first, last);
            return relation < 0 || (relation == 0 && !first.strict && !last.strict);
        }
    };
    static bool clipAxis(Coordinate start, Coordinate end, std::int32_t velocity,
                         int low, int high, Interval& interval) {
        if (low >= high) return false;
        if (!velocity) return start.pixel >= low && start.pixel < high;
        constexpr auto unit = MachineGunTrajectory::unit;
        if (velocity > 0) {
            if (end.pixel < low || start.pixel >= high) return false;
            const auto speed = static_cast<std::uint32_t>(velocity);
            if (start.pixel < low)
                interval.enter({static_cast<std::uint32_t>((low - start.pixel) * unit) - start.fraction, speed, false});
            if (end.pixel >= high)
                interval.leave({static_cast<std::uint32_t>((high - start.pixel) * unit) - start.fraction, speed, true});
        } else {
            if (start.pixel < low || end.pixel >= high) return false;
            const auto speed = static_cast<std::uint32_t>(-velocity);
            if (start.pixel >= high)
                interval.enter({static_cast<std::uint32_t>((start.pixel - high) * unit) + start.fraction, speed, true});
            if (end.pixel < low)
                interval.leave({static_cast<std::uint32_t>((start.pixel - low) * unit) + start.fraction, speed, false});
        }
        return interval.valid();
    }
    bool intersects(int left, int right, int top, int bottom) const {
        Interval interval;
        return clipAxis(x, endX, dx, left, right, interval) &&
               clipAxis(y, endY, dy, top, bottom, interval);
    }
    bool intersects(const ContactActor& actor) const {
        const auto bounds = contactWorldBounds(actor);
        return intersects(bounds.left, bounds.right, bounds.top, bounds.bottom);
    }
};
}
#endif
