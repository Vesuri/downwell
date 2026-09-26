#ifndef DOWNWELL_CORE_TIME_STOP_BOUND_H
#define DOWNWELL_CORE_TIME_STOP_BOUND_H
#include "MotionBinary64.h"
namespace downwell {
// TimeStopBound/checkTimeStopArea. Coordinates are the published instance x/y,
// not private motion accumulators. Field queries must use the instance's mask
// (including transforms), against parentTimeField (82), not its centre alone.
struct TimeStopBound {
    template<class Meeting> static bool evaluate(bool stopped,MotionBinary64 x,
        MotionBinary64 y,int roomWidth,Meeting meeting,bool& allowed){
        if(!stopped){allowed=true;return true;}
        if(!x.valid() || !y.valid() || roomWidth<0)return false;
        bool inField=false;
        // The VM calls place_meeting BEFORE either strict side comparison.
        if(!meeting(x,y,inField))return false;
        allowed=inField || less(x,MotionBinary64::fromInteger(160)) ||
            less(MotionBinary64::fromInteger(roomWidth-160),x);
        return true;
    }
private:
    static bool less(MotionBinary64 a,MotionBinary64 b){
        if(a.zero() && b.zero())return false;
        const bool negativeA=(a.high>>31)!=0,negativeB=(b.high>>31)!=0;
        if(negativeA!=negativeB)return negativeA;
        const auto ah=a.high&0x7fffffffu,bh=b.high&0x7fffffffu;
        if(ah==bh && a.low==b.low)return false;
        const bool magnitudeLess=ah<bh || (ah==bh && a.low<b.low);
        return negativeA?!magnitudeLess:magnitudeLess;
    }
};
}
#endif
