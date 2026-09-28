#include "core/GameplayTimeChangeRegistration.h"
using namespace downwell;
namespace {
struct Owner {
    std::uint32_t next=105187;unsigned births=0,marks=0,retires=0;
    bool bad=false;
    template<class R> bool activation(InstanceHandle h,const R& r,bool){return h.generation==r.lifetimeId&&!bad;}
    template<class R> bool birth(InstanceHandle h,const R& r,std::uint32_t& id){
        if(!h.generation||r.objectId!=211||r.x.bits!=0x43704000u||r.y.bits!=0x43308000u||
           r.alarms.get(0)!=-1||r.alarms.get(1)!=-1)return false;
        ++births;id=next++;return !bad;
    }
    template<class R> bool markDestroyed(InstanceHandle h,const R& r){
        if(h.generation!=r.lifetimeId||r.destroyRequested)return false;
        ++marks;return true;
    }
    template<class R> bool retire(InstanceHandle h,const R& r){
        if(h.generation!=r.lifetimeId)return false;
        ++retires;return true;
    }
};
}
// Also compiled directly with -m68000: no host heap, float instructions or
// filesystem. Runtime invocation remains a separate verification gate.
bool nativeTimeChangeRegistrationProbe(){
    Owner owner;GameplayTimeChangeRegistration<2,Owner> registry(owner);
    TimeChangeEffects<2> effects;int speed=60;unsigned pauses=0;
    if(!effects.bind(registry))return false;
    auto create=[&]{return effects.createAt(RunnerImageValue{0x43704000u}.promoted(),
        RunnerImageValue{0x43308000u}.promoted(),speed,[&](unsigned ms){
            const auto* r=registry.find(registry.handle(owner.births));
            ++pauses;return ms==30&&speed==30&&r&&r->alarms.get(0)==2&&r->alarms.get(1)==2;
        });};
    if(!create()||effects.sourceId(1)!=105187||registry.size()!=1)return false;
    const auto first=registry.handle(1);
    auto* r=registry.schedulerRecord(first);if(!r)return false;
    if(!registry.activation(first,false))return false;
    bool eligible=true;
    if(!registry.eventEligible(1,105187,eligible)||eligible)return false;
    if(!registry.activation(first,true))return false;
    // Global per-index decrement/dispatch, not the owner's standalone clock.
    for(unsigned tick=1;tick<=14;++tick){
        for(unsigned index=0;index<2;++index)
            if(r->alarms.advanceOne(index)&&!registry.dispatchAlarm(first,effects,index,speed))return false;
        if(speed!=int(tick<12?30+5*(tick/2):60))return false;
    }
    if(effects.size()!=0||registry.size()!=1||!r->destroyRequested||owner.marks!=1||owner.retires)return false;
    if(!registry.retireMarked(first,effects)||registry.find(first)||registry.size()||owner.retires!=1)return false;
    if(!create())return false;
    const auto second=registry.handle(2);
    if(second.slot!=first.slot||second.generation==first.generation||registry.find(first)||
       effects.sourceId(2)!=105188||pauses!=2)return false;
    // An old phase/identity must not alias a reused native slot.
    if(!effects.clear()||registry.size()||registry.find(second)||owner.retires!=2)return false;
    return registry.valid()&&effects.succeeded();
}
#ifndef DOWNWELL_NATIVE_QUERY_PROBE
#include <cassert>
#include <cstdio>
int main(){
    assert(nativeTimeChangeRegistrationProbe());
    for(unsigned fault=0;fault<3;++fault){
        Owner owner;
        if(fault==0)owner.bad=true;
        if(fault==1)owner.next=0;
        if(fault==2)owner.next=0x80000000u;
        GameplayTimeChangeRegistration<1,Owner> registry(owner);TimeChangeEffects<1> effects;
        int speed=60;unsigned pauses=0;
        assert(effects.bind(registry));
        assert(!effects.createAt(RunnerImageValue{0x43704000u}.promoted(),
            RunnerImageValue{0x43308000u}.promoted(),speed,[&](unsigned){++pauses;return true;}));
        assert(!registry.valid()&&!effects.succeeded()&&registry.size()==1&&owner.births==1);
        assert(speed==60&&pauses==0&&owner.marks==0&&owner.retires==0);
        assert(!effects.createAt({},{},speed,[](unsigned){return true;})&&owner.births==1);
    }
    std::puts("Native time-change registration: positioned birth, numeric IDs, alarms, retained destruction, stale reuse and failure passed");
}
#endif
