#ifndef DOWNWELL_CORE_STANDARD_BULLET_IMPACT_H
#define DOWNWELL_CORE_STANDARD_BULLET_IMPACT_H
namespace downwell {
enum class BulletImpactKind { ignored, immune, damaged, defeated };
struct BulletImpact { BulletImpactKind kind; int sound; };

// Implemented usual gun: non-piercing, ten damage, downward direction 270.
// Enemy storage exposes the same hit-state fields on both platforms. Queued
// hit delivery and immediate fatal retirement remain phase approximations;
// original hitAngle/hitDmg, damageStun, momentDelay and effects are not modeled.
template<class Enemy, class Count, class Random, class Reward>
BulletImpact applyStandardBulletImpact(Enemy& enemy, Count& defeated,
                                      Random& random, Reward reward) {
    if (!enemy.active || enemy.health <= 0) return {BulletImpactKind::ignored, -1};
    if (enemy.objectId == 112) {
        // scrTypicalBulletDamage's immune branch: choose(30,31) before
        // soundPlay. Sound playback itself does not draw random values.
        return {BulletImpactKind::immune, 30 + static_cast<int>(random.irandomSmall(1))};
    }
    if (enemy.objectId == 113) enemy.frogHit.pending = true;
    if (enemy.objectId == 117) enemy.snailHit.pending = true;
    if (enemy.objectId == 116) enemy.batHit.queue(270);
    enemy.health -= 10;
    if (enemy.health > 0) return {BulletImpactKind::damaged, 29};
    reward();
    enemy.active = false;
    ++defeated;
    return {BulletImpactKind::defeated, 29};
}
} // namespace downwell
#endif
