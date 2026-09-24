#ifndef DOWNWELL_CORE_GEM_VERIFICATION_H
#define DOWNWELL_CORE_GEM_VERIFICATION_H
#include "GemMotion.h"
#include "GameMakerRandom.h"
#include "StateDigest.h"
namespace downwell {
struct GemVerificationResult {
    std::uint64_t digest;
    unsigned coverage;
};
// Isolated deterministic CPU-equivalence fixture, not a Windows gameplay
// oracle. Collection is observed but not applied, to exercise long lifetimes.
inline GemVerificationResult verifyGemSimulation() {
    GameMakerRandom random(2);
    GemMotion gems[3];
    gems[0].initialize(0, 0, false, random);
    gems[1].initialize(30, 0, false, random);
    gems[2].initialize(-40, -20, true, random);
    bool alive[3] = {true, true, true};
    StateDigest digest;
    unsigned coverage = 0;
    digest.addU32(1); // Fixture schema, independent of player checkpoint.
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
                if (alive[i] && gems[i].lifetime.age > 310) coverage |= 32;
                digest.addBool(gems[i].collectible(targetX, targetY));
            }
            digest.addBool(alive[i]);
            digest.addI32(gems[i].x); digest.addI32(gems[i].y);
            digest.addI32(gems[i].vx); digest.addI32(gems[i].vy);
            digest.addBool(gems[i].attracted);
            digest.addU32(gems[i].lifetime.age);
            digest.addI32(gems[i].homing.direction);
            digest.addI32(gems[i].homing.turnFifths);
            digest.addI32(gems[i].homing.speedEighths);
            digest.addI32(gems[i].animation.rateTenths);
            digest.addU32(gems[i].animation.phaseTenths);
        }
        digest.addU64(random.drawCount());
        digest.addU32(static_cast<std::uint32_t>(random.stateIndex()));
    }
    for (unsigned i = 0; i < 16; ++i) digest.addU32(random.stateWord(i));
    digest.addU32(coverage);
    return {digest.value(), coverage};
}
} // namespace downwell
#endif
