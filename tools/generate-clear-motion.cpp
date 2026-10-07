#ifndef DOWNWELL_REFERENCE_CLEAR_MOTION
#define DOWNWELL_REFERENCE_CLEAR_MOTION
#endif
#include "../src/core/LevelClearMotion.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
using downwell::HudReal;
static std::uint64_t bits(HudReal value) {
    return value.mantissa ? (std::uint64_t(value.exponent+1023)<<52)|
        (value.mantissa&0xfffffffffffffULL) : 0;
}
static double real(HudReal value) {
    const auto raw=bits(value); double result;std::memcpy(&result,&raw,sizeof result);return result;
}
static void check(double actual,HudReal expected) { if(actual!=real(expected))std::abort(); }
static void emit(HudReal value) { std::printf("0x%016llxULL",static_cast<unsigned long long>(bits(value))); }
int main() {
    downwell::LevelClearMotion motion;
    double player=0,camera=142;
    std::puts("// Generated exact binary64 states; regenerate with generate-clear-motion.py.");
    std::puts("inline constexpr std::uint64_t entrance[] = {");
    for(unsigned i=0;i<60;++i){
        emit(motion.playerY);std::puts(",");
        if(i!=59){motion.step(false);player+=(160-player)/20;check(player,motion.playerY);}
    }
    std::puts("};\ninline constexpr std::uint64_t choices[][2] = {");
    motion.showChoices();
    for(unsigned i=0;i<1024;++i){
        std::printf("{");emit(motion.playerY);std::printf(",");emit(motion.cameraY);std::puts("},");
        const auto p=bits(motion.playerY),c=bits(motion.cameraY);
        motion.step(false);player+=(302-player)/10;camera+=(340-camera)/10;
        check(player,motion.playerY);check(camera,motion.cameraY);
        if(p==bits(motion.playerY)&&c==bits(motion.cameraY)){std::puts("};");return 0;}
    }
    return 1;
}
