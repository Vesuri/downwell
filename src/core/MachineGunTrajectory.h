#ifndef DOWNWELL_CORE_MACHINE_GUN_TRAJECTORY_H
#define DOWNWELL_CORE_MACHINE_GUN_TRAJECTORY_H
#include "LevelClearTrig.h"
#include "MachineGunProfile.h"
#include <cstdint>
namespace downwell {
// Shared machine-gun motion used by both live collision adapters.
// Source: bulletStandard Step's one-time random_range(-3,3),
// lengthdir_(8,270+spread), then retained xsp/ysp each Step. Other gun speeds,
// aim angles and Laser Sight need explicit support rather than silent fallback.
// Q20 velocity is a bounded approximation to the Windows binary64/libm path.
struct MachineGunTrajectory {
    static_assert(machineGunProfile(false).speed == 8 && machineGunProfile(true).speed == 8 &&
                  machineGunProfile(false).accuracy == 3 && machineGunProfile(true).accuracy == 3,
                  "trajectory constants must match both source machine-gun profiles");
    static constexpr std::int32_t unit = 1 << 20;
    struct Coordinate {
        std::int32_t pixel = 0;
        std::uint32_t fraction = 0; // Always [0,unit), including negative positions.
        void advance(std::int32_t velocity) {
            const std::int32_t sum = static_cast<std::int32_t>(fraction) + velocity;
            auto whole = sum / unit;
            auto remainder = sum % unit;
            if (remainder < 0) { remainder += unit; --whole; }
            pixel += whole;
            fraction = static_cast<std::uint32_t>(remainder);
        }
    } x, y;
    std::int32_t velocityX = 0, velocityY = 0;
    std::uint32_t direction = 0; // Q32 turns, GameMaker's anticlockwise convention.
    bool initialized = false;

    void reset(std::int32_t px, std::int32_t py) {
        x = {px, 0}; y = {py, 0};
        velocityX = velocityY = 0;
        direction = 0; initialized = false;
    }
    static std::int32_t sineVelocity(std::uint32_t angle) {
        // abs(sin) Q30 * speed 8 -> Q20; round to nearest, ties upward.
        const auto magnitude = (LevelClearTrig::sineQ30(angle) + 64u) >> 7;
        return (angle & 0x80000000u) ? -static_cast<std::int32_t>(magnitude)
                                     : static_cast<std::int32_t>(magnitude);
    }
    template<class Random> void initialize(Random& random) {
        if (initialized) return;
        const std::uint32_t sample = random.nextU32();
        // (sample/2^32 * 6 - 3) degrees equals (sample-2^31)/60
        // phase units. Signed magnitude avoids implementation-defined casts.
        const std::int32_t spread = sample >= 0x80000000u
            ? static_cast<std::int32_t>((sample - 0x80000000u) / 60u)
            : -static_cast<std::int32_t>((0x80000000u - sample) / 60u);
        direction = 0xc0000000u + static_cast<std::uint32_t>(spread);
        velocityX = sineVelocity(direction + 0x40000000u);
        velocityY = -sineVelocity(direction);
        initialized = true;
    }
    // Separate initialization from movement so the caller can query the
    // upcoming segment before committing it, as scrBulCheckSolid requires.
    void advance() { x.advance(velocityX); y.advance(velocityY); }
};
}
#endif
