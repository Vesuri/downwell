#ifndef DOWNWELL_PROJECTILE_PUBLICATION_H
#define DOWNWELL_PROJECTILE_PUBLICATION_H
#include "MachineGunTrajectory.h"
#include "RunnerImageValue.h"
#include "MultiplyBinary64.h"
namespace downwell {
// Publish the currently implemented Q20/Q32 trajectory without truncating its
// fractions to drawing pixels. This does not make that approximate trajectory
// an original-runner oracle; projectile selectors remain gated until replaced.
struct ProjectilePublication {
    static bool coordinate(MachineGunTrajectory::Coordinate value,int origin,RunnerImageValue& out){
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
