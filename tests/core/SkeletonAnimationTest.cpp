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
    unsigned changes=0;
    // All admitted counts, just below/at/above the reset threshold. Retain the
    // old binary64 conversion as an independent oracle for the narrowed bound.
    for(unsigned frames=1;frames<=65535;++frames){
        downwell::RunnerImageValue bound;
        check(bound.assign(downwell::MotionBinary64::fromInteger(frames)));
        for(int delta:{-1,0,1}){
            downwell::RunnerSpriteChange::Result result;
            check(downwell::RunnerSpriteChange::plan(1,2,{bound.bits+delta},frames,result));
            check(result.changed&&result.resetIndex==(delta>=0)&&result.resetTimeline==(delta>=0));
            downwell::RunnerSpriteChange::Result reference;
            check(downwell::RunnerSpriteChange::planReference(1,2,{bound.bits+delta},frames,reference));
            check(result.changed==reference.changed&&result.resetIndex==reference.resetIndex&&
                result.resetTimeline==reference.resetTimeline);
            ++changes;
        }
    }
    // Exercise every exponent and sign, including rejected values, with
    // arbitrary mantissas/counts and unchanged-ID early exits.
    std::uint32_t random=73191;
    for(unsigned i=0;i<262144;++i){
        random=random*1664525u+1013904223u;
        const unsigned bits=(i%512u)<<23|(random&0x7fffffu);
        const unsigned frames=(random>>8)%65538u;
        downwell::RunnerSpriteChange::Result actual{true,false,true},reference=actual;
        const int next=i%17?2:1;
        check(downwell::RunnerSpriteChange::plan(1,next,{bits},frames,actual)==
            downwell::RunnerSpriteChange::planReference(1,next,{bits},frames,reference));
        check(actual.changed==reference.changed&&actual.resetIndex==reference.resetIndex&&
            actual.resetTimeline==reference.resetTimeline);
        ++changes;
    }
    for(unsigned frames:{0u,1u,6u,8u,65535u})for(int eighth=-1024;eighth<=2048;++eighth){
        const float value=float(eighth)/8;
        downwell::RunnerImageValue index;std::memcpy(&index.bits,&value,4);
        for(bool sameId:{false,true}){
            downwell::RunnerSpriteChange::Result result{true,true,true};
            check(downwell::RunnerSpriteChange::plan(1,sameId?1:2,index,frames,result));
            const bool reset=!sameId&&int(value)>=int(frames);
            check(result.changed==!sameId&&result.resetIndex==reset&&result.resetTimeline==(reset&&frames!=0));
            ++changes;
        }
    }
    for(auto bits:{0x80000000u,0xcf000000u,0x4effffffu}){
        float value;std::memcpy(&value,&bits,4);
        downwell::RunnerSpriteChange::Result result;
        check(downwell::RunnerSpriteChange::plan(1,2,{bits},0,result));
        check(result.resetIndex==(int(value)>=0)&&!result.resetTimeline);
    }
    for(auto bits:{1u,0x7f800000u,0x7fc00000u,0x4f000000u,0xcf000001u}){
        downwell::RunnerSpriteChange::Result result{false,true,true};
        check(!downwell::RunnerSpriteChange::plan(1,2,{bits},6,result));
        check(!result.changed&&result.resetIndex&&result.resetTimeline);
        check(downwell::RunnerSpriteChange::plan(1,1,{bits},6,result)&&!result.changed);
    }
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
    std::printf("Shared sprite changes: %u signed-index, empty-track and same-ID comparisons passed\n",changes);
}
