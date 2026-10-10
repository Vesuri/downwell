#ifndef DOWNWELL_CORE_SHOP_RADIO_EVENTS_H
#define DOWNWELL_CORE_SHOP_RADIO_EVENTS_H
#include "RunnerRealComparison.h"
namespace downwell {
// Blocks 1..16 inside Shop Step's with(Shop), not an independent audio tick.
struct ShopRadioEvents {
    template<class Ops> static bool step(bool& enabled,Ops& ops){
        bool stopped;if(!ops.timeStop(stopped))return false;
        if(!stopped){
            if(enabled){if(!ops.gain(MotionBinary64{},0))return false;enabled=false;}
            return true;
        }
        bool dead;if(!ops.death(dead))return false;
        if(dead)return true;
        if(!enabled){
            MotionBinary64 distance;int order;
            if(!ops.distance(distance)||!compareRunnerReal(distance,MotionBinary64::fromInteger(160),order))return false;
            if(order<0)enabled=true;
        }
        if(!enabled)return true;
        bool visible;if(!ops.inView(visible))return false;
        if(!ops.gain(visible?MotionBinary64::fromInteger(1):MotionBinary64{0x3fd33333u,0x33333333u},100))return false;
        bool muted;if(!ops.noBgm(muted))return false;
        if(muted)return ops.gain(MotionBinary64{},100);
        bool paused;return ops.paused(paused)&&(!paused||ops.resume());
    }
};
}
#endif
