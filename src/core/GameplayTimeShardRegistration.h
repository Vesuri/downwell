#ifndef DOWNWELL_CORE_GAMEPLAY_TIME_SHARD_REGISTRATION_H
#define DOWNWELL_CORE_GAMEPLAY_TIME_SHARD_REGISTRATION_H
#include "TimeShardEffects.h"
#include "InstanceHandle.h"
#include "FailureSignal.h"
namespace downwell {
// Fixed-address native registration. Owner supplies resource-backed lifetime,
// population, animation membership and collision publication, including real
// numeric IDs. Local generations never substitute for those IDs.
template<unsigned Capacity,class Owner> class GameplayTimeShardRegistration {
    static_assert(Capacity>0,"Shard registration must be nonempty");
public:
    struct Record {
        std::uint32_t lifetimeId=0,sourceId=0;
        RunnerImageValue x{},y{},imageIndex{},imageSpeed{};
        AlarmBank alarms;
        bool active=true,destroyRequested=false;
        static constexpr int objectId=238;
    };
    explicit GameplayTimeShardRegistration(Owner& owner):owner_(owner){}
    GameplayTimeShardRegistration(const GameplayTimeShardRegistration&)=delete;
    GameplayTimeShardRegistration& operator=(const GameplayTimeShardRegistration&)=delete;
    bool valid()const{return good_;}
    void observeFailure(bool& owner){good_.observe(owner);}
    unsigned size()const{return count_;} // Includes marks awaiting ordered cleanup.
    const Record* find(InstanceHandle key)const{
        return key.generation&&key.slot<Capacity&&records_[key.slot].lifetimeId==key.generation?&records_[key.slot]:nullptr;
    }
    InstanceHandle handle(std::uint32_t local)const{
        for(unsigned i=0;local&&i<Capacity;++i)if(records_[i].lifetimeId==local)return {i,local};
        return {};
    }
    Record* schedulerRecord(InstanceHandle key){return good_?const_cast<Record*>(find(key)):nullptr;}
    bool birth(std::uint32_t local,RunnerImageValue x,RunnerImageValue y,std::uint32_t& id){
        if(!good_||!local||local<=newest_||!SequenceTimeline::valid(x)||!SequenceTimeline::valid(y))return fail();
        unsigned i=0;while(i<Capacity&&records_[i].lifetimeId)++i;
        if(i==Capacity)return fail();
        auto& r=records_[i];r={};r.lifetimeId=local;r.x=x;r.y=y;newest_=local;++count_;
        std::uint32_t source=0;const bool born=owner_.birth({i,local},r,source);
        r.sourceId=source;id=source;
        if(!born||!source||source>0x7fffffffu)return fail();
        for(unsigned j=0;j<Capacity;++j)if(j!=i&&records_[j].lifetimeId&&records_[j].sourceId==source)return fail();
        return true;
    }
    bool created(std::uint32_t local,std::uint32_t id,const TimeShardAnimation& effect){
        auto* r=resolve(local,id);if(!r)return fail();
        r->imageSpeed=effect.imageSpeed;
        return publishAnimation(local,id,effect)&&publishAlarm(local,id,effect);
    }
    bool population(unsigned& count){return good_&&(owner_.population(count)||fail());}
    bool eventEligible(std::uint32_t local,std::uint32_t id,bool& eligible){
        const auto* r=resolve(local,id);if(!r)return fail();
        eligible=r->active&&!r->destroyRequested;return true;
    }
    bool activation(InstanceHandle key,bool active){
        auto* r=schedulerRecord(key);
        if(!r||r->destroyRequested||!owner_.activation(key,*r,active))return fail();
        r->active=active;return true;
    }
    bool animationEligible(std::uint32_t local,std::uint32_t id,bool& eligible){
        const auto* r=resolve(local,id);
        return r&&(owner_.animationMember(handle(local),*r,eligible)||fail());
    }
    bool publishAnimation(std::uint32_t local,std::uint32_t id,const TimeShardAnimation& effect){
        auto* r=resolve(local,id);
        if(!r||!owner_.imageIndex(handle(local),*r,effect.imageIndex))return fail();
        r->imageIndex=effect.imageIndex;return true;
    }
    bool publishAlarm(std::uint32_t local,std::uint32_t id,const TimeShardAnimation& effect){
        auto* r=resolve(local,id);if(!r)return fail();
        r->alarms.set(0,effect.alarm0);r->alarms.set(1,effect.alarm1);r->alarms.set(2,effect.alarm2);return true;
    }
    bool publishStep(std::uint32_t local,std::uint32_t id,const TimeShardAnimation& effect){
        auto* r=resolve(local,id);
        // Even a destroyed event completes its final position setters.
        if(!r||!owner_.position(handle(local),*r,effect.x,effect.y))return fail();
        r->x=effect.x;r->y=effect.y;r->imageSpeed=effect.imageSpeed;
        return publishAlarm(local,id,effect);
    }
    bool destroyed(std::uint32_t local,std::uint32_t id,const TimeShardAnimation&){
        auto* r=resolve(local,id);
        if(!r||!owner_.markDestroyed(handle(local),*r))return fail();
        r->destroyRequested=true;return true;
    }
    bool retire(std::uint32_t local,std::uint32_t id,const TimeShardAnimation&){
        auto* r=resolve(local,id);if(!r||!owner_.retire(handle(local),*r))return fail();
        *r={};--count_;return true;
    }
    template<unsigned N> bool dispatchAnimation(InstanceHandle key,TimeShardEffects<N>& effects){
        const auto* r=find(key);
        if(!good_||!r||!effects.phasesBoundTo(*this))return fail();
        return effects.advanceAnimationFor(key.generation,r->sourceId)||fail();
    }
    template<unsigned N> bool dispatchAlarm(InstanceHandle key,TimeShardEffects<N>& effects,unsigned alarm){
        const auto* r=find(key);
        if(!good_||!r||!effects.phasesBoundTo(*this))return fail();
        // Enclosing room scheduler has already decremented the real counters.
        return effects.dispatchAlarm(key.generation,r->sourceId,r->alarms,alarm)||fail();
    }
    template<unsigned N,class Random,class Outside>
    bool dispatchStep(InstanceHandle key,TimeShardEffects<N>& effects,bool stopped,Random& random,Outside outside){
        const auto* r=find(key);
        if(!good_||!r||!effects.phasesBoundTo(*this))return fail();
        return effects.stepFor(key.generation,r->sourceId,r->alarms,stopped,random,
            [&](auto,const auto&,auto x,auto y,bool& hit){return owner_.meeting(key,*r,x,y,hit);},
            [&](auto,const auto& effect,bool& retire){return outside(effect,retire);})||fail();
    }
    template<unsigned N,class Random>
    bool dispatchContact(InstanceHandle key,TimeShardEffects<N>& effects,MotionBinary64 xsp,MotionBinary64 ysp,Random& random){
        const auto* r=find(key);
        if(!good_||!r||!effects.phasesBoundTo(*this)||effects.sourceId(key.generation)!=r->sourceId||
           !effects.get(key.generation))return fail();
        return effects.playerContact(key.generation,xsp,ysp,random,&r->alarms)||fail();
    }
    template<unsigned N> bool retireMarked(InstanceHandle key,TimeShardEffects<N>& effects){
        const auto* r=find(key);
        if(!good_||!r||!r->destroyRequested||!effects.phasesBoundTo(*this))return fail();
        return effects.retireMarked(key.generation,r->sourceId)||fail();
    }
private:
    Record* resolve(std::uint32_t local,std::uint32_t id){
        auto* r=schedulerRecord(handle(local));
        if(!r||r->sourceId!=id){fail();return nullptr;}return r;
    }
    bool fail(){good_=false;return false;}
    Owner& owner_;Record records_[Capacity]{};unsigned count_=0;std::uint32_t newest_=0;FailureSignal<> good_=true;
};
}
#endif
