#include "core/SkeletonAnimation.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
static void check(bool v) { if (!v) std::abort(); }
static bool same(downwell::MotionBinary64 v, double expected) {
    std::uint64_t raw; std::memcpy(&raw,&expected,8);
    return v.high == std::uint32_t(raw>>32) && v.low == std::uint32_t(raw);
}
int main() {
    unsigned updates = 0;
    for (unsigned seed = 0; seed < 64; ++seed) {
        downwell::SkeletonAnimation actual;
        int sprite = 208;
        float timeline = 0, image = 0, speed = 0;
        double rate = 0;
        unsigned rng = seed;
        for (unsigned tick = 0; tick < 500; ++tick) {
            const unsigned count = downwell::SkeletonAnimation::frames(sprite);
            // Scalar model: frame-zero writes, unit tracks and loop playback.
            if (timeline < 0 || timeline >= count || std::fabs(double(float(image-timeline))) > 0.00001)
                timeline = 0;
            timeline += speed;
            const bool expectedBoundary = timeline < 0 || timeline >= count;
            if (expectedBoundary) {
                const float rem = float(std::fmod(double(timeline),double(count)));
                timeline = float(std::fmod(double(float(rem+float(count))),double(count)));
            }
            image = timeline;
            bool crossed = !expectedBoundary;
            check(actual.advanceImage(crossed));
            check(same(actual.imageIndex,double(image)) && crossed == expectedBoundary);
            rng = rng*1664525u+1013904223u;
            const bool stopped = (rng & 15) == 0;
            actual.beginStep(stopped); speed = stopped ? 0 : float(rate);
            if (rng % 13 == 0) { actual.imageIndex = {}; image = 0; }
            if (rng % 7 == 0) {
                const int next = rng % 3 == 0 ? 205 : rng % 3 == 1 ? 207 : 208;
                if (sprite != next && int(image) >= int(downwell::SkeletonAnimation::frames(next))) image = timeline = 0;
                check(actual.setSprite(next)); sprite = next;
            }
            if (rng % 11 == 0) { actual.rate = {}; rate = 0; }
            if (rng % 5 == 0) { actual.alarm(3); rate = 0.3; }
            check(same(actual.imageIndex,double(image)) && same(actual.imageSpeed,double(speed)));
            ++updates;
        }
    }
    downwell::SkeletonAnimation rejected;
    rejected.imageIndex = downwell::MotionBinary64::fromInteger(3);
    bool crossed = true;
    check(!rejected.advanceImage(crossed) && crossed && rejected.timeline.bits == 0);
    check(!rejected.setSprite(460) && rejected.sprite == 208);
    std::printf("Skeleton animation: %u reset/sprite/time-stop updates match scalar sequence model\n",updates);
}
