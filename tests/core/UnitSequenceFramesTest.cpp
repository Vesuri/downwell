#include "core/UnitSequenceFrames.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
static void check(bool v) { if (!v) std::abort(); }
static downwell::RunnerImageValue stored(float f) {
    downwell::RunnerImageValue value; std::memcpy(&value.bits,&f,4); return value;
}
int main() {
    using downwell::UnitSequenceFrames;
    unsigned cases = 0;
    for (unsigned count : {1u,4u,6u,8u,65535u}) for (unsigned i = 0; i < 10000; ++i) {
        const float position = float(double(i) * count / 10000);
        const auto key = unsigned(position);
        // The last key interpolates toward frame+1 using key length;
        // earlier keys interpolate toward the next key's frame/time.
        const float start = float(key), end = float(key+1);
        const float fraction = (position-start)/(end-start);
        const float expected = fraction*(float(key+1)-float(key))+float(key);
        downwell::RunnerImageValue image;
        check(UnitSequenceFrames::evaluate(stored(position),count,image));
        check(image.bits == stored(expected).bits);
        ++cases;
    }
    for (float base : {0.0f,0.3f,1.0f,7.0f}) for (int i = -100; i <= 100; ++i) {
        const float written = base + float(i)*0.000001f;
        const float difference = written-base;
        bool needed = false;
        check(UnitSequenceFrames::needsReconcile(stored(written),stored(base),needed));
        check(needed == (std::fabs(double(difference)) > 0.00001));
        ++cases;
    }
    auto timeline = stored(0), image = stored(0);
    bool crossed = false;
    check(UnitSequenceFrames::advance(timeline,stored(-0.3f),4,image,crossed));
    check(crossed && timeline.bits == image.bits && image.bits == stored(3.7f).bits);
    check(!UnitSequenceFrames::evaluate(stored(-0.3f),4,image));
    check(!UnitSequenceFrames::evaluate(stored(4),4,image));
    check(UnitSequenceFrames::evaluate(stored(-0.0f),4,image) && image.bits == 0);
    std::printf("Unit sequence frames: %u interpolation/reconciliation comparisons and reverse advance passed\n",cases);
}
