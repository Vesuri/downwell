#ifndef DOWNWELL_CORE_BAT_HIT_STATE_H
#define DOWNWELL_CORE_BAT_HIT_STATE_H
namespace downwell {
// Hit delivery uses the same provisional next-Step queue as frog/snail.
// Alarm 0 belongs to the world's shared alarm countdown, not a second timer.
struct BatHitState {
    bool pending = false, ready = false, stunned = false;
    int pendingDirection = 270, direction = 270;
    void queue(int value) { pending = true; pendingDirection = value; }
    void advanceFrame() {
        ready = pending;
        if (pending) direction = pendingDirection;
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
