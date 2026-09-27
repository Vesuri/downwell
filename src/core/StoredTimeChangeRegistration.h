#ifndef DOWNWELL_CORE_STORED_TIME_CHANGE_REGISTRATION_H
#define DOWNWELL_CORE_STORED_TIME_CHANGE_REGISTRATION_H
#include "InstanceStore.h"
#include "InstanceLifecycle.h"
#include "ObjectPack.h"
#include "TimeChangeEffect.h"
#include <cstring>
namespace downwell {
// Borrowed, fixed-address object-211 registration. Retire all bound effects
// before room teardown. Numeric IDs alone cannot identify recycled lifetimes.
template<unsigned WorldCapacity,unsigned Capacity> class StoredTimeChangeRegistration {
    struct Entry {std::uint32_t local=0,id=0;InstanceHandle key{};};
    Entry entries_[Capacity]{};
    InstanceStore<WorldCapacity>& records_;const ObjectPack* objects_;
    InstanceLifecycleBinding lifecycle_;std::uint32_t& nextId_;const int& room_;bool& succeeded_;
    bool fail(){succeeded_=false;return false;}
    Entry* entry(std::uint32_t local,std::uint32_t id){
        for(auto& e:entries_)if(e.local==local&&e.id==id)return &e;
        return nullptr;
    }
    InstanceRecord* resolve(const Entry& e){
        auto* record=records_.resolve(e.key);
        return record&&record->instanceId==e.id&&record->objectId==211?record:nullptr;
    }
    static float stored(RunnerImageValue value){float f;std::memcpy(&f,&value.bits,sizeof(f));return f;}
public:
    StoredTimeChangeRegistration(InstanceStore<WorldCapacity>& records,const ObjectPack* objects,
        InstanceLifecycleBinding lifecycle,std::uint32_t& nextId,const int& room,bool& succeeded)
        :records_(records),objects_(objects),lifecycle_(lifecycle),nextId_(nextId),room_(room),succeeded_(succeeded){}
    StoredTimeChangeRegistration(const StoredTimeChangeRegistration&)=delete;
    StoredTimeChangeRegistration& operator=(const StoredTimeChangeRegistration&)=delete;
    bool belongsTo(const InstanceStore<WorldCapacity>& records)const{return &records_==&records;}
    template<class Effects> bool dispatchAlarm(const InstanceRecord& record,Effects& effects,unsigned index,int& speed){
        if(!succeeded_)return false;
        if(!effects.boundTo(*this))return fail();
        for(auto& e:entries_)if(e.local&&e.id==record.instanceId){
            if(resolve(e)!=&record)return fail();
            if(!effects.dispatchAlarm(e.local,e.id,record.alarms,index,speed))return fail();
            return true;
        }
        return fail();
    }
    bool birth(std::uint32_t local,RunnerImageValue x,RunnerImageValue y,std::uint32_t& id){
        if(!succeeded_)return false;
        Entry* free=nullptr;
        for(auto& e:entries_){if(e.local==local&&local)return fail();if(!e.local&&!free)free=&e;}
        PackedObject definition;std::int32_t px,py;
        if(!local||!free||!objects_||!objects_->findObject(211,definition)||
           !x.promoted().roundPixel(px)||!y.promoted().roundPixel(py)||
           !nextId_||nextId_>0x7fffffffu)return fail();
        id=nextId_;
        if(records_.create(id,211,room_,definition.persistent(),definition.creationDepth)!=CreateInstanceResult::success)return fail();
        ++nextId_;*free={local,id,records_.handleAt(records_.size()-1)};
        auto* record=resolve(*free);if(!record)return fail();
        record->x=px;record->y=py;record->preciseX=stored(x);record->preciseY=stored(y);
        record->spriteId=definition.spriteId;record->maskId=definition.textureMaskId;
        // PreCreate only calls inherited and the resource has no parent.
        if(!lifecycle_.birthAt(free->key,*record,x,y))return fail();
        return true;
    }
    bool publish(std::uint32_t local,std::uint32_t id,const TimeChangeEffect& effect){
        if(!succeeded_)return false;
        auto* e=entry(local,id);auto* record=e?resolve(*e):nullptr;
        if(!record)return fail();
        record->alarms.set(0,effect.alarm0);record->alarms.set(1,effect.alarm1);return true;
    }
    bool eventEligible(std::uint32_t local,std::uint32_t id,bool& eligible){
        if(!succeeded_)return false;
        auto* e=entry(local,id);auto* record=e?resolve(*e):nullptr;
        if(!record)return fail();
        eligible=record->active&&!record->destroyRequested;return true;
    }
    bool destroyed(std::uint32_t local,std::uint32_t id,const TimeChangeEffect&){
        if(!succeeded_)return false;
        auto* e=entry(local,id);auto* record=e?resolve(*e):nullptr;
        if(!record||!lifecycle_.markDestroyed(e->key,*record))return fail();
        record->destroyRequested=true;return true;
    }
    bool retire(std::uint32_t local,std::uint32_t id,const TimeChangeEffect&){
        if(!succeeded_)return false;
        auto* e=entry(local,id);if(!e||!resolve(*e))return fail();
        if(!records_.erase(id,[&](InstanceHandle key,const InstanceRecord& record){return lifecycle_.retire(key,record);}))return fail();
        *e={};return true;
    }
    template<class Effects> bool retireMarked(InstanceHandle key,const InstanceRecord& record,Effects& effects){
        if(!succeeded_)return false;
        if(!effects.boundTo(*this)||!record.destroyRequested||records_.resolve(key)!=&record)return fail();
        for(const auto& e:entries_)if(e.local&&e.key.slot==key.slot&&e.key.generation==key.generation){
            if(resolve(e)!=&record||e.id!=record.instanceId)return fail();
            if(!effects.retireMarked(e.local,e.id))return fail();
            return true;
        }
        return fail();
    }
};
}
#endif
