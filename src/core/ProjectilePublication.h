#ifndef DOWNWELL_PROJECTILE_PUBLICATION_H
#define DOWNWELL_PROJECTILE_PUBLICATION_H
#include "BitCount.h"
#include "MachineGunTrajectory.h"
#include "RunnerImageValue.h"
#include "MultiplyBinary64.h"
#include "Profile.h"
namespace downwell {
// Publish the currently implemented Q20/Q32 trajectory without truncating its
// fractions to drawing pixels. This does not make that approximate trajectory
// an original-runner oracle; projectile selectors remain gated until replaced.
struct ProjectilePublication {
    static bool coordinate(MachineGunTrajectory::Coordinate value,int origin,RunnerImageValue& out){
        DW_PROFILE_SCOPE(profileProjectileCoordinate);
        if(origin<0||value.fraction>=unsigned(MachineGunTrajectory::unit)||
            value.pixel>0x7fffffff-origin)return false;
        const std::int32_t pixel=value.pixel+origin;
        const bool negative=pixel<0;
        std::uint32_t whole=negative?0u-std::uint32_t(pixel):std::uint32_t(pixel);
        std::uint32_t fraction=value.fraction;
        if(negative&&fraction){--whole;fraction=unsigned(MachineGunTrajectory::unit)-fraction;}
        // Exact magnitude in units of 2^-20, at most 52 significant bits.
        // The retained binary64 addition is exact throughout this domain.
        const std::uint32_t high=whole>>12,low=(whole<<20)|fraction;
        if(!high&&!low){out.bits=0;return true;}
        unsigned top=high?63-leadingZeros32(high):
            31-leadingZeros32(low);
        std::uint32_t mantissa;
        if(top<=23)mantissa=low<<(23-top);
        else{
            const unsigned shift=top-23; // 1..28: no 32-bit/64-bit shift helpers.
            mantissa=(high<<(32-shift))|(low>>shift);
            const std::uint32_t tail=low&((1u<<shift)-1),half=1u<<(shift-1);
            if(tail>half||(tail==half&&(mantissa&1u)))++mantissa;
            if(mantissa==0x1000000u){mantissa>>=1;++top;}
        }
        out.bits=(negative?0x80000000u:0u)|((top+107u)<<23)|(mantissa&0x7fffffu);
        return true;
    }
    // Portable reference for tests; not used by the live publication path.
    static bool coordinateBinary64(MachineGunTrajectory::Coordinate value,int origin,RunnerImageValue& out){
        if(origin<0||value.fraction>=unsigned(MachineGunTrajectory::unit)||
            value.pixel>0x7fffffff-origin)return false;
        auto result=MotionBinary64::fromInteger(value.pixel+origin);
        auto fraction=MotionBinary64::fromInteger(int(value.fraction));
        if(!fraction.zero())fraction.high-=20u<<20;
        return result.add(fraction)&&out.assign(result);
    }
    static bool angle(std::uint32_t turns,RunnerImageValue& out){
        auto half=MotionBinary64::fromInteger(int(turns>>1));
        if((turns&1u)&&!half.add({0x3fe00000u,0}))return false;
        if(!half.zero())half.high-=31u<<20;
        MotionBinary64 degrees;
        return multiplyBinary64(half,MotionBinary64::fromInteger(360),degrees)&&out.assign(degrees);
    }
};
}
#endif
