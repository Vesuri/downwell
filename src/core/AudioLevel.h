#ifndef DOWNWELL_CORE_AUDIO_LEVEL_H
#define DOWNWELL_CORE_AUDIO_LEVEL_H
#include "FixedPoint.h"
#include <cstdint>
namespace downwell {
// W3.20: audio_sound_gain and audio_sound_pitch values, gameVolume and the
// master gain as Q16 (steps of 1/65536; the Paula mixer always worked in
// Q16). Source constants round to the nearest step: 0.7 is 45875/65536.
using AudioLevel=Fixed16;
namespace audio_level {
constexpr AudioLevel one=AudioLevel::fromInteger(1);
constexpr AudioLevel half=AudioLevel::raw(32768);
// The nearest Q16 of a stored binary64 bit pair (the installed asset
// volumes), with integer operations only; negative, invalid or huge values
// give zero, values above 256 saturate there.
inline AudioLevel fromBits(std::uint32_t high,std::uint32_t low){
    if(high&0x80000000u)return {};
    const int exponent=int((high>>20)&2047u)-1023;
    if(exponent==1024||exponent<-17)return {};
    if(exponent>=8)return AudioLevel::raw(256*65536);
    // significand (53 bits) * 2^(exponent-52) * 2^16 = m >> (36-exponent)
    const std::uint32_t mh=(high&0xfffffu)|0x100000u;
    const unsigned shift=unsigned(36-exponent); // 29..53
    std::uint32_t magnitude,roundBit;
    if(shift>=32){
        const unsigned s=shift-32;
        magnitude=mh>>s;roundBit=s?(mh>>(s-1))&1u:low>>31;
    }else{
        magnitude=(mh<<(32-shift))|(low>>shift);roundBit=(low>>(shift-1))&1u;
    }
    return AudioLevel::raw(std::int32_t(magnitude+roundBit));
}
// a * b, both nonnegative levels; saturates at 256.
inline AudioLevel product(AudioLevel a,AudioLevel b){
    if(a.v<=0||b.v<=0)return {};
    const std::uint64_t p=(std::uint64_t(std::uint32_t(a.v))*std::uint32_t(b.v)+0x8000u)>>16;
    return AudioLevel::raw(p>256u*65536u?256*65536:std::int32_t(p));
}
}
}
#endif
