#ifndef DOWNWELL_CORE_SKELETON_EFFECT_BURSTS_H
#define DOWNWELL_CORE_SKELETON_EFFECT_BURSTS_H
#include "RandomBinary64.h"
namespace downwell {
// The skeleton's scrSmokefx(xx,yy,5,1) and scrFlashballfx(xx,yy,6,2,10).
// create(object,x,y) must complete the child's Create, including its RNG,
// synchronously. Positions are VM reals; the instance owner applies builtin
// coordinate narrowing. Failure stops without undoing earlier births/draws.
template<class Random,class Create>
bool skeletonSmokeBurst(MotionBinary64 x,MotionBinary64 y,Random& random,Create create) {
    const unsigned count=4+random.irandomSmall(2);
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
template<class Random,class Create>
bool skeletonFlashBurst(MotionBinary64 x,MotionBinary64 y,Random& random,Create create) {
    const unsigned count=4+random.irandomSmall(4);
    for(unsigned i=0;i<count;++i) {
        auto py=y,px=x;
        MotionBinary64 dy,dx;
        // random_range(-10,10): multiply by 20, add -10, then add position.
        if(!randomBinary64(random.nextU32(),MotionBinary64::fromInteger(20),dy) ||
           !dy.add(MotionBinary64::fromInteger(-10)) || !py.add(dy)) return false;
        if(!randomBinary64(random.nextU32(),MotionBinary64::fromInteger(20),dx) ||
           !dx.add(MotionBinary64::fromInteger(-10)) || !px.add(dx)) return false;
        if(!create(224,px,py)) return false;
    }
    return true;
}
}
#endif
