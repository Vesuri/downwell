#ifndef DOWNWELL_CORE_SMOKE_BIRTH_H
#define DOWNWELL_CORE_SMOKE_BIRTH_H
#include "RandomBinary64.h"
#include "MultiplyBinary64.h"
#include "RunnerImageValue.h"
namespace downwell {
// Object 225's synchronous fxSmoke Create only. First Step deliberately has a
// second animation-rate sample; owners must not treat this as initialized
// velocity or bypass that event. Position arrives through builtin float fields.
struct SmokeBirth {
    MotionBinary64 xx{},yy{},speed{},direction{};
    RunnerImageValue imageSpeed{};
    bool noCollision=true,allSet=false;
    template<class Random> bool sampleImageSpeed(Random& random) {
        MotionBinary64 rate,product;
        // double(0.13)-double(0.05), as computed by random_range.
        if (!randomBinary64(random.nextU32(),{0x3fb47ae1u,0x47ae147bu},rate) ||
            !rate.add({0x3fa99999u,0x9999999au}) ||
            !multiplyBinary64(speed,rate,product)) return false;
        return imageSpeed.assign(product);
    }
    template<class Random> bool create(MotionBinary64 x,MotionBinary64 y,Random& random) {
        SmokeBirth next;
        RunnerImageValue px,py;
        if (!px.assign(x) || !py.assign(y)) return false;
        next.xx=px.promoted();next.yy=py.promoted();
        if (!randomBinary64(random.nextU32(),MotionBinary64::fromInteger(2),next.speed) ||
            !next.speed.add(MotionBinary64::fromInteger(2)) || !next.sampleImageSpeed(random) ||
            !randomBinary64(random.nextU32(),MotionBinary64::fromInteger(359),next.direction)) return false;
        *this=next;return true;
    }
};
}
#endif
