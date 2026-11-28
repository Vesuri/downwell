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
    // needsReconcile against its binary64 form over valid operand pairs
    // near and far apart, including the 1e-5 boundary.
    for (unsigned n = 0, rng = 11; n < 4000000; ++n) {
        rng = rng * 1664525u + 1013904223u; const auto r1 = rng;
        rng = rng * 1664525u + 1013904223u; const auto r2 = rng;
        downwell::RunnerImageValue w{r1 & 0x7fffffffu}, e{};
        const unsigned mode = r2 & 3;
        if (mode == 0) e.bits = r2 >> 2 & 0x7fffffffu;
        else e.bits = w.bits + ((r2 >> 4) & 0xffffu) - 0x8000u; // nearby values
        if (mode == 3) w.bits = (r1 & 0x7fffffu) | (112u << 23), e.bits = w.bits ^ ((r2 >> 3) & 0x3ffu);
        if (!downwell::SequenceTimeline::valid(w) || !downwell::SequenceTimeline::valid(e)) continue;
        auto difference = w.promoted();
        auto subtract = e.promoted(); subtract.high ^= 0x80000000u;
        downwell::RunnerImageValue rounded;
        const bool ok = difference.add(subtract) && rounded.assign(difference);
        bool expected = false;
        if (ok) {
            rounded.bits &= 0x7fffffffu;
            const auto magnitude = rounded.promoted();
            expected = magnitude.high > 0x3ee4f8b5u ||
                (magnitude.high == 0x3ee4f8b5u && magnitude.low > 0x88e368f1u);
        }
        bool needed = !expected;
        check(UnitSequenceFrames::needsReconcile(w, e, needed) == ok);
        if (ok) check(needed == expected);
        ++cases;
    }
    for (std::uint32_t bits : {0x3727c5acu, 0x3727c5adu}) {
        bool needed = false;
        check(UnitSequenceFrames::needsReconcile(downwell::RunnerImageValue{bits}, {}, needed) &&
            needed == (bits == 0x3727c5adu));
    }
    std::printf("Unit sequence frames: %u interpolation/reconciliation comparisons and reverse advance passed\n",cases);
}
