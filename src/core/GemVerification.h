#ifndef DOWNWELL_CORE_GEM_VERIFICATION_H
#define DOWNWELL_CORE_GEM_VERIFICATION_H
#include "GemMotion.h"
#include "CurrencyStep.h"
#include "GameMakerRandom.h"
#include "VerificationDigest.h"
namespace downwell {
constexpr unsigned gemVerificationCoverage = 1023;
struct GemVerificationResult {
    std::uint64_t digest;
    unsigned coverage;
};
// Isolated deterministic CPU-equivalence fixture, not a Windows gameplay
// oracle. The long trajectory observes collection without applying it; a
// separate shared-phase fixture below applies collection and account expiry.
inline GemVerificationResult verifyGemSimulation() {
    GameMakerRandom random(2);
    GemMotion gems[3];
    gems[0].initialize(0, 0, false, random);
    gems[1].initialize(30, 0, false, random);
    gems[2].initialize(-40, -20, true, random);
    bool alive[3] = {true, true, true};
    VerificationDigest digest;
    unsigned coverage = 0;
    digest.addU32(7); // Includes same-phase chained alarm decrements.
    for (unsigned tick = 0; tick < 400; ++tick) {
        digest.addU32(tick);
        for (unsigned i = 0; i < 3; ++i) {
            if (alive[i]) {
                const bool wasAttracted = gems[i].attracted;
                bool terrainHit = false;
                const long targetX = i == 0 ? 1000 * 256L : i == 1 ? 32 * 256L :
                    (static_cast<long>(tick % 80) - 40) * 256L;
                const long targetY = i == 0 ? 1000 * 256L : 0;
                alive[i] = gems[i].step(targetX, targetY,
                    [&terrainHit](int, int y) { if (y >= 48) terrainHit = true; return y >= 48; }, random);
                if (!wasAttracted) coverage |= 1;
                if (wasAttracted && alive[i]) coverage |= 2;
                if (!wasAttracted && gems[i].attracted) coverage |= 4;
                if (terrainHit) coverage |= 8;
                if (!alive[i]) coverage |= 16;
                if (alive[i] && gems[i].lifetime.age > GemLifetime::expiryAge) coverage |= 32;
                digest.addBool(gems[i].collectible(targetX, targetY, i == 1));
            }
            digest.addBool(alive[i]);
            digest.addBool(gems[i].lifetime.visible());
            if (alive[i] && gems[i].lifetime.disappearing())
                coverage |= gems[i].lifetime.visible() ? 256 : 512;
            digest.addI32(gems[i].x); digest.addI32(gems[i].y);
            digest.addI32(gems[i].vx); digest.addI32(gems[i].vy);
            digest.addBool(gems[i].attracted);
            digest.addU32(gems[i].lifetime.age);
            digest.addI32(gems[i].homing.direction);
            digest.addI32(gems[i].homing.turnFifths);
            digest.addI32(gems[i].homing.speedEighths);
            digest.addI32(gems[i].animation.rateTenths);
            digest.addU32(gems[i].sound.allSet);
            digest.addI32(gems[i].sound.bounceSound);
            digest.addI32(gems[i].sound.mySound);
            digest.addU32(gems[i].animation.phaseTenths);
        }
        digest.addU64(random.drawCount());
        digest.addU32(static_cast<std::uint32_t>(random.stateIndex()));
    }
    CurrencyAccount account;
    GemMotion pickup;
    pickup.lifetime.age = 9;
    const auto solid = [](int, int) { return false; };
    const auto collected = stepCurrency(pickup, false, 2, 1000 * 256L, 0,
                                       0, 0, account, solid, random);
    digest.addU32(static_cast<unsigned>(collected));
    digest.addI32(pickup.x); digest.addI32(pickup.y);
    const bool firstCollected = collected == CurrencyStepResult::collected && account.currency == 2;
    for (unsigned tick = 0; tick < 300; ++tick) {
        account.beginStep();
        digest.addU32(account.streak); digest.addU32(account.timer);
    }
    const bool streakExpired = account.timer == 0 && account.streak == 0;
    GemMotion expired;
    expired.lifetime.age = GemLifetime::expiryAge-1;
    const auto retired = stepCurrency(expired, true, 10, 0, 0, 0, 0, account, solid, random);
    digest.addU32(static_cast<unsigned>(retired));
    const bool noPostExpiryCredit = retired == CurrencyStepResult::expired && account.currency == 2;
    GemMotion large;
    large.lifetime.age = 9;
    const auto fringe = stepCurrency(large, true, 10, 1000 * 256L, 0,
                                    9 * 256L, 0, account, solid, random);
    digest.addU32(static_cast<unsigned>(fringe));
    digest.addU32(account.currency); digest.addU32(account.gameGem);
    digest.addU32(account.streak); digest.addU32(account.timer);
    if (firstCollected && streakExpired && noPostExpiryCredit &&
        fringe == CurrencyStepResult::collected && account.currency == 12 &&
        account.gameGem == 12 && account.streak == 10 && account.timer == 300)
        coverage |= 64;
    GemMotion attractor;
    attractor.initialize(0, 0, false, random, true);
    bool delayed = true;
    for (unsigned tick = 1; tick <= 30; ++tick) {
        // Isolate the source alarm/radius edges without collection or terrain.
        attractor.x = attractor.y = attractor.vx = attractor.vy = 0;
        attractor.step(40L * 256, 0, solid, random);
        if (tick < 30 && attractor.attracted) delayed = false;
        digest.addU32(attractor.lifetime.age);
        digest.addU32(attractor.attracted);
        digest.addU32(attractor.attractorAtBirth);
        digest.addI32(attractor.homing.speedEighths);
    }
    if (delayed && attractor.attracted && attractor.homing.speedEighths == 32) coverage |= 128;
    for (unsigned i = 0; i < 16; ++i) digest.addU32(random.stateWord(i));
    digest.addU32(coverage);
    return {digest.value(), coverage};
}
} // namespace downwell
#endif
