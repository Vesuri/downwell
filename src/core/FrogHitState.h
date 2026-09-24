#ifndef DOWNWELL_CORE_FROG_HIT_STATE_H
#define DOWNWELL_CORE_FROG_HIT_STATE_H
namespace downwell {
// Existing-enemy Step precedes a newly created bullet's Step. Queue bullet
// hit flags for the following frog update, after its alarm phase.
struct FrogHitState {
    bool pending = false;
    bool ready = false;
    unsigned remaining = 0;
    void advanceFrame() {
        ready = pending;
        pending = false;
        if (remaining != 0) --remaining;
    }
    bool consume() {
        if (!ready) return false;
        ready = false;
        remaining = 5;
        return true;
    }
    static long dampHorizontal(long q16) {
        const long scaled = q16 * 4;
        return scaled < 0 ? -((-scaled + 2) / 5) : (scaled + 2) / 5;
    }
};
} // namespace downwell
#endif
