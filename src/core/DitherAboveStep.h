#ifndef DOWNWELL_CORE_DITHER_ABOVE_STEP_H
#define DOWNWELL_CORE_DITHER_ABOVE_STEP_H
#include "RunnerImageValue.h"
#include "SequenceTimeline.h"
namespace downwell {
// Object 61 Begin Step (990). Operations read published fields, not camera
// easing accumulators. Each successful setter commits before the next read.
// Boolean inputs are already converted using the VM's boolean semantics.
struct DitherAboveStep {
    template<class Operations> static bool run(Operations& ops){
        MotionBinary64 camera;
        RunnerImageValue value;
        if(!ops.cameraX(camera)||!value.assign(camera)||!ops.setX(value))return false;
        bool lowSpec;
        if(!ops.lowSpec(lowSpec)||!ops.cameraY(camera)||!camera.valid())return false;
        RunnerImageValue y;
        if(!ops.y(y)||!SequenceTimeline::valid(y))return false;
        auto negativeY=y.promoted();negativeY.high^=0x80000000u;
        if(!camera.add(negativeY))return false;
        const auto threshold=MotionBinary64::fromInteger(lowSpec?208:272);
        const bool advance=!(camera.high&0x80000000u)&&
            (camera.high>threshold.high||(camera.high==threshold.high&&camera.low>=threshold.low));
        if(advance){
            // The source reads y again for +=, then narrows through its setter.
            if(!ops.y(y)||!SequenceTimeline::valid(y))return false;
            auto next=y.promoted();
            if(!next.add(MotionBinary64::fromInteger(16))||!value.assign(next)||!ops.setY(value))return false;
        }
        // round(y) reads the narrowed builtin, even after an increment.
        std::int32_t rounded;
        if(!ops.y(y)||!SequenceTimeline::valid(y)||!y.roundPixel(rounded)||!value.assignInteger(rounded)||!ops.setY(value))return false;
        bool ending;
        return ops.ending(ending)&&(!ending||ops.destroySelf());
    }
};
}
#endif
