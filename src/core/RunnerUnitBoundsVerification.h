#ifndef DOWNWELL_RUNNER_UNIT_BOUNDS_VERIFICATION_H
#define DOWNWELL_RUNNER_UNIT_BOUNDS_VERIFICATION_H
#include "RunnerSpriteBounds.h"
namespace downwell {
struct RunnerUnitBoundsVerification {
    struct Resource {int marginLeft,marginTop,marginRight,marginBottom,originX,originY;};
    static bool run(){
        using B=RunnerSpriteBounds;using R=B::Real;
        for(unsigned i=0;i<32;++i){
            Resource resource{int(i)-16,11,7-int(i),19,8,12};
            R x,y;if(!x.assign(MotionBinary64::fromInteger(int(i)*127-2048))||
                !y.assign(MotionBinary64::fromInteger(int(i)*1024)))return false;
            for(unsigned policy=0;policy<5;++policy){
                unsigned fastCalls=0,referenceCalls=0;
                auto round=[&](unsigned& calls,R value,std::int32_t& out){
                    ++calls;if(policy==3&&calls==3)return false;
                    if(!value.promoted().roundPixel(out))return false;
                    if(policy==2)out+=calls&1?1:-1;
                    if(policy==4)out=calls&1?2147483647:(-2147483647-1);
                    return true;
                };
                B::Box fast{},reference{};
                const bool a=B::fromSprite(resource,x,y,R{0x3f800000u},R{0x3f800000u},false,{},{},policy!=0,
                    [&](R v,std::int32_t& p){return round(fastCalls,v,p);},fast);
                const bool b=B::fromSpriteBinary32(resource,x,y,R{0x3f800000u},R{0x3f800000u},false,{},{},policy!=0,
                    [&](R v,std::int32_t& p){return round(referenceCalls,v,p);},reference);
                if(a!=b||fastCalls!=referenceCalls||fast.left.bits!=reference.left.bits||
                    fast.right.bits!=reference.right.bits||fast.top.bits!=reference.top.bits||fast.bottom.bits!=reference.bottom.bits)return false;
            }
        }
        Resource zero{};B::Box signedZero;
        unsigned calls=0;
        if(!B::fromSprite(zero,R{0x80000000u},R{0x80000000u},R{0x3f800000u},R{0x3f800000u},false,{},{},true,
            [&](R v,std::int32_t& p){++calls;return v.promoted().roundPixel(p);},signedZero)||calls!=4||
            signedZero.left.bits||signedZero.top.bits||signedZero.right.bits||signedZero.bottom.bits)return false;
        return true;
    }
};
}
#endif
