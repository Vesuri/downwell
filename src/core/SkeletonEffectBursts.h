#ifndef DOWNWELL_CORE_SKELETON_EFFECT_BURSTS_H
#define DOWNWELL_CORE_SKELETON_EFFECT_BURSTS_H
#include "RandomBinary64.h"
#include "FixedPoint.h"
namespace downwell {
// The burst scripts take either coordinate type: Fixed16 for the compact
// owners, MotionBinary64 for the registered (world two) owners that still
// keep it (W3.20). burst_detail holds the two arithmetic forms.
namespace burst_detail {
inline bool shift(MotionBinary64& v,int whole){return v.add(MotionBinary64::fromInteger(whole));}
inline bool shift(Fixed16& v,int whole){return v.addInteger(whole);}
// random(radius * 2) - radius, added to v.
template<class Random> bool jitter(MotionBinary64& v,int radius,Random& random){
    MotionBinary64 d;
    return randomBinary64(random.nextU32(),MotionBinary64::fromInteger(radius*2),d)&&
        d.add(MotionBinary64::fromInteger(-radius))&&v.add(d);
}
template<class Random> bool jitter(Fixed16& v,int radius,Random& random){
    if(radius>16383)return false;
    const Fixed16 d=Fixed16::random(random.nextU32(),Fixed16::fromInteger(radius*2));
    const std::int64_t sum=std::int64_t(v.v)-std::int64_t(radius)*65536+d.v;
    if(sum< -0x7fffffffLL-1||sum>0x7fffffffLL)return false;
    v.v=std::int32_t(sum);return true;
}
}
// scrSmokefx and scrFlashballfx share fields on their caller, not their children.
template<class Real> struct SmokeBurstCallerOf {
    Real smx{},smy{};
    int smokenum=0,vary=0,pvary=0;
};
using SmokeBurstCaller=SmokeBurstCallerOf<MotionBinary64>;
template<unsigned Count,unsigned Variation,class Real,class Random,class Create>
bool smokeBurstPublished(SmokeBurstCallerOf<Real>& caller,Real x,Real y,
                         Random& random,Create create){
    static_assert(Count>=Variation&&Variation<=32767&&Count<=32767,"admitted integer burst count");
    caller.smx=x;caller.smy=y;caller.smokenum=Count;caller.vary=Variation;
    caller.smokenum+=int(random.irandomSmall(Variation*2))-int(Variation);
    // The VM repeat count is captured once; smx/smy are re-read for each child.
    const int count=caller.smokenum;
    for(int i=0;i<count;++i){
        auto py=caller.smy,px=caller.smx;
        if(!burst_detail::shift(py,int(random.chooseIndexSmall(5))-2)||
           !burst_detail::shift(px,int(random.chooseIndexSmall(5))-2)||
           !create(225,px,py))return false;
    }
    return true;
}
// Shared constant-argument scrSmokefx and scrFlashballfx calls. The skeleton
// uses (5,1)/(6,2,10); box destruction uses (1,0)/(1,0,2).
// create(object,x,y) must complete the child's Create, including its RNG,
// synchronously. Positions are VM reals; the instance owner applies builtin
// coordinate narrowing. Failure stops without undoing earlier births/draws.
template<unsigned Count,unsigned Variation,class Real,class Random,class Create>
bool smokeBurstScript(Real x,Real y,Random& random,Create create) {
    static_assert(Count>=Variation&&Variation<=32767&&Count<=32767,"admitted integer burst count");
    // Even irandom_range(0,0) consumes both original RNG words.
    const unsigned count=Count-Variation+random.irandomSmall(Variation*2);
    for(unsigned i=0;i<count;++i) {
        auto py=y,px=x;
        if(!burst_detail::shift(py,int(random.chooseIndexSmall(5))-2)) return false;
        if(!burst_detail::shift(px,int(random.chooseIndexSmall(5))-2)) return false;
        if(!create(225,px,py)) return false;
    }
    return true;
}
template<unsigned Count,unsigned Variation,unsigned Radius,class Real,class Random,class Create>
bool flashBurstPublished(SmokeBurstCallerOf<Real>& caller,Real x,Real y,
                         Random& random,Create create) {
    static_assert(Count>=Variation&&Variation<=32767&&Count<=32767&&Radius<=32767,"admitted integer burst arguments");
    caller.smx=x;caller.smy=y;caller.smokenum=Count;caller.vary=Variation;caller.pvary=Radius;
    caller.smokenum+=int(random.irandomSmall(Variation*2))-int(Variation);
    const int count=caller.smokenum;
    for(int i=0;i<count;++i) {
        // VM argument evaluation is Y then X. Synchronous child Create may
        // change these shared fields, but cannot change the captured repeat count.
        auto py=caller.smy,px=caller.smx;
        if(caller.pvary<0 || caller.pvary>32767)return false;
        if(!burst_detail::jitter(py,caller.pvary,random) ||
           !burst_detail::jitter(px,caller.pvary,random)) return false;
        if(!create(224,px,py)) return false;
    }
    return true;
}
template<unsigned Count,unsigned Variation,unsigned Radius,class Real,class Random,class Create>
bool flashBurstScript(Real x,Real y,Random& random,Create create) {
    SmokeBurstCallerOf<Real> caller;
    return flashBurstPublished<Count,Variation,Radius>(caller,x,y,random,create);
}
template<class Real,class Random,class Create>
bool skeletonSmokeBurst(Real x,Real y,Random& random,Create create) {
    return smokeBurstScript<5,1>(x,y,random,create);
}
template<class Real,class Random,class Create>
bool skeletonFlashBurst(Real x,Real y,Random& random,Create create) {
    return flashBurstScript<6,2,10>(x,y,random,create);
}
template<class Real,class Random,class Create>
bool boxSmokeBurst(Real x,Real y,Random& random,Create create) {
    return smokeBurstScript<1,0>(x,y,random,create);
}
template<class Real,class Random,class Create>
bool boxFlashBurst(Real x,Real y,Random& random,Create create) {
    return flashBurstScript<1,0,2>(x,y,random,create);
}
}
#endif
