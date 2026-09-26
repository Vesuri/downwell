#include "core/TimeChangeEffects.h"
#include <cassert>
#include <cstdio>
using namespace downwell;
int main(){
    TimeChangeEffects<3> owner;int speed=60;unsigned pauses=0;
    auto pause=[&](unsigned ms){assert(ms==30 && speed==30);++pauses;return true;};
    const auto empty=owner.beginPhase();assert(owner.create(speed,pause));
    for(unsigned i=0;i<4;++i)owner.advanceAlarm(empty,1,speed);
    assert(speed==30 && owner.size()==1);
    for(unsigned tick=1;tick<=14;++tick){
        const auto phase=owner.beginPhase();owner.advanceAlarm(phase,0,speed);owner.advanceAlarm(phase,1,speed);
        assert(speed==int(tick<12?30+5*(tick/2):60));assert(owner.size()==unsigned(tick<14));
    }
    assert(owner.create(speed,pause) && owner.create(speed,pause));
    for(unsigned tick=1;tick<=8;++tick){
        const auto phase=owner.beginPhase();owner.advanceAlarm(phase,0,speed);owner.advanceAlarm(phase,1,speed);
        assert(speed==int(tick<6?30+10*(tick/2):60));
    }
    assert(owner.size()==0);
    assert(owner.create(speed,pause) && owner.create(speed,pause) && owner.create(speed,pause));
    assert(!owner.create(speed,pause) && pauses==6);
    const auto stale=owner.beginPhase();owner.clear();assert(owner.create(speed,pause));
    owner.advanceAlarm(stale,1,speed);owner.advanceAlarm(stale,1,speed);assert(speed==30);
    const auto phase=owner.beginPhase();owner.advanceAlarm(phase,1,speed);owner.advanceAlarm(phase,1,speed);assert(speed==35);
    owner.clear();assert(!owner.create(speed,[](unsigned){return false;}));assert(owner.size()==1 && speed==30);
    std::puts("Time-change ownership: single/overlapping ramps, phase cutoffs, capacity, reset and partial failure passed");
}
