#ifndef DOWNWELL_CORE_RANGED_SHOT_H
#define DOWNWELL_CORE_RANGED_SHOT_H
#include "GunTable.h"
#include "LevelClearTrig.h"
#include "MachineGunTrajectory.h"
#include "NarrowArithmetic.h"
#include "RandomRangeBinary32.h"
#include "UnitSequenceFrames.h"
#include <cstdint>
namespace downwell {
// Compact bulletRanged (261), the object of six of bStatInitLevel1's seven
// guns, including the starting machine gun. Create draws imgSp =
// random_range(0.4,0.6) then bSpeed += random_range(-rangeRandom,0); Alarm 0
// (decTimer = rangeTimer, Laser Sight +floor(/3)) starts decelerate. Step:
// first bDir += random_range(-bdirRand,bdirRand); decelerate scales bSpeed by
// 0.8; below 3 the animation starts at imgSp; lengthdir from bSpeed/bDir;
// scrBulCheckSolid; x += xsp, y += ysp. Animation End destroys it.
// Positions are split Q20 as for MachineGunTrajectory (so ProjectileSegment
// applies); speed is Q20 and the angle Q32 turns. This is the compact
// world's arithmetic, not the binary64/libm VM path.
struct RangedShot {
    using Coordinate = MachineGunTrajectory::Coordinate;
    static constexpr std::int32_t unit = MachineGunTrajectory::unit;
    Coordinate x, y;
    std::int32_t velocityX = 0, velocityY = 0;
    std::uint32_t speed = 0;      // bSpeed, Q20
    std::uint32_t direction = 0;  // bDir, Q32 turns
    std::uint32_t cosine = 0, sine = 0; // |cos|,|sin| Q30 of the fixed bDir
    RunnerImageValue imageSpeed{}, imageIndex{};
    int decelerateAlarm = -1, accuracyHalves = 0;
    bool allSet = false, decelerate = false, animation = false, zeroSpread = false;

    // 68000 note: only 32-bit products and quotients (no __muldi3/__divdi3).
    // floor(degrees * 2^32 / 360), as 2^32 = 360 * 11930464 + 256.
    // Callers pass aim angles (270 +- aimLimit +- spread) or [0, 360).
    static std::uint32_t degreesToTurns(int degrees) {
        degrees = fitsS16(degrees) ? std::int16_t(degrees) % 360 : degrees % 360;
        if (degrees < 0) degrees += 360;
        const auto d = static_cast<std::uint16_t>(degrees);
        // 11930464 = 182 * 2^16 + 2912; 256/360 = 32/45 and 32 * 359 < 2^16.
        return (mulu16(d, 182) << 16) + mulu16(d, 2912) + std::uint16_t(d * 32u) / 45u;
    }
    // |trig| (Q30, taken at Q15) times bSpeed (Q20, below 2^25), rounded.
    static std::int32_t component(std::uint32_t magnitudeQ30, std::uint32_t speedQ20, bool negative) {
        const std::uint32_t m = magnitudeQ30 >> 15, high = speedQ20 >> 16, low = speedQ20 & 0xffffu;
        const auto value = static_cast<std::int32_t>(m * high * 2u + ((m * low + (1u << 14)) >> 15));
        return negative ? -value : value;
    }
    // bulletRanged Create, after bulPropertyCheck. bDir is the caller's
    // assignment after instance_create; Laser Sight clears bdirRand only on
    // the caller's last bullet (scrPlayerShootN's myBullet).
    template<class Random>
    bool create(Random& random, int px, int py, int degrees, const GunStats& gun, bool laserSight) {
        *this = {};
        x = {px, 0}; y = {py, 0};
        direction = degreesToTurns(degrees);
        accuracyHalves = gun.accuracyHalves;
        // image_speed = random_range(0.4, 0.6): width 0.6 - 0.4 is the
        // binary64 0x3fc9999999999998, low 0x3fd999999999999a.
        imageSpeed = RandomRangeBinary32::value(random.nextU32(), 0x19999999999998u, 0x1999999999999au);
        const std::uint32_t word = random.nextU32();
        const std::uint32_t range = static_cast<std::uint32_t>(gun.rangeRandom);
        // floor(word * range / 2^12): bSpeed - rangeRandom + u * rangeRandom.
        speed = (static_cast<std::uint32_t>(gun.speed - gun.rangeRandom) << 20) +
            (mulu16(word >> 16, range) << 4) + mulu16((word >> 12) & 15u, range) + // rangeRandom <= 10
            (mulu16(word & 4095u, range) >> 12);
        int timer = gun.rangeTimer;
        if (laserSight) timer += std::int16_t(timer) / 3; // rangeTimer <= 20
        decelerateAlarm = timer;
        return true;
    }
    // bulletDrone 266 Create (objDrone's shot and Gunpowder Blocks' box
    // shot): bSpeed 7, bdirRand 0, image_speed 0 until it slows below 3,
    // then 0.5; decTimer 16 with declSp 0.8. No Create RNG; its first Step
    // still draws random_range(-0, 0).
    void createDrone(int px, int py, int degrees) {
        *this = {};
        x = {px, 0}; y = {py, 0};
        direction = degreesToTurns(degrees);
        imageSpeed = {0x3f000000u};
        speed = 7u << 20;
        decelerateAlarm = 16;
    }
    // Alarm 0 (from the alarm dispatcher, index 0).
    void alarm() {
        if (decelerateAlarm > 0 && --decelerateAlarm == 0) { decelerate = true; decelerateAlarm = -1; }
    }
    // Step up to, not including, scrBulCheckSolid: the caller queries the
    // returned segment, then calls advance() (also after a hit).
    template<class Random> void prepareStep(Random& random) {
        if (!allSet) {
            const std::uint32_t word = random.nextU32();
            const std::int32_t halves = zeroSpread ? 0 : accuracyHalves;
            // random_range(-bdirRand, bdirRand) degrees in Q16 (u * 2r - r
            // with r = halves / 2), then Q16 degrees to Q32 turns:
            // 2^16 / 360 = 182 + 2 / 45.
            const std::int32_t degreesQ16 = static_cast<std::int32_t>(word >> 16) * halves - (halves << 15);
            direction += static_cast<std::uint32_t>(degreesQ16 * 182 + (degreesQ16 * 2) / 45);
            cosine = LevelClearTrig::sineQ30(direction + 0x40000000u);
            sine = LevelClearTrig::sineQ30(direction);
            allSet = true;
        }
        if (decelerate) speed = (speed * 4u) / 5u; // bSpeed stays below 2^25
        if (speed < (3u << 20) && !animation) animation = true;
        // lengthdir_x = cos * speed; lengthdir_y = -sin * speed.
        velocityX = component(cosine, speed, ((direction + 0x40000000u) & 0x80000000u) != 0);
        velocityY = component(sine, speed, (direction & 0x80000000u) == 0);
    }
    void advance() { x.advance(velocityX); y.advance(velocityY); }
    // Runner animation phase, before Begin Step. Returns false on invalid
    // state; ended reports the Animation End event (action_kill_object).
    bool advanceAnimation(unsigned frames, bool& ended) {
        ended = false;
        if (!animation) return true;
        RunnerImageValue image; bool crossed = false;
        if (!UnitSequenceFrames::advance(imageIndex, imageSpeed, frames, image, crossed)) return false;
        ended = crossed;
        return true;
    }
    unsigned frame() const {
        const unsigned exponent = (imageIndex.bits >> 23) & 255u;
        return exponent < 127 ? 0 : ((imageIndex.bits & 0x7fffffu) | 0x800000u) >> (150u - exponent);
    }
};
}
#endif
