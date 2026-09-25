#ifndef DOWNWELL_CORE_LASER_SIGHT_RAY_H
#define DOWNWELL_CORE_LASER_SIGHT_RAY_H
#include "ProjectileSegment.h"
namespace downwell {
// playerDraw's stepped ray. Angles are unsigned Q32 turns (negatives wrap);
// xSpeedTurns is xsp interpreted as degrees, independently of bullet spread.
// As with the source's downward aim range, aim must give positive Y progress.
struct LaserSightRay {
    using Coordinate = MachineGunTrajectory::Coordinate;
    Coordinate x, y;
    // Convert the shared player's Q8 position without rounding away subpixels.
    static Coordinate fromQ8(std::int32_t value, int origin = 0) {
        std::int32_t whole = value / 256, remainder = value % 256;
        if (remainder < 0) { --whole; remainder += 256; }
        return {origin + whole, static_cast<std::uint32_t>(remainder) << 12};
    }
    // Exact truncation to Q32 turns from Q8 degrees, modulo one revolution.
    // 2^32/(360*256) = 2^21/45. Split before multiplying so the 68000 path
    // needs neither a 64-bit product nor a 64-bit division helper.
    static std::uint32_t turnsFromQ8Degrees(std::int32_t value) {
        const auto reduced = value % (360*256);
        const auto magnitude = static_cast<std::uint32_t>(reduced < 0 ? -reduced : reduced);
        const auto turns = (magnitude / 45u << 21) + ((magnitude % 45u) << 21) / 45u;
        return reduced < 0 ? 0u - turns : turns;
    }
    template<class Walls, class Enemies>
    static LaserSightRay trace(Coordinate originX, Coordinate originY,
                               Coordinate viewBottom, std::uint32_t xSpeedTurns,
                               Walls walls, Enemies enemies,
                               std::uint32_t aimTurns = 0) {
        const auto dx = MachineGunTrajectory::sineVelocity(aimTurns - xSpeedTurns);
        const auto dy = MachineGunTrajectory::sineVelocity(0x40000000u + aimTurns);
        ProjectileSegment segment(originX, originY, 0, 0);
        for (;;) {
            // All earlier segments were empty. Their union with this segment
            // is the source's cumulative line, so testing only its new portion
            // preserves the stepped endpoint for a stable collision snapshot.
            // Queries must be pure: no RNG, actor mutation or event dispatch.
            if (walls(segment) || enemies(segment)) return {segment.endX, segment.endY};
            segment = ProjectileSegment(segment.endX, segment.endY, dx, dy);
            // Source tests the strict bottom limit AFTER advancing, BEFORE
            // querying collisions. Do not clamp to bottom or exact contact.
            if (segment.endY.pixel > viewBottom.pixel ||
                (segment.endY.pixel == viewBottom.pixel &&
                 segment.endY.fraction > viewBottom.fraction))
                return {segment.endX, segment.endY};
        }
    }
};
}
#endif
