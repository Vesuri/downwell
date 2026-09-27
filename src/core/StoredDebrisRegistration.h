#ifndef DOWNWELL_CORE_STORED_DEBRIS_REGISTRATION_H
#define DOWNWELL_CORE_STORED_DEBRIS_REGISTRATION_H
#include "InstanceStore.h"
#include "InstanceLifecycle.h"
#include "ObjectPack.h"
#include "DebrisEffects.h"
#include "DebrisDraw.h"
#include "TimeStopBound.h"
#include <cstring>
namespace downwell {
// SDL stored-world identity bridge. Keep both owners at fixed addresses and
// clear effects through this adapter before room teardown. This is not a
// second simulation: DebrisEffects owns motion, animation and alarm behavior.
template<unsigned WorldCapacity,unsigned Capacity> class StoredDebrisRegistration {
    struct Entry {unsigned local=0,id=0;InstanceHandle key{};const DebrisAnimation* effect=nullptr;};
    Entry entries_[Capacity]{};unsigned newest_=0;
    InstanceStore<WorldCapacity>& records_;const ObjectPack* objects_;
    InstanceLifecycleBinding lifecycle_;std::uint32_t& nextId_;const int& room_;bool& succeeded_;
    bool fail(){succeeded_=false;return false;}
    Entry* forRecord(const InstanceRecord& r){
        for(auto& e:entries_)if(e.local&&e.id==r.instanceId)return record(e)==&r?&e:nullptr;
        return nullptr;
    }
    Entry* entry(unsigned local){for(auto& e:entries_)if(local&&e.local==local)return &e;return nullptr;}
    InstanceRecord* record(const Entry& e){
        auto* r=records_.resolve(e.key);return r&&r->instanceId==e.id&&r->objectId==370?r:nullptr;
    }
    Entry* checked(unsigned local,const DebrisAnimation& effect){
        auto* e=entry(local);return succeeded_&&e&&e->effect==&effect&&record(*e)?e:nullptr;
    }
    template<unsigned N> Entry* selected(InstanceHandle key,const InstanceRecord& r,DebrisEffects<N>& effects){
        if(!succeeded_||records_.resolve(key)!=&r)return nullptr;
        for(auto& e:entries_)if(e.local&&e.key.slot==key.slot&&e.key.generation==key.generation)
            return record(e)==&r&&effects.retained(e.local)==e.effect?&e:nullptr;
        return nullptr;
    }
    static float stored(RunnerImageValue v){float f;std::memcpy(&f,&v.bits,4);return f;}
public:
    StoredDebrisRegistration(InstanceStore<WorldCapacity>& records,const ObjectPack* objects,
        InstanceLifecycleBinding lifecycle,std::uint32_t& nextId,const int& room,bool& succeeded)
        :records_(records),objects_(objects),lifecycle_(lifecycle),nextId_(nextId),room_(room),succeeded_(succeeded){}
    StoredDebrisRegistration(const StoredDebrisRegistration&)=delete;
    StoredDebrisRegistration& operator=(const StoredDebrisRegistration&)=delete;
    bool belongsTo(const InstanceStore<WorldCapacity>& records)const{return &records_==&records;}
    const DebrisAnimation* drawEffect(InstanceHandle key,const InstanceRecord& r)const{
        if(!succeeded_||records_.resolve(key)!=&r||r.objectId!=370)return nullptr;
        for(const auto& e:entries_)if(e.local&&e.id==r.instanceId&&
            e.key.slot==key.slot&&e.key.generation==key.generation)return e.effect;
        return nullptr;
    }
    unsigned sourceId(unsigned local){auto* e=entry(local);return succeeded_&&e&&record(*e)?e->id:0;}
    bool birth(unsigned local,const DebrisAnimation& effect){
        if(!succeeded_)return false;
        Entry* free=nullptr;for(auto& e:entries_)if(!e.local&&!free)free=&e;
        PackedObject definition;std::int32_t x,y;
        if(!local||local<=newest_||!free||!objects_||!objects_->findObject(370,definition)||
           !effect.x.promoted().roundPixel(x)||!effect.y.promoted().roundPixel(y)||
           !nextId_||nextId_>0x7fffffffu)return fail();
        const auto id=nextId_;
        if(records_.create(id,370,room_,definition.persistent(),DebrisDraw::sourceDepth)!=CreateInstanceResult::success)return fail();
        ++nextId_;newest_=local;*free={local,id,records_.handleAt(records_.size()-1),&effect};
        auto* r=record(*free);if(!r)return fail();
        r->x=x;r->y=y;r->preciseX=stored(effect.x);r->preciseY=stored(effect.y);
        r->spriteId=effect.spriteId;r->maskId=definition.textureMaskId;
        r->imageIndex=stored(effect.imageIndex);r->imageSpeed=stored(effect.imageSpeed);r->alarms=effect.alarms;
        return lifecycle_.birthAt(free->key,*r,effect.x,effect.y)||fail();
    }
    bool image(unsigned local,const DebrisAnimation& effect){
        auto* e=checked(local,effect);auto* r=e?record(*e):nullptr;
        if(!r||!lifecycle_.imageIndex(e->key,*r,effect.imageIndex))return fail();
        r->imageIndex=stored(effect.imageIndex);return true;
    }
    bool move(unsigned local,const DebrisAnimation& effect,RunnerImageValue x,RunnerImageValue y){
        auto* e=checked(local,effect);auto* r=e?record(*e):nullptr;std::int32_t px,py;
        if(!r||!x.promoted().roundPixel(px)||!y.promoted().roundPixel(py)||
           !lifecycle_.publishedPosition(e->key,*r,x,y))return fail();
        r->x=px;r->y=py;r->preciseX=stored(x);r->preciseY=stored(y);return true;
    }
    bool meeting(unsigned local,const DebrisAnimation& effect,MotionBinary64 x,MotionBinary64 y,bool& hit){
        auto* e=checked(local,effect);auto* r=e?record(*e):nullptr;bool next=false;
        if(!r||!lifecycle_.placeMeeting(e->key,*r,56,x,y,next))return fail();
        hit=next;return true;
    }
    bool destroyed(unsigned local,const DebrisAnimation& effect){
        auto* e=checked(local,effect);auto* r=e?record(*e):nullptr;
        if(!r||!effect.destroyRequested||r->destroyRequested||!lifecycle_.markDestroyed(e->key,*r))return fail();
        r->destroyRequested=true;return true;
    }
    bool frozen(InstanceHandle key,const DebrisAnimation& effect,bool pTimeStop,int roomWidth,bool& frozen){
        const auto* r=records_.resolve(key);auto* e=r?forRecord(*r):nullptr;bool allowed=false;
        if(!e||!checked(e->local,effect))return fail();
        if(!TimeStopBound::evaluate(pTimeStop,effect.x.promoted(),effect.y.promoted(),roomWidth,
            [&](auto x,auto y,bool& hit){return lifecycle_.placeMeeting(key,*r,82,x,y,hit);},allowed))return fail();
        frozen=!allowed;return true;
    }
    bool retire(unsigned local,const DebrisAnimation& effect){
        auto* e=entry(local);
        if(!e||e->effect!=&effect||!record(*e)||
           !records_.erase(e->id,[&](InstanceHandle h,const InstanceRecord& r){return lifecycle_.retire(h,r);}))return fail();
        *e={};return true;
    }
    template<unsigned N> bool dispatchAnimation(InstanceHandle key,const InstanceRecord& r,DebrisEffects<N>& effects,
        typename DebrisEffects<N>::Phase phase){
        auto* e=selected(key,r,effects);
        return (e&&effects.advanceRegisteredAnimationOne(e->local,phase,*this))||fail();
    }
    // Advance once, before any generic scheduler decrements the record bank.
    template<unsigned N> bool advanceAlarm(InstanceHandle key,const InstanceRecord& r,DebrisEffects<N>& effects,
        typename DebrisEffects<N>::Phase phase,unsigned index){
        auto* e=selected(key,r,effects);
        if(!e||!effects.advanceRegisteredAlarmOne(e->local,phase,index,*this))return fail();
        record(*e)->alarms=e->effect->alarms;return true;
    }
    template<unsigned N> bool dispatchAlarmEvent(const InstanceRecord& r,DebrisEffects<N>& effects,unsigned index){
        auto* e=forRecord(r);
        if(!e||!selected(e->key,r,effects))return fail();
        return effects.dispatchRegisteredAlarmEventOne(e->local,r.alarms,index,*this)||fail();
    }
    template<unsigned N,class Random,class Stopped>
    bool dispatchScheduledStep(const InstanceRecord& r,DebrisEffects<N>& effects,Random& random,Stopped stopped){
        auto* e=forRecord(r);
        if(!e||!selected(e->key,r,effects))return fail();
        auto* effect=effects.get(e->local);if(!effect)return fail();
        // Include countdowns that did not fire an event, and the following
        // tick's zero-to-inactive changes, before TimeStopBound refunds them.
        effect->alarms=r.alarms;
        return dispatchStep(e->key,r,effects,effects.beginPhase(),random,stopped);
    }
    template<unsigned N,class Random,class Stopped>
    bool dispatchStep(InstanceHandle key,const InstanceRecord& r,DebrisEffects<N>& effects,
        typename DebrisEffects<N>::Phase phase,Random& random,Stopped stopped){
        auto* e=selected(key,r,effects);if(!e)return fail();
        if(!effects.stepRegisteredOne(e->local,phase,random,
            [&](auto,const auto& effect,bool& frozen){return stopped(key,effect,frozen);},
            [&](auto local,const auto& effect,auto x,auto y,bool& hit){return meeting(local,effect,x,y,hit);},
            [&](auto local,const auto& effect,auto x,auto y){return move(local,effect,x,y);},*this))return fail();
        auto* updated=record(*e);if(!updated)return fail();
        updated->alarms=e->effect->alarms;updated->imageSpeed=stored(e->effect->imageSpeed);return true;
    }
    template<unsigned N> bool retireMarked(InstanceHandle key,const InstanceRecord& r,DebrisEffects<N>& effects){
        auto* e=selected(key,r,effects);
        return (e&&r.destroyRequested&&effects.retireMarked(e->local,*this))||fail();
    }
};
}
#endif
