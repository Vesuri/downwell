// Host-only domain audit, not a runtime lookup generator. Uses the retained
// integer program for every rounding boundary; no host libm substitutions.
#include "../src/core/TableAtan.h"
#include "../src/core/RunnerPointDirection.h"
#include "../src/core/SmokeDirection.h"
#include "../src/core/SmokeTrig.h"
#include <cstdio>
#include <set>
#include <cstdint>

int main() {
    using namespace downwell;
    std::set<std::uint32_t> directions, speeds;
    std::uint32_t digest=2166136261u;
    auto word=[&](std::uint32_t value){digest=(digest^value)*16777619u;};
    // The table returns signed Q16 radians in [-pi,pi]. Enumerating every
    // integer in this interval is a superset, independent of contact tapes,
    // ratio interpolation and which live collision masks currently occur.
    for(unsigned q=0;q<=table_atan_detail::pi;++q)for(unsigned sign=0;sign<2;++sign){
        const auto radians=RunnerImageValue{table_atan_detail::rounded(q<<14)|(sign<<31)}.promoted();
        MotionBinary64 direction;
        // Non-axis synthetic differences reach the original post-atan wrapper.
        // They do not stand in for the contact geometry's admission predicate.
        if(!RunnerPointDirection::evaluate({}, {},MotionBinary64::fromInteger(1),
            MotionBinary64::fromInteger(1),direction,
            [&](auto,auto,auto& result){result=radians;return true;}))return 1;
        RunnerImageValue angle,radian,speed;
        MotionBinary64 cosine,motion;
        if(!angle.assign(direction)||!SmokeDirection::radians(angle,radian)||
           !smokeCosine(radian.promoted(),cosine)||
           !SmokeDirection::component(RunnerImageValue{0x40000000u},cosine,false,motion)||
           !speed.assign(motion))return 2;
        // lengthdir_x returns a promoted binary32 (including signed zero).
        const auto promoted=speed.promoted();
        if(promoted.high!=motion.high||promoted.low!=motion.low)return 3;
        directions.insert(angle.bits);speeds.insert(speed.bits);
        word(angle.bits);word(speed.bits);
    }
    std::printf("Q16 pi: %u\nsigned inputs: %u\ndistinct directions: %zu\n"
        "distinct speeds: %zu\nflat speed payload bytes: %zu\ndigest: %08x\n",
        table_atan_detail::pi,2*(table_atan_detail::pi+1),directions.size(),
        speeds.size(),directions.size()*sizeof(std::uint32_t),digest);
}
