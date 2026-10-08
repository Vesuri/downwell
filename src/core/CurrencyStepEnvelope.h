#ifndef DOWNWELL_CORE_CURRENCY_STEP_ENVELOPE_H
#define DOWNWELL_CORE_CURRENCY_STEP_ENVELOPE_H
#include "CurrencyCreate.h"
#include "RunnerRealComparison.h"
namespace downwell {
// objCurrency ordinary Step around its two movement branches. Both movement
// operations are required; this is not an alternative approximate gem mover.
struct CurrencyStepEnvelope {
    template<class Random,class Ops> static bool run(CurrencyCreate& s,Random& random,Ops& ops,bool& good){
        if(!good)return false;
        auto check=[&](bool ok){if(!ok)good=false;return ok;};
        s.begin(s.size,random);
        if(!s.attracted){
            bool hit=false;
            if(!check(ops.wall(s.x.promoted(),s.y.promoted(),hit)))return false;
            if(hit){if(++s.stuckKamo>2&&!check(ops.destroy()))return false;}
            else s.stuckKamo=0;
        }
        // Destruction is deferred. The source continues into time/motion and
        // publishes the final coordinates even after the stuck-wall mark.
        if(!s.attracted){
            bool allowed=false;if(!check(ops.timeAllowed(allowed)))return false;
            if(allowed){
                int order=0;if(!check(compareRunnerReal(s.imageSpeed.promoted(),s.imgSp,order)))return false;
                if(order&&!check(s.imageSpeed.assign(s.imgSp)))return false;
                if((!s.attracted||!s.suckable)&&!check(ops.freeMotion(s)))return false;
            }else{
                s.imageSpeed={};s.scriptIndex=0;
                for(;s.scriptIndex<=3;++s.scriptIndex){
                    const int alarm=s.alarms.get(s.scriptIndex);
                    if(alarm>0){if(alarm==0x7fffffff)return check(false);s.alarms.set(s.scriptIndex,alarm+1);}
                }
                s.obtainable=true;
            }
        }else if(s.attracted){if(!check(ops.attractedMotion(s)))return false;}
        if(s.disappearing)s.flash=-s.flash;
        std::int32_t x,y;
        if(!check(s.xx.roundPixel(x)&&s.x.assignInteger(x)&&ops.publish(s.x,s.y)))return false;
        return check(s.yy.roundPixel(y)&&s.y.assignInteger(y)&&ops.publish(s.x,s.y));
    }
};
}
#endif
