#ifndef DOWNWELL_CORE_BAT_HIT_STATE_H
#define DOWNWELL_CORE_BAT_HIT_STATE_H
#include "LevelClearTrig.h"
namespace downwell {
// Hit delivery uses the same provisional next-Step queue as frog/snail.
// Alarm 0 belongs to the world's shared alarm countdown, not a second timer.
struct BatHitState {
    bool pending = false, ready = false, stunned = false;
    int pendingDirection = 270, direction = 270;
    unsigned short pendingFraction = 0, fraction = 0;
    void queue(int value) { pending = true; pendingDirection = value; pendingFraction = 0; }
    void queueTurns(std::uint32_t turns) {
        // Q32 turns -> degrees Q16, without target 64-bit arithmetic.
        const auto degrees = LevelClearTrig::productHigh(turns, 360u * 65536u);
        queue(static_cast<int>(degrees >> 16));
        pendingFraction = static_cast<unsigned short>(degrees);
    }
    void advanceFrame() {
        ready = pending;
        if (pending) { direction = pendingDirection; fraction = pendingFraction; }
        pending = false;
    }
    bool consume() {
        if (!ready) return false;
        ready = false; stunned = true;
        return true;
    }
    void alarm() { stunned = false; }
};
}
#endif
