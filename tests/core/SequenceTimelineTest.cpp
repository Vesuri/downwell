#include "core/SequenceTimeline.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
static void require(bool value, int line) { if (!value) { std::fprintf(stderr,"Sequence check failed at line %d\n",line); std::abort(); } }
#define check(value) require((value), __LINE__)
static downwell::RunnerImageValue stored(float value) {
    downwell::RunnerImageValue out; std::memcpy(&out.bits,&value,4); return out;
}
static float wrapped(float value, unsigned length) {
    const float first = float(std::fmod(double(value),double(length)));
    const float added = first + float(length);
    return float(std::fmod(double(added),double(length)));
}
int main() {
    using downwell::SequenceTimeline;
    unsigned cases = 0;
    std::uint32_t rng = 7;
    for (unsigned i = 0; i < 100000; ++i) {
        rng = rng * 1664525u + 1013904223u;
        downwell::RunnerImageValue input; input.bits = (rng & 0x807fffffu) | ((1 + i % 254) << 23);
        float value; std::memcpy(&value,&input.bits,4);
        const unsigned length = 1 + (rng >> 16) % 65535;
        downwell::RunnerImageValue result;
        check(SequenceTimeline::remainder(input,length,result));
        check(result.bits == stored(float(std::fmod(double(value),double(length)))).bits);
        check(SequenceTimeline::wrap(input,length,result));
        check(result.bits == stored(wrapped(value,length)).bits);
        ++cases;
    }
    for (unsigned length : {1u,4u,6u,8u}) for (float speed : {-20.0f,-0.45f,-0.3f,-0.05f,0.0f,0.2f,0.3f,20.0f}) {
        float expected = 0;
        auto position = stored(expected);
        for (unsigned tick = 0; tick < 1000; ++tick) {
            expected = float(double(expected) + double(speed));
            const bool boundary = expected < 0 || expected >= float(length);
            if (boundary) expected = wrapped(expected,length);
            bool crossed = !boundary;
            check(SequenceTimeline::advance(position,stored(speed),length,crossed));
            check(position.bits == stored(expected).bits && crossed == boundary);
            ++cases;
        }
    }
    auto position = stored(1); bool crossed = true;
    check(!SequenceTimeline::advance(position,stored(1),0,crossed) && position.bits == stored(1).bits && crossed);
    downwell::RunnerImageValue invalid; invalid.bits = 0x7f800000u;
    check(!SequenceTimeline::advance(position,invalid,4,crossed) && position.bits == stored(1).bits);
    check(!SequenceTimeline::advance(position,stored(1),65536,crossed) && position.bits == stored(1).bits);
    downwell::RunnerImageValue result;
    check(SequenceTimeline::remainder(stored(-0.0f),4,result) && result.bits == 0x80000000u);
    check(SequenceTimeline::wrap(stored(-0.0f),4,result) && result.bits == 0);
    invalid.bits = 1;
    check(!SequenceTimeline::advance(position,invalid,4,crossed) && position.bits == stored(1).bits);
    std::printf("Sequence timeline: %u signed remainder/wrap/advance cases passed\n",cases);
}
