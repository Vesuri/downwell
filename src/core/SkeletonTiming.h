#ifndef DOWNWELL_CORE_SKELETON_TIMING_H
#define DOWNWELL_CORE_SKELETON_TIMING_H

#include "MotionBinary64.h"
#include "AlarmBank.h"
#include "RunnerImageValue.h"

namespace downwell {
// enmSkeleton's four Alarm events and the timing writes in Step_0.
// Call alarm for each dispatched Alarm event, then beginStep in ordinary Step.
// Animation advancement, sprite selection and the hit sound belong to the
// caller. In particular, changing imgSp does NOT immediately change image_speed.
struct SkeletonTiming {
    std::int32_t alarms[4] = {-1, -1, -1, -1};
    MotionBinary64 imageIndex{}, imageSpeed{}, rate{};
    bool damaged = false, hitStun = false, shot = false;

    static MotionBinary64 throwRate() { return {0x3fd33333u, 0x33333333u}; }

    // The runner visits alarm indices globally, then instances within each
    // index. The owner must call this in that order, not run all slots for
    // one instance before visiting the next. Higher-slot writes take effect
    // in this phase; lower-slot writes wait until the next phase.
    bool advanceAlarm(unsigned index) {
        if (index >= 4 || !AlarmBank::advanceValue(alarms[index])) return false;
        alarm(index);
        return true;
    }

    void alarm(unsigned index) {
        switch (index) {
            case 0: damaged = false; break;
            case 1: hitStun = false; break;
            case 2:
                imageIndex = {}; shot = false; alarms[3] = 15;
                break;
            case 3: rate = throwRate(); break;
        }
    }

    void beginStep(bool stopped) {
        if (!stopped) {
            RunnerImageValue stored;
            // This state admits only recovered rates 0, 0.2 and 0.3.
            if (stored.assign(rate)) imageSpeed = stored.promoted();
        }
        else {
            imageSpeed = {};
            if (alarms[0] > 0) ++alarms[0];
        }
    }

    // Caller applies the active/hit guards and clears the consumed hit flag.
    void hit() {
        hitStun = true;
        alarms[1] += 5;
        imageIndex = {};
        rate = {};
        alarms[3] = 30;
    }

    // Caller applies active && image_index > 6. The nonzero imgSp guard
    // prevents rearming Alarm 2 on every stopped animation frame.
    void finishThrow() {
        if ((rate.high & 0x7fffffffu) || rate.low) {
            rate = {};
            alarms[2] = 30;
        }
    }
};
} // namespace downwell
#endif
