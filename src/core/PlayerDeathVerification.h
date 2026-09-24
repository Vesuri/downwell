#ifndef DOWNWELL_CORE_PLAYER_DEATH_VERIFICATION_H
#define DOWNWELL_CORE_PLAYER_DEATH_VERIFICATION_H
#include "PlayerDeathSequence.h"
#include "PlayerMotion.h"
#include "StateDigest.h"
#include "RunClock.h"
#include "RunTimeParts.h"

namespace downwell {
struct PlayerDeathVerificationResult {
    std::uint64_t digest;
    unsigned coverage;
};
constexpr unsigned playerDeathVerificationCoverage = 511;
// GCC/LTO otherwise expands each volatile FNV byte temporary into this
// fixture's stack frame. Keep per-value hashing and player hashing in separate
// calls, preserving the byte stream without accumulating their stack slots.
[[gnu::noinline]] inline void deathHashWord(StateDigest& digest, std::uint32_t value) {
    digest.addU32(value);
}
[[gnu::noinline]] inline void deathHashBool(StateDigest& digest, bool value) {
    digest.addBool(value);
}
[[gnu::noinline]] inline void deathHashPlayer(StateDigest& digest, const PlayerMotion& player) {
    player.appendStateDigest(digest);
}
// Small CPU-equivalence fixture, not a Windows recording or full-world replay.
// No inlining: keep the player's landing-profile storage out of the platform
// main function's already large stack frame on the 4 KiB Amiga task stack.
[[gnu::noinline]] inline PlayerDeathVerificationResult verifyPlayerDeath() {
    StateDigest digest;
    unsigned coverage = 0;
    deathHashWord(digest, 2);
    // Arithmetic boundaries exercise the timer on the actual target CPU too.
    for (std::uint32_t seconds = 1; seconds <= (1u << 26); seconds <<= 1) {
        for (int offset = -2; offset <= 2; ++offset) {
            const auto parts = RunTimeParts::fromTicks(seconds * 60 + offset);
            deathHashWord(digest, parts.minutes);
            deathHashWord(digest, parts.seconds);
            deathHashWord(digest, parts.centiseconds);
        }
    }
    for (unsigned scenario = 0; scenario < 2; ++scenario) {
        PlayerMotion player(false, 32);
        PlayerDeathSequence death;
        RunClock clock;
        int health = 4;
        unsigned shotsAtDeath = 0, jumpsAtDeath = 0;
        for (unsigned tick = 0; tick < 240; ++tick) {
            Input input;
            input.right = true; // Must be suppressed only after publication.
            input.fire = (tick >= 8 && tick <= 14) ||
                (scenario == 1 && (tick == 16 || tick == 18 || tick == 20));
            death.beginStep(input.fire);
            player.step(input, false, death.movementDead());
            clock.advance(true, death.active);
            death.publishHealth(health);
            const bool effect = death.advanceAlarms(player.grounded());
            if (tick == 10) { // Collision phase, after player Begin Step.
                health = 0;
                player.contactKnockback(-4, 0);
                if (!death.active) coverage |= 1;
            }
            if (death.active && !death.movementDead()) {
                coverage |= 2;
                shotsAtDeath = player.shots(); jumpsAtDeath = player.jumps();
            }
            if (death.movementDead() && player.speedYFixed() < 0) coverage |= 4;
            if (scenario == 0 && death.animation == -1) coverage |= 8;
            if (scenario == 0 && death.menuReady && death.frames == 150) coverage |= 16;
            if (scenario == 1 && death.menuReady && death.skipPresses == 3 && death.frames < 30) coverage |= 32;
            if (effect) coverage |= 64;
            if (death.movementDead() && player.grounded() && player.speedXFixed() == 0 &&
                player.shots() == shotsAtDeath && player.jumps() == jumpsAtDeath) coverage |= 128;
            if (scenario == 1 && death.menuReady && death.animation == -2 && death.alarm == -1) coverage |= 256;
            deathHashWord(digest, scenario); deathHashWord(digest, tick); deathHashWord(digest, health);
            deathHashBool(digest, input.fire); deathHashBool(digest, effect);
            deathHashBool(digest, death.active); deathHashBool(digest, death.started);
            deathHashBool(digest, death.priorFire); deathHashBool(digest, death.menuReady);
            deathHashWord(digest, death.frames); deathHashWord(digest, death.skipPresses);
            deathHashWord(digest, death.animation); deathHashWord(digest, death.alarm);
            deathHashWord(digest, clock.ticks);
            deathHashPlayer(digest, player);
            deathHashWord(digest, player.spriteId(death.movementDead()));
            deathHashWord(digest, player.spriteFrame(death.movementDead(), death.animation));
        }
    }
    return {digest.value(), coverage};
}
} // namespace downwell
#endif
