#ifndef DOWNWELL_CORE_STORED_TIME_SHARD_REGISTRATION_H
#define DOWNWELL_CORE_STORED_TIME_SHARD_REGISTRATION_H
#include "InstanceStore.h"
#include "InstanceLifecycle.h"
#include "ObjectPack.h"
#include "TimeShardAnimation.h"
#include <cstring>
namespace downwell {
// Borrowed bridge to the real stored world. Keep this adapter at a fixed
// address until every bound shard has retired, before room teardown. Neither
// the effect handle nor a numeric ID alone may resolve a recycled room slot.
template<unsigned WorldCapacity,unsigned ShardCapacity> class StoredTimeShardRegistration {
    struct Entry {std::uint32_t local=0,id=0;InstanceHandle key{};};
    Entry entries_[ShardCapacity]{};
    InstanceStore<WorldCapacity>& records_;
    const ObjectPack* objects_;
    InstanceLifecycleBinding lifecycle_;
    std::uint32_t& nextId_;
    const int& room_;
    bool& succeeded_;
    Entry* entry(std::uint32_t local,std::uint32_t id){
        for(auto& e:entries_)if(e.local==local&&e.id==id)return &e;
        return nullptr;
    }
    InstanceRecord* resolve(const Entry& e){
        auto* r=records_.resolve(e.key);
        return r&&r->instanceId==e.id&&r->objectId==238?r:nullptr;
    }
    static float stored(RunnerImageValue v){float f;std::memcpy(&f,&v.bits,sizeof(f));return f;}
    bool fail(){succeeded_=false;return false;}
public:
    StoredTimeShardRegistration(InstanceStore<WorldCapacity>& records,const ObjectPack* objects,
        InstanceLifecycleBinding lifecycle,std::uint32_t& nextId,const int& room,bool& succeeded)
        :records_(records),objects_(objects),lifecycle_(lifecycle),nextId_(nextId),room_(room),succeeded_(succeeded){}
    StoredTimeShardRegistration(const StoredTimeShardRegistration&)=delete;
    StoredTimeShardRegistration& operator=(const StoredTimeShardRegistration&)=delete;
    bool birth(std::uint32_t local,RunnerImageValue x,RunnerImageValue y,std::uint32_t& id){
        if(!succeeded_)return false;
        Entry* free=nullptr;
        for(auto& e:entries_){if(e.local==local&&local)return fail();if(!e.local&&!free)free=&e;}
        PackedObject definition;std::int32_t px,py;
        if(!local||!free||!objects_||!objects_->findObject(238,definition)||
           !x.promoted().roundPixel(px)||!y.promoted().roundPixel(py)||
           !nextId_||nextId_>0x7fffffffu)return fail();
        id=nextId_;
        if(records_.create(id,238,room_,definition.persistent(),definition.creationDepth)!=CreateInstanceResult::success)
            return fail();
        ++nextId_;
        *free={local,id,records_.handleAt(records_.size()-1)};
        auto* r=resolve(*free);
        if(!r)return fail();
        r->x=px;r->y=py;r->preciseX=stored(x);r->preciseY=stored(y);
        r->spriteId=definition.spriteId;r->maskId=definition.textureMaskId;
        // fxTimeShard PreCreate only calls inherited with no parent. Its
        // actual Create belongs to TimeShardEffects and runs after this call.
        if(!lifecycle_.birthAt(free->key,*r,x,y))return fail();
        return true;
    }
    bool created(std::uint32_t local,std::uint32_t id,const TimeShardAnimation& effect){
        if(!succeeded_)return false;
        auto* e=entry(local,id);auto* r=e?resolve(*e):nullptr;
        if(!r)return fail();
        r->imageSpeed=stored(effect.imageSpeed);
        if(!lifecycle_.imageIndex(e->key,*r,effect.imageIndex))return fail();
        r->imageIndex=stored(effect.imageIndex);
        r->alarms.set(0,effect.alarm0);r->alarms.set(1,effect.alarm1);r->alarms.set(2,effect.alarm2);
        return true;
    }
    bool retire(std::uint32_t local,std::uint32_t id,const TimeShardAnimation&){
        if(!succeeded_)return false;
        auto* e=entry(local,id);
        if(!e||!resolve(*e))return fail();
        if(!records_.erase(id,[&](InstanceHandle h,const InstanceRecord& r){return lifecycle_.retire(h,r);}))return fail();
        *e={};return true;
    }
};
}
#endif
