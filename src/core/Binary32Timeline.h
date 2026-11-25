#ifndef DOWNWELL_CORE_BINARY32_TIMELINE_H
#define DOWNWELL_CORE_BINARY32_TIMELINE_H
#include <cstdint>
namespace downwell {
// A nonnegative binary32 sprite timeline below 16 frames, held in 32-bit Q27
// (W3 fixed point). Every binary32 value in [1/16, 16) is exact in Q27, and
// each sum is rounded to the binary32 spacing of its magnitude (ties to
// even), so advancing by a rate of at least 1/16 reproduces SequenceTimeline's
// binary32 additions bit for bit: the same frames and the same Animation End
// tick. Smaller rates round to the nearest Q27 step.
struct Binary32Timeline {
    static constexpr unsigned fraction=27;
    static constexpr std::int32_t one=std::int32_t(1)<<fraction;
    static constexpr unsigned maximumFrames=15;
    // A binary32 rate (its bits), nonnegative and below 16.
    static bool rate(std::uint32_t bits,std::int32_t& q27){
        if(bits&0x80000000u){if(bits&0x7fffffffu)return false;q27=0;return true;}
        const unsigned biased=(bits>>23)&255u;
        if(!biased){q27=0;return true;} // zero and subnormals
        if(biased>=127+4)return false;
        const std::uint32_t m=(bits&0x7fffffu)|0x800000u;
        const int shift=int(biased)-127+4; // value = m * 2^(shift-27)
        if(shift>=0){q27=std::int32_t(m<<shift);return true;}
        if(shift<-24){q27=0;return true;}
        q27=std::int32_t((m+(std::uint32_t(1)<<(-shift-1)))>>-shift);return true;
    }
    // Round a Q27 sum to binary32's 24 significant bits, ties to even.
    static std::int32_t round(std::uint32_t v){
        if(v<(std::uint32_t(1)<<24))return std::int32_t(v);
        unsigned top=24;while(v>>(top+1))++top;
        const unsigned drop=top-23;
        const std::uint32_t unit=std::uint32_t(1)<<drop,low=v&(unit-1),half=unit>>1;
        v-=low;
        if(low>half||(low==half&&(v&unit)))v+=unit;
        return std::int32_t(v);
    }
    // position += rate; reaching frames wraps once and reports Animation End.
    static bool advance(std::int32_t& position,std::int32_t rate,std::int32_t frames){
        position=round(std::uint32_t(position)+std::uint32_t(rate));
        if(position<frames)return false;
        position=round(std::uint32_t(position-frames));
        return true;
    }
};
}
#endif
