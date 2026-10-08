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
    // Live storage supplies generation-checked reads and individual writes.
    // Never hold its record pointer across synchronous child Create/setters.
    template<class Read,class WriteAmount,class WriteAttracted,class WriteChild,class Ops>
    static bool runRetained(Read read,WriteAmount writeAmount,WriteAttracted writeAttracted,
        WriteChild writeChild,MotionBinary64 amount,Ops& ops,bool& good){
        if(!good)return false;
        auto check=[&](bool ok){if(!ok)good=false;return ok;};
        CurrencySpawnSource<typename Ops::Handle> source;
        auto refresh=[&](){return check(read(source));};
        if(!check(writeAmount(amount))||!check(writeAttracted(false))||!refresh())return false;
        int parent=0,order=0;
        if(!check(ops.parent(source.objectIndex,parent)))return false;
        if(parent==83){
            if(!refresh())return false;
            if(!check(compareRunnerReal(source.stomped,MotionBinary64::fromInteger(1),order)))return false;
            if(order==0&&!check(writeAttracted(true)))return false;
        }
        // These are two distinct VM loops. Once in the twos loop a callback
        // raising moniAmount above ten does not re-enter the tens loop.
        for(unsigned phase=0;phase<2;++phase){
            const int denomination=phase?2:10;
            for(;;){
                if(!refresh())return false;
                if(!check(compareRunnerReal(source.moniAmount,phase?MotionBinary64{}:
                    MotionBinary64::fromInteger(10),order)))return false;
                if(phase?order<=0:order<0)break;
                typename Ops::Handle child{};
                if(!check(ops.create(248,source.x,source.y,child)))return false;
                if(!check(writeChild(child)))return false;
                // The with environment retains its selected child through
                // both writes, even if callbacks mutate the caller's myGem.
                if(!check(ops.amount(child,denomination)))return false;
                if(!phase&&!check(ops.size(child,1)))return false;
                if(!refresh())return false;
                if(source.atrState&&!check(ops.attracted(source.myGem,true)))return false;
                if(!refresh()||!check(source.moniAmount.add(MotionBinary64::fromInteger(-denomination)))||
                   !check(writeAmount(source.moniAmount)))return false;
            }
        }
        return true;
    }
    template<class Ops> static bool run(CurrencySpawnSource<typename Ops::Handle>& source,
        MotionBinary64 amount,Ops& ops,bool& good){
        return runRetained([&](auto& value){value=source;return true;},
            [&](MotionBinary64 value){source.moniAmount=value;return true;},
            [&](bool value){source.atrState=value;return true;},
            [&](typename Ops::Handle value){source.myGem=value;return true;},amount,ops,good);
    }
};
}
#endif
