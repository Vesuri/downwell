#ifndef DOWNWELL_CORE_SMOKE_EFFECT_H
#define DOWNWELL_CORE_SMOKE_EFFECT_H
#include "SmokeMotion.h"
#include "UnitSequenceFrames.h"
namespace downwell {
// Object 225, sprite 84: no image_index write in Create. The runner initializes
// offset 0x94 to zero (0x46190e); unlike flash balls, smoke starts at frame zero.
struct SmokeEffect : SmokeMotion {
    RunnerImageValue timeline{},imageIndex{};
    bool destroyRequested=false;
    template<class Random> bool create(MotionBinary64 x,MotionBinary64 y,Random& random) {
        SmokeEffect next;
        if (!next.SmokeMotion::create(x,y,random)) return false;
        *this=next;return true;
    }
    // Owner calls this before Begin Step; first animation therefore uses the
    // Create rate, while subsequent updates use first Step's resampled rate.
    bool advanceImage() {
        bool crossed=false;
        if (!UnitSequenceFrames::advance(timeline,imageSpeed,9,imageIndex,crossed)) return false;
        if (crossed) destroyRequested=true; // Other 7: action_kill_object.
        return true;
    }
    unsigned drawFrame() const {
        const unsigned exponent=(imageIndex.bits>>23)&255u;
        return exponent<127?0:((imageIndex.bits&0x7fffffu)|0x800000u)>>(150u-exponent);
    }
};
}
#endif
