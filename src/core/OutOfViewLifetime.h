#ifndef DOWNWELL_CORE_OUT_OF_VIEW_LIFETIME_H
#define DOWNWELL_CORE_OUT_OF_VIEW_LIFETIME_H
#include "RunnerRealComparison.h"
namespace downwell {
// scrOutofview for real-valued published coordinates. Call only for an
// eligible End Step instance; activation is the scheduler's responsibility.
// Culling is retirement, never enemy death/rewards or an expiration alarm.
inline bool outOfViewRetirement(MotionBinary64 publishedY,MotionBinary64 viewY,
                                bool mainRoom,bool& retire){
    auto edge=viewY;int order;
    if(!edge.add(MotionBinary64::fromInteger(-160))||
       !compareRunnerReal(publishedY,edge,order))return false;
    bool outside=order<0;
    if(!outside){
        edge=viewY;
        if(!edge.add(MotionBinary64::fromInteger(3000))||
           !compareRunnerReal(publishedY,edge,order))return false;
        outside=order>0;
    }
    retire=outside&&mainRoom;return true;
}
}
#endif
