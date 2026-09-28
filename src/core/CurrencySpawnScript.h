#ifndef DOWNWELL_CORE_CURRENCY_SPAWN_SCRIPT_H
#define DOWNWELL_CORE_CURRENCY_SPAWN_SCRIPT_H
#include "RunnerRealComparison.h"
namespace downwell {
// Caller fields, not gem state. Handle may be a native lifetime key or a source
// numeric ID; an adapter must not conflate the two identity domains.
template<class Handle> struct CurrencySpawnSource {
    MotionBinary64 x{},y{},moniAmount{},stomped{};
    int objectIndex=0;
    bool atrState=false;
    Handle myGem{};
};
struct CurrencySpawnScript {
    template<class Ops> static bool run(CurrencySpawnSource<typename Ops::Handle>& source,
        MotionBinary64 amount,Ops& ops,bool& good){
        if(!good)return false;
        auto check=[&](bool ok){if(!ok)good=false;return ok;};
        source.moniAmount=amount;source.atrState=false;
        int parent=0,order=0;
        if(!check(ops.parent(source.objectIndex,parent)))return false;
        if(parent==83){
            if(!check(compareRunnerReal(source.stomped,MotionBinary64::fromInteger(1),order)))return false;
            if(order==0)source.atrState=true;
        }
        // These are two distinct VM loops. Once in the twos loop a callback
        // raising moniAmount above ten does not re-enter the tens loop.
        for(unsigned phase=0;phase<2;++phase){
            const int denomination=phase?2:10;
            for(;;){
                if(!check(compareRunnerReal(source.moniAmount,phase?MotionBinary64{}:
                    MotionBinary64::fromInteger(10),order)))return false;
                if(phase?order<=0:order<0)break;
                typename Ops::Handle child{};
                if(!check(ops.create(248,source.x,source.y,child)))return false;
                source.myGem=child;
                // The with environment retains its selected child through
                // both writes, even if callbacks mutate the caller's myGem.
                if(!check(ops.amount(child,denomination)))return false;
                if(!phase&&!check(ops.size(child,1)))return false;
                if(source.atrState&&!check(ops.attracted(source.myGem,true)))return false;
                if(!check(source.moniAmount.add(MotionBinary64::fromInteger(-denomination))))return false;
            }
        }
        return true;
    }
};
}
#endif
