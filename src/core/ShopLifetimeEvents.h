#ifndef DOWNWELL_CORE_SHOP_LIFETIME_EVENTS_H
#define DOWNWELL_CORE_SHOP_LIFETIME_EVENTS_H
#include "RunnerRealComparison.h"
namespace downwell {
struct ShopLifetimeEvents {
    template<class Ops> static bool roomEnd(Ops& ops){
        bool playing;
        return ops.isPlaying(playing)&&(!playing||ops.stop());
    }
    template<class Ops> static bool endStep(MotionBinary64 y,Ops& ops){
        MotionBinary64 view;int order;
        if(!ops.viewY(view)||!view.add(MotionBinary64::fromInteger(-320))||
           !compareRunnerReal(y,view,order))return false;
        if(order>=0){
            // The second ___view_get is a fresh source read, not a snapshot.
            if(!ops.viewY(view)||!view.add(MotionBinary64::fromInteger(3000))||
               !compareRunnerReal(y,view,order))return false;
            if(order<=0)return true;
        }
        int room;if(!ops.room(room))return false;
        return room!=11||(roomEnd(ops)&&ops.destroy());
    }
};
}
#endif
