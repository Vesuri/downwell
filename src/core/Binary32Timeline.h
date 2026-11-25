#ifndef DOWNWELL_CORE_BINARY32_TIMELINE_H
#define DOWNWELL_CORE_BINARY32_TIMELINE_H
#include <cstdint>
namespace downwell {
// A binary32 sprite timeline below 16 frames, held in 32-bit Q27 (W3 fixed
// point). Every binary32 value of magnitude in [1/16, 16) is exact in Q27,
// and each sum is rounded to the binary32 spacing of its magnitude (ties to
// even), so advancing by a rate of magnitude at least 1/16 reproduces
// SequenceTimeline's binary32 additions and its wrap
// float(fmod(float(fmod(x, L)) + L, L)) bit for bit: the same frames and the
// same Animation End tick. Smaller rates round to the nearest Q27 step.
struct Binary32Timeline {
    // Index of the highest set bit of a nonzero value (bfffo on the 68020).
    static unsigned top(std::uint32_t v){return 31u-unsigned(__builtin_clz(v));}
    static constexpr unsigned fraction=27;
    static constexpr std::int32_t one=std::int32_t(1)<<fraction;
    static constexpr unsigned maximumFrames=15;
    // A binary32 value (its bits) of magnitude below 16 in Q27; exact is
    // false when bits below 2^-27 were rounded away.
    static bool fromBits(std::uint32_t bits,std::int32_t& q27,bool& exact){
        const unsigned biased=(bits>>23)&255u;
        exact=true;
        if(!biased){q27=0;exact=!(bits&0x7fffffu);return true;} // zeros; subnormals round to 0
        if(biased>=127+4)return false;
        const std::uint32_t m=(bits&0x7fffffu)|0x800000u;
        const int shift=int(biased)-127+4; // value = m * 2^(shift-27)
        std::uint32_t v;
        if(shift>=0)v=m<<shift;
        else if(shift<-24){v=0;exact=false;}
        else{v=(m+(std::uint32_t(1)<<(-shift-1)))>>-shift;exact=!(m&((std::uint32_t(1)<<-shift)-1));}
        q27=bits&0x80000000u?-std::int32_t(v):std::int32_t(v);return true;
    }
    // A nonnegative rate below 16 (its bits).
    static bool rate(std::uint32_t bits,std::int32_t& q27){
        bool exact;
        return fromBits(bits,q27,exact)&&q27>=0&&!((bits&0x80000000u)&&(bits&0x7fffffffu));
    }
    // The binary32 bits of a Q27 value that is itself a binary32 value.
    static std::uint32_t toBits(std::int32_t q27){
        if(!q27)return 0;
        const std::uint32_t sign=q27<0?0x80000000u:0,v=q27<0?std::uint32_t(-q27):std::uint32_t(q27);
        const unsigned t=top(v);
        const std::uint32_t m=t>23?v>>(t-23):v<<(23-t);
        return sign|(std::uint32_t(int(t)-27+127)<<23)|(m&0x7fffffu);
    }
    // Round a Q27 magnitude to binary32's 24 significant bits, ties to even.
    static std::uint32_t round(std::uint32_t v){
        if(v<(std::uint32_t(1)<<24))return v;
        const unsigned drop=top(v)-23;
        const std::uint32_t unit=std::uint32_t(1)<<drop,low=v&(unit-1),half=unit>>1;
        v-=low;
        if(low>half||(low==half&&(v&unit)))v+=unit;
        return v;
    }
    // position += rate for position in [0, frames) and |rate| <= frames;
    // leaving that range wraps once and reports the boundary (Animation End).
    static bool advance(std::int32_t& position,std::int32_t rate,std::int32_t frames){
        const std::uint32_t f=std::uint32_t(frames);
        if(rate>=0){
            const std::uint32_t sum=round(std::uint32_t(position)+std::uint32_t(rate));
            if(sum<f){position=std::int32_t(sum);return false;}
            // sum is in [L, 2L): fmod(sum, L) = sum - L exactly, adding L
            // back restores sum and the outer fmod gives sum - L again.
            position=std::int32_t(sum-f);
            return true;
        }
        const std::int32_t sum=position+rate;
        if(sum>=0){
            position=std::int32_t(round(std::uint32_t(sum)));
            return false;
        }
        // fmod of a negative value above -L is itself; + L rounds.
        const std::uint32_t added=round(f-round(std::uint32_t(-sum)));
        position=std::int32_t(added>=f?added-f:added);
        return true;
    }
};
}
#endif
