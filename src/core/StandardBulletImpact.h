#ifndef DOWNWELL_CORE_STANDARD_BULLET_IMPACT_H
#define DOWNWELL_CORE_STANDARD_BULLET_IMPACT_H
#include "MachineGunProfile.h"
#include <cstdint>
namespace downwell {
struct NoBulletRetirement {
    template<class Enemy> bool operator()(const Enemy&) const { return true; }
};
enum class BulletImpactKind { ignored, immune, damaged, defeated, failed };
struct BulletImpact { BulletImpactKind kind; int sound; };

// Implemented machine gun: non-piercing, nine/fifteen damage, retained direction.
// bStatInitialize's temporary 10 is overwritten by bStatUpdate(0, 0), which
// reads bStatInitLevel1's MACHINEGUN entry. Level is captured at bullet birth.
// Enemy storage exposes the same hit-state fields on both platforms. Queued
// hit delivery and immediate fatal retirement remain phase approximations;
// original hitAngle/hitDmg, damageStun, momentDelay and effects are not modeled.
// bdmg is bulPropertyCheck's birth copy of global.pBulDamage; the compact
// world's seven guns supply it explicitly.
template<class Enemy, class Count, class Random, class Reward, class Retire = NoBulletRetirement>
BulletImpact applyBulletDamage(Enemy& enemy, Count& defeated, Random& random, Reward reward,
                               int damage, std::uint32_t directionTurns = 0xc0000000u,
                               Retire retire = {}) {
    if (!enemy.active || enemy.health <= 0) return {BulletImpactKind::ignored, -1};
    if (enemy.objectId == 112 || enemy.objectId == 114) {
        // scrTypicalBulletDamage's immune branch: choose(30,31) before
        // soundPlay. Sound playback itself does not draw random values.
        return {BulletImpactKind::immune, 30 + static_cast<int>(random.chooseIndexSmall(2))};
    }
    if (enemy.objectId == 113) enemy.frogHit.pending = true;
    if (enemy.objectId == 117) enemy.snailHit.pending = true;
    if (enemy.objectId == 116) enemy.batHit.queueTurns(directionTurns);
    enemy.health -= damage;
    if (enemy.health > 0) return {BulletImpactKind::damaged, 29};
    if(!reward())return {BulletImpactKind::failed,-1};
    // Retain damage/reward writes if owner removal fails, but do not publish
    // retirement, defeat accounting or sound. Caller must latch failure.
    if(!retire(enemy))return {BulletImpactKind::failed,-1};
    enemy.active = false;
    ++defeated;
    return {BulletImpactKind::defeated, 29};
}
template<class Enemy, class Count, class Random, class Reward, class Retire = NoBulletRetirement>
BulletImpact applyStandardBulletImpact(Enemy& enemy, Count& defeated,
                                      Random& random, Reward reward, bool gemHigh = false,
                                      std::uint32_t directionTurns = 0xc0000000u,
                                      Retire retire = {}) {
    return applyBulletDamage(enemy, defeated, random, reward, machineGunProfile(gemHigh).damage,
                             directionTurns, retire);
}
} // namespace downwell
#endif
