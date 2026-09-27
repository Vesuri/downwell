#ifndef DOWNWELL_CORE_SKELETON_EFFECT_BURSTS_H
#define DOWNWELL_CORE_SKELETON_EFFECT_BURSTS_H
#include "RandomBinary64.h"
namespace downwell {
// Shared constant-argument scrSmokefx and scrFlashballfx calls. The skeleton
// uses (5,1)/(6,2,10); box destruction uses (1,0)/(1,0,2).
// create(object,x,y) must complete the child's Create, including its RNG,
// synchronously. Positions are VM reals; the instance owner applies builtin
// coordinate narrowing. Failure stops without undoing earlier births/draws.
template<unsigned Count,unsigned Variation,class Random,class Create>
bool smokeBurstScript(MotionBinary64 x,MotionBinary64 y,Random& random,Create create) {
    static_assert(Count>=Variation&&Variation<=32767&&Count<=32767,"admitted integer burst count");
    // Even irandom_range(0,0) consumes both original RNG words.
    const unsigned count=Count-Variation+random.irandomSmall(Variation*2);
    for(unsigned i=0;i<count;++i) {
        auto py=y,px=x;
        const int dy=int(random.chooseIndexSmall(5))-2;
        if(!py.add(MotionBinary64::fromInteger(dy))) return false;
        const int dx=int(random.chooseIndexSmall(5))-2;
        if(!px.add(MotionBinary64::fromInteger(dx))) return false;
        if(!create(225,px,py)) return false;
    }
    return true;
}
template<unsigned Count,unsigned Variation,unsigned Radius,class Random,class Create>
bool flashBurstScript(MotionBinary64 x,MotionBinary64 y,Random& random,Create create) {
    static_assert(Count>=Variation&&Variation<=32767&&Count<=32767&&Radius<=32767,"admitted integer burst arguments");
    const unsigned count=Count-Variation+random.irandomSmall(Variation*2);
    for(unsigned i=0;i<count;++i) {
        auto py=y,px=x;
        MotionBinary64 dy,dx;
        if(!randomBinary64(random.nextU32(),MotionBinary64::fromInteger(int(Radius)*2),dy) ||
           !dy.add(MotionBinary64::fromInteger(-int(Radius))) || !py.add(dy)) return false;
        if(!randomBinary64(random.nextU32(),MotionBinary64::fromInteger(int(Radius)*2),dx) ||
           !dx.add(MotionBinary64::fromInteger(-int(Radius))) || !px.add(dx)) return false;
        if(!create(224,px,py)) return false;
    }
    return true;
}
template<class Random,class Create>
bool skeletonSmokeBurst(MotionBinary64 x,MotionBinary64 y,Random& random,Create create) {
    return smokeBurstScript<5,1>(x,y,random,create);
}
template<class Random,class Create>
bool skeletonFlashBurst(MotionBinary64 x,MotionBinary64 y,Random& random,Create create) {
    return flashBurstScript<6,2,10>(x,y,random,create);
}
template<class Random,class Create>
bool boxSmokeBurst(MotionBinary64 x,MotionBinary64 y,Random& random,Create create) {
    return smokeBurstScript<1,0>(x,y,random,create);
}
template<class Random,class Create>
bool boxFlashBurst(MotionBinary64 x,MotionBinary64 y,Random& random,Create create) {
    return flashBurstScript<1,0,2>(x,y,random,create);
}
}
#endif
