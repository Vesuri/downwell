#ifndef DOWNWELL_GAMEPLAY_SCRIPT_SPAWN_INSTANCES_H
#define DOWNWELL_GAMEPLAY_SCRIPT_SPAWN_INSTANCES_H
#include "InstanceLifecycle.h"
#include "ScriptSpawnEffect.h"
#include "RunnerDepthChanges.h"
namespace downwell {
// Native registry borrows stable records from ScriptSpawnEffects. Clear that
// owner through these hooks before room teardown or destroying either owner.
class GameplayScriptSpawnInstances {
public:
    static constexpr unsigned capacity=scriptSpawnEffectCapacity;
    struct Record {const ScriptSpawnEffect* effect=nullptr;unsigned lifetimeId=0;bool active=true,destroyRequested=false;};
    using Binding=RecordLifecycleBinding<Record>;
    explicit GameplayScriptSpawnInstances(Binding binding={}):binding_(binding){}
    bool valid()const{return good_;}
    unsigned size()const{return count_;}
    const Record* find(InstanceHandle h)const{
        return h.generation&&h.slot<capacity&&records_[h.slot].lifetimeId==h.generation?&records_[h.slot]:nullptr;
    }
    InstanceHandle handle(unsigned local)const{
        for(unsigned i=0;local&&i<capacity;++i)if(records_[i].lifetimeId==local)return {i,local};
        return {};
    }
    bool birth(unsigned local,const ScriptSpawnEffect& effect){
        if(!good_||!local||local<=newest_||handle(local).generation)return good_=false;
        unsigned i=0;while(i<capacity&&records_[i].lifetimeId)++i;
        if(i==capacity)return good_=false;
        records_[i]={&effect,local,true};newest_=local;++count_;
        return good_=binding_.birth({i,local},records_[i]);
    }
    bool sprite(unsigned local,const ScriptSpawnEffect& effect){
        const auto h=handle(local);const auto* r=find(h);
        return good_=good_&&r&&r->effect==&effect&&binding_.sprite(h,*r,effect.spriteId)&&
            binding_.imageIndex(h,*r,effect.imageIndex);
    }
    bool speed(unsigned local,const ScriptSpawnEffect& effect){return check(local,effect);}
    bool depth(unsigned local,const ScriptSpawnEffect& effect){
        const auto h=handle(local);const auto* r=find(h);std::int32_t integer;RunnerImageValue exact;
        if(!check(local,effect)||!RunnerDepthService::integerDepth(effect.depth,integer)||
           !exact.assign(MotionBinary64::fromInteger(integer))||!RunnerSpatialSplit::equal(exact,effect.depth))return good_=false;
        // Resource-less fixtures retain only the original unchanged-zero path.
        if(!binding_.hasDepthWrites())return good_=(effect.depth.bits&0x7fffffffu)==0;
        return good_=r&&binding_.publishedDepth(h,*r,effect.depth.promoted());
    }
    bool angle(unsigned local,const ScriptSpawnEffect& effect){
        const auto h=handle(local);const auto* r=find(h);
        return good_=check(local,effect)&&binding_.transform(h,*r,RunnerTransformField::angle,
            effect.angle,{0x3f800000u},0xffffffu);
    }
    bool image(unsigned local,const ScriptSpawnEffect& effect){
        const auto h=handle(local);const auto* r=find(h);
        return good_=check(local,effect)&&binding_.imageIndex(h,*r,effect.imageIndex);
    }
    bool retire(unsigned local,const ScriptSpawnEffect& effect){
        const auto h=handle(local);const auto* r=find(h);
        if(!check(local,effect)||!binding_.retire(h,*r))return good_=false;
        records_[h.slot]={};--count_;return true;
    }
    bool destroyed(unsigned local,const ScriptSpawnEffect& effect){
        const auto h=handle(local);const auto* r=find(h);
        if(!check(local,effect)||!binding_.markDestroyed(h,*r))return good_=false;
        records_[h.slot].destroyRequested=true;return true;
    }
    template<class Effects> bool retireMarked(InstanceHandle key,const Record& record,Effects& effects){
        if(!good_||!effects.boundTo(*this)||find(key)!=&record||!record.destroyRequested)return good_=false;
        return good_=effects.retireMarked(key.generation,*this);
    }
    template<class Effects> bool dispatchAnimation(InstanceHandle key,const Record& record,Effects& effects,
        typename Effects::Phase phase){
        if(!good_||!effects.boundTo(*this)||find(key)!=&record)return good_=false;
        return good_=effects.advanceRegisteredOne(key.generation,phase,*this);
    }
private:
    bool check(unsigned local,const ScriptSpawnEffect& effect){
        const auto* r=find(handle(local));return good_=good_&&r&&r->effect==&effect;
    }
    Record records_[capacity]{};unsigned count_=0,newest_=0;bool good_=true;Binding binding_{};
};
}
#endif
