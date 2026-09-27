#ifndef DOWNWELL_GAMEPLAY_DEBRIS_INSTANCES_H
#define DOWNWELL_GAMEPLAY_DEBRIS_INSTANCES_H
#include "InstanceLifecycle.h"
#include "DebrisEffects.h"
namespace downwell {
// Borrow stable particle storage. The room must retire these records before
// either the effects owner or its registry is destroyed. Never copy a registry.
class GameplayDebrisInstances {
public:
    static constexpr unsigned capacity=64;
    struct Record {const DebrisAnimation* effect=nullptr;unsigned lifetimeId=0;bool active=true,destroyRequested=false;};
    using Binding=RecordLifecycleBinding<Record>;
    explicit GameplayDebrisInstances(Binding binding):binding_(binding){}
    GameplayDebrisInstances(const GameplayDebrisInstances&)=delete;
    GameplayDebrisInstances& operator=(const GameplayDebrisInstances&)=delete;
    bool valid()const{return good_;}
    unsigned size()const{return count_;}
    const Record* find(InstanceHandle h)const{
        return h.generation&&h.slot<capacity&&records_[h.slot].lifetimeId==h.generation?&records_[h.slot]:nullptr;
    }
    InstanceHandle handle(unsigned local)const{
        for(unsigned i=0;local&&i<capacity;++i)if(records_[i].lifetimeId==local)return {i,local};
        return {};
    }
    bool birth(unsigned local,const DebrisAnimation& effect){
        if(!good_||!binding_.hasOwner()||!local||local<=newest_)return good_=false;
        unsigned i=0;while(i<capacity&&records_[i].lifetimeId)++i;
        if(i==capacity)return good_=false;
        records_[i]={&effect,local,true,false};newest_=local;++count_;
        // Create has already completed in stable storage; birth publishes its
        // final position, sprite and image through the resource-backed owner.
        return good_=binding_.birth({i,local},records_[i]);
    }
    bool image(unsigned local,const DebrisAnimation& effect){
        const auto h=handle(local);const auto* r=find(h);
        return good_=check(local,effect)&&binding_.imageIndex(h,*r,effect.imageIndex);
    }
    bool move(unsigned local,const DebrisAnimation& effect,RunnerImageValue x,RunnerImageValue y){
        const auto h=handle(local);const auto* r=find(h);
        return good_=check(local,effect)&&binding_.publishedPosition(h,*r,x,y);
    }
    bool destroyed(unsigned local,const DebrisAnimation& effect){
        const auto h=handle(local);const auto* r=find(h);
        if(!check(local,effect)||!effect.destroyRequested||r->destroyRequested||
           !binding_.markDestroyed(h,*r))return good_=false;
        records_[h.slot].destroyRequested=true;return true;
    }
    bool retire(unsigned local,const DebrisAnimation& effect){
        const auto h=handle(local);const auto* r=find(h);
        // Explicit teardown must still release records after a terminal error.
        if(!r||r->effect!=&effect||!binding_.retire(h,*r))return good_=false;
        records_[h.slot]={};--count_;return true;
    }
    template<unsigned N> bool dispatchAnimation(InstanceHandle key,const Record& record,
        DebrisEffects<N>& effects,typename DebrisEffects<N>::Phase phase){
        if(!owns(key,record,effects))return good_=false;
        return good_=effects.advanceRegisteredAnimationOne(key.generation,phase,*this);
    }
    template<unsigned N> bool retireMarked(InstanceHandle key,const Record& record,DebrisEffects<N>& effects){
        if(!owns(key,record,effects)||!record.destroyRequested)return good_=false;
        return good_=effects.retireMarked(key.generation,*this);
    }
    template<unsigned N> bool dispatchAlarm(InstanceHandle key,const Record& record,
        DebrisEffects<N>& effects,typename DebrisEffects<N>::Phase phase,unsigned index){
        if(!owns(key,record,effects))return good_=false;
        return good_=effects.advanceRegisteredAlarmOne(key.generation,phase,index,*this);
    }
    template<unsigned N,class Random,class Stopped,class Meeting>
    bool dispatchStep(InstanceHandle key,const Record& record,DebrisEffects<N>& effects,
        typename DebrisEffects<N>::Phase phase,Random& random,Stopped stopped,Meeting meeting){
        if(!owns(key,record,effects))return good_=false;
        return good_=effects.stepRegisteredOne(key.generation,phase,random,
            [&](auto,const auto& effect,bool& frozen){return stopped(key,effect,frozen);},
            [&](auto,const auto& effect,auto x,auto y,bool& hit){return meeting(key,effect,x,y,hit);},
            [&](auto h,const auto& effect,auto x,auto y){return move(h,effect,x,y);},*this);
    }
private:
    bool check(unsigned local,const DebrisAnimation& effect){
        const auto* r=find(handle(local));return good_=good_&&r&&r->effect==&effect;
    }
    template<unsigned N> bool owns(InstanceHandle key,const Record& record,const DebrisEffects<N>& effects)const{
        return good_&&find(key)==&record&&effects.retained(key.generation)==record.effect;
    }
    Record records_[capacity]{};unsigned count_=0,newest_=0;bool good_=true;Binding binding_;
};
}
#endif
