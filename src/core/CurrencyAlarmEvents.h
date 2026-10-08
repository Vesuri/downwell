#ifndef DOWNWELL_CORE_CURRENCY_ALARM_EVENTS_H
#define DOWNWELL_CORE_CURRENCY_ALARM_EVENTS_H
#include "CurrencyCreate.h"
namespace downwell {
struct CurrencyAlarmEvents {
    // Countdown belongs to the global alarm pass, not this event body.
    template<class Other> static bool run(CurrencyCreate& state,unsigned index,Other other,bool& good){
        if(!good)return false;
        if(index>=5||state.alarms.get(index)!=0)return good=false;
        if(index==0){state.obtainable=true;state.alarms.set(1,state.obtime);return true;}
        if(index==1){state.disappearing=true;state.alarms.set(2,state.distime);return true;}
        if(index==3){state.suckable=true;return true;}
        return good=other(state,index,good)&&good;
    }
};
}
#endif
