#ifndef DOWNWELL_CORE_SNAIL_HIT_STATE_H
#define DOWNWELL_CORE_SNAIL_HIT_STATE_H
#include "AlarmBank.h"
namespace downwell {
// Queued like the existing frog adapter until world collision-event dispatch
// is consolidated. Unlike frog hit-stun, snail Alarm 2 is extended, not reset.
struct SnailHitState {
    bool pending = false, ready = false, stunned = false;
    int alarm2 = -1;
    void advanceFrame() {
        ready = pending; pending = false;
        if (AlarmBank::advanceValue(alarm2)) stunned = false;
    }
    template<class Random>
    int consume(Random& random) {
        if (!ready) return -1;
        ready = false; stunned = true;
        alarm2 += 5;
        // Positive random_range(50,80), projected to the integer alarm bank.
        return 50 + static_cast<int>(random.randomIntegerSmall(30));
    }
};
}
#endif
