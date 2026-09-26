#ifndef DOWNWELL_CORE_DEAD_BODY_CREATION_H
#define DOWNWELL_CORE_DEAD_BODY_CREATION_H
#include "RunnerRealComparison.h"
#include "MultiplyBinary64.h"
namespace downwell {
// scrDeadBody caller fields, not corpse state. In particular the speed fallback
// writes the dying enemy's xsp. Adapters must publish these source mutations.
struct DeadBodySource {
    MotionBinary64 x{},y{},xsp{},imageXscale{},impactSp{};
    int bodySprite=0;
    std::uint32_t myDeadBody=0;
};
// create completes inherited/child Create and relocation synchronously before
// any overrides. Setters publish through the live world's normal mutation
// hooks (sprite assignment can reset animation/collision state). Callbacks may
// modify source fields: the script re-reads them after each call. Failure is
// terminal; retain earlier changes and do not retry or replay RNG draws.
template<class Events>
bool createDeadBody(DeadBodySource& source,int sprite,MotionBinary64 impact,
                    int explosion,Events& events){
    source.bodySprite=sprite;source.impactSp=impact;
    int order;
    if(!compareRunnerReal(source.impactSp,MotionBinary64::fromInteger(4),order))return false;
    if(order>0)source.impactSp=MotionBinary64::fromInteger(4);
    std::uint32_t child=0;
    if(!events.create(263,source.x,source.y,child)||!child)return false;
    source.myDeadBody=child;
    if(!events.sprite(source.myDeadBody,source.bodySprite)||
       !events.verticalSpeed(source.myDeadBody,source.impactSp)||
       !events.facing(source.myDeadBody,source.imageXscale)||
       !events.explosion(source.myDeadBody,explosion))return false;
    auto absolute=source.xsp;absolute.high&=0x7fffffffu;
    if(!compareRunnerReal(absolute,{0x3fe00000u,0},order))return false;
    if(order<0 && !multiplyBinary64(source.imageXscale,{0x3ff80000u,0},source.xsp))return false;
    if(!compareRunnerReal(source.impactSp,{},order))return false;
    MotionBinary64 speed;
    if(order){
        if(!multiplyBinary64(source.xsp,source.impactSp,speed))return false;
    }else{
        if(!source.xsp.valid())return false;
        speed=source.xsp.zero()?MotionBinary64{}:
            MotionBinary64{0x3fe00000u|(source.xsp.high&0x80000000u),0};
    }
    return events.horizontalSpeed(source.myDeadBody,speed);
}
}
#endif
