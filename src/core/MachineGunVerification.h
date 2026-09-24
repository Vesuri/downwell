#ifndef DOWNWELL_CORE_MACHINE_GUN_VERIFICATION_H
#define DOWNWELL_CORE_MACHINE_GUN_VERIFICATION_H
#include "MachineGunProfile.h"
#include "CurrencyAccount.h"
#include "PlayerMotion.h"
#include "VerificationDigest.h"
#include "StandardBulletImpact.h"
#include "BatHitState.h"
#include "FrogHitState.h"
#include "SnailHitState.h"
namespace downwell {
constexpr unsigned machineGunVerificationCoverage = 127;
struct MachineGunVerificationResult { std::uint64_t digest; unsigned coverage; };

// Focused CPU-equivalence fixture, not a Windows recording or rendering test.
// Delayed synthetic contacts straddle expiry and exercise the real shared
// impact helper, while shared player movement determines shot timing/charge.
inline MachineGunVerificationResult verifyMachineGun() {
    CurrencyAccount currency;
    currency.streak = 100; currency.timer = 12;
    PlayerMotion player(false, 10000);
    struct Shot { unsigned born = 0; bool high = false, hit = false; } shots[8];
    unsigned count = 0, coverage = 0, hits = 0, defeated = 0;
    bool correct = true;
    struct Random { unsigned irandomSmall(unsigned) { return 0; } } random;
    VerificationDigest digest;
    digest.addU32(1);
    Input fire; fire.fire = true;
    for (unsigned tick = 0; tick < 64; ++tick) {
        currency.beginStep();
        const bool high = machineGunProfileForStreak(currency.streak).level != 0;
        if (tick == 11 && !high && currency.timer == 0 && currency.streak == 0) coverage |= 32;
        const auto before = player.shots();
        player.step(fire, false, false, false, false, high);
        if (player.shots() != before && count < 8) {
            if (count) {
                const auto& prior = shots[count - 1];
                const unsigned gap = tick - prior.born;
                correct &= gap == static_cast<unsigned>(machineGunProfile(prior.high).interval);
                if (gap == 6) coverage |= 4;
                if (gap == 7) coverage |= 8;
            }
            shots[count++] = {tick, high, false};
            coverage |= high ? 2 : 1;
        }
        for (unsigned i = 0; i < count; ++i) {
            auto& shot = shots[i];
            if (!shot.hit && tick - shot.born == 10) {
                struct Enemy {
                    bool active = true;
                    int objectId = 116, health = 40;
                    BatHitState batHit;
                    FrogHitState frogHit;
                    SnailHitState snailHit;
                } enemy;
                const auto impact = applyStandardBulletImpact(enemy, defeated, random, [] {}, shot.high);
                correct &= impact.kind == BulletImpactKind::damaged &&
                    enemy.health == (shot.high ? 25 : 31) && enemy.batHit.pending;
                if (shot.high && !high && enemy.health == 25) coverage |= 16;
                digest.addI32(enemy.health);
                shot.hit = true; ++hits;
            }
            digest.addU32(shot.born); digest.addBool(shot.high); digest.addBool(shot.hit);
            digest.addI32(machineGunProfile(shot.high).sprite);
        }
        digest.addU32(tick); digest.addBool(high);
        digest.addU32(currency.timer); digest.addU32(currency.streak);
        digest.addU32(player.shots()); digest.addI32(player.ammo());
        digest.addBool(player.shotDelayed());
    }
    if (correct && count == 8 && hits == 8 && defeated == 0) coverage |= 64;
    digest.addU32(coverage);
    return {digest.value(), coverage};
}
} // namespace downwell
#endif
