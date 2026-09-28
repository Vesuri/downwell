#ifndef DOWNWELL_CORE_GAMEPLAY_TIME_CHANGE_REGISTRATION_H
#define DOWNWELL_CORE_GAMEPLAY_TIME_CHANGE_REGISTRATION_H
#include "InstanceHandle.h"
#include "TimeChangeEffects.h"
#include "SequenceTimeline.h"
namespace downwell {
// Integer-only registration for object 211. The enclosing native world supplies
// resource-backed birth/destruction/retirement and ORIGINAL numeric IDs. A
// family-local handle must never masquerade as a Windows instance ID.
// Borrowed, fixed-address storage; the owner must outlive this registration.
template<unsigned Capacity,class Owner> class GameplayTimeChangeRegistration {
    static_assert(Capacity>0,"Time-change registration must be nonempty");
public:
    struct Record {
        std::uint32_t lifetimeId=0,sourceId=0;
        RunnerImageValue x{},y{};
        AlarmBank alarms;
        bool active=true,destroyRequested=false;
        static constexpr int objectId=211;
    };
    explicit GameplayTimeChangeRegistration(Owner& owner):owner_(owner){}
    GameplayTimeChangeRegistration(const GameplayTimeChangeRegistration&)=delete;
    GameplayTimeChangeRegistration& operator=(const GameplayTimeChangeRegistration&)=delete;
    bool valid()const{return good_;}
    unsigned size()const{return count_;} // Includes deferred destroyed records.
    const Record* find(InstanceHandle key)const{
        return key.generation&&key.slot<Capacity&&records_[key.slot].lifetimeId==key.generation?
            &records_[key.slot]:nullptr;
    }
    InstanceHandle handle(std::uint32_t local)const{
        for(unsigned i=0;local&&i<Capacity;++i)
            if(records_[i].lifetimeId==local)return {i,local};
        return {};
    }
    bool birth(std::uint32_t local,RunnerImageValue x,RunnerImageValue y,std::uint32_t& id){
        if(!good_||!local||local<=newest_||!SequenceTimeline::valid(x)||!SequenceTimeline::valid(y))return fail();
        unsigned i=0;while(i<Capacity&&records_[i].lifetimeId)++i;
        if(i==Capacity)return fail();
        records_[i]={};auto& r=records_[i];r.lifetimeId=local;r.x=x;r.y=y;
        newest_=local;++count_;
        // Publish the pre-Create record before the callback. Never invent an
        // identity if the resource-backed world cannot supply it.
        std::uint32_t source=0;
        const bool born=owner_.birth({i,local},r,source);
        r.sourceId=source;id=source;
        if(!born)return fail();
        if(!source||source>0x7fffffffu)return fail();
        for(unsigned j=0;j<Capacity;++j)
            if(j!=i&&records_[j].lifetimeId&&records_[j].sourceId==source)return fail();
        return true;
    }
    bool publish(std::uint32_t local,std::uint32_t id,const TimeChangeEffect& effect){
        auto* r=resolve(local,id);if(!r)return fail();
        r->alarms.set(0,effect.alarm0);r->alarms.set(1,effect.alarm1);return true;
    }
    bool eventEligible(std::uint32_t local,std::uint32_t id,bool& eligible){
        const auto* r=resolve(local,id);if(!r)return fail();
        eligible=r->active&&!r->destroyRequested;return true;
    }
    bool destroyed(std::uint32_t local,std::uint32_t id,const TimeChangeEffect& effect){
        auto* r=resolve(local,id);
        if(!r||r->destroyRequested||!effect.destroyRequested||
           !owner_.markDestroyed(handle(local),*r))return fail();
        r->destroyRequested=true;return true;
    }
    bool retire(std::uint32_t local,std::uint32_t id,const TimeChangeEffect&){
        auto* r=resolve(local,id);if(!r||!owner_.retire(handle(local),*r))return fail();
        *r={};--count_;return true;
    }
    template<unsigned N> bool dispatchAlarm(InstanceHandle key,TimeChangeEffects<N>& effects,
                                         unsigned index,int& roomSpeed){
        const auto* r=find(key);
        if(!good_||!r||!effects.boundTo(*this)||index>1)return fail();
        // The enclosing mixed-family scheduler decrements this authoritative
        // counter before dispatch. No second family-specific decrement here.
        return effects.dispatchAlarm(key.generation,r->sourceId,r->alarms,index,roomSpeed)||fail();
    }
    template<unsigned N> bool retireMarked(InstanceHandle key,TimeChangeEffects<N>& effects){
        const auto* r=find(key);
        if(!good_||!r||!r->destroyRequested||!effects.boundTo(*this))return fail();
        return effects.retireMarked(key.generation,r->sourceId)||fail();
    }
    // The world's scheduler uses this only after resolving the current handle.
    // Activation reconciliation is likewise owned by the enclosing world.
    Record* schedulerRecord(InstanceHandle key){return good_?const_cast<Record*>(find(key)):nullptr;}
private:
    Record* resolve(std::uint32_t local,std::uint32_t id){
        auto* r=schedulerRecord(handle(local));return r&&r->sourceId==id?r:nullptr;
    }
    bool fail(){good_=false;return false;}
    Owner& owner_;Record records_[Capacity]{};unsigned count_=0;std::uint32_t newest_=0;bool good_=true;
};
}
#endif
