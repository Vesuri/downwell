#ifndef DOWNWELL_CORE_STORED_ENTRY_DITHER_REGISTRATION_H
#define DOWNWELL_CORE_STORED_ENTRY_DITHER_REGISTRATION_H
#include "EntryDither.h"
#include "InstanceStore.h"
#include "InstanceLifecycle.h"
#include "ObjectPack.h"
#include <cstring>
namespace downwell {
// Borrowed room bridge. It owns effect state, not room traversal. The caller
// must retire every record before room teardown and keep this bridge stable.
template<unsigned WorldCapacity,unsigned Capacity> class StoredEntryDitherRegistration {
    struct Slot {InstanceHandle key{};std::uint32_t id=0;EntryDither effect;};
    Slot slots_[Capacity]{};
    InstanceStore<WorldCapacity>& records_;const ObjectPack* objects_;
    InstanceLifecycleBinding lifecycle_;std::uint32_t& nextId_;const int& room_;bool& good_;
    bool fail(){good_=false;return false;}
    Slot* slot(InstanceHandle key){
        for(auto& s:slots_)if(s.id&&s.key.slot==key.slot&&s.key.generation==key.generation)return &s;
        return nullptr;
    }
    InstanceRecord* record(const Slot& s){
        auto* r=records_.resolve(s.key);return r&&r->instanceId==s.id&&r->objectId==73?r:nullptr;
    }
    static float stored(RunnerImageValue v){float f;std::memcpy(&f,&v.bits,4);return f;}
    bool publish(Slot& s,EntryDither::Write write,const EntryDither& e){
        auto* r=record(s);if(!r)return fail();
        if(write==EntryDither::Write::alarm){r->alarms.set(0,e.alarm0);return true;}
        std::int32_t pixel;
        const auto value=write==EntryDither::Write::x?e.x:e.y;
        if(!value.promoted().roundPixel(pixel))return fail();
        // Publish one setter at a time; the other coordinate is unchanged.
        RunnerImageValue x,y;
        std::memcpy(&x.bits,&r->preciseX,4);std::memcpy(&y.bits,&r->preciseY,4);
        if(write==EntryDither::Write::x)x=value;else y=value;
        if(!lifecycle_.publishedPosition(s.key,*r,x,y))return fail();
        if(write==EntryDither::Write::x){r->x=pixel;r->preciseX=stored(value);}
        else {r->y=pixel;r->preciseY=stored(value);}
        return true;
    }
public:
    StoredEntryDitherRegistration(InstanceStore<WorldCapacity>& records,const ObjectPack* objects,
        InstanceLifecycleBinding lifecycle,std::uint32_t& nextId,const int& room,bool& good)
        :records_(records),objects_(objects),lifecycle_(lifecycle),nextId_(nextId),room_(room),good_(good){}
    StoredEntryDitherRegistration(const StoredEntryDitherRegistration&)=delete;
    StoredEntryDitherRegistration& operator=(const StoredEntryDitherRegistration&)=delete;
    bool belongsTo(const InstanceStore<WorldCapacity>& records)const{return &records_==&records;}
    InstanceHandle handle(const InstanceRecord& r){
        for(auto& s:slots_)if(s.id&&s.id==r.instanceId&&record(s)==&r)return s.key;
        return {};
    }
    template<class CameraX,class ViewY> bool create(CameraX cameraX,ViewY viewY,InstanceHandle& result){
        if(!good_)return false;
        Slot* free=nullptr;for(auto& s:slots_)if(!s.id){free=&s;break;}
        PackedObject definition;
        if(!free||!objects_||!objects_->findObject(73,definition)||!nextId_||nextId_>0x7fffffffu)return fail();
        const auto id=nextId_;
        if(records_.create(id,73,room_,definition.persistent(),definition.creationDepth)!=CreateInstanceResult::success)return fail();
        ++nextId_;*free={};free->id=id;free->key=records_.handleAt(records_.size()-1);
        result=free->key;auto* r=record(*free);if(!r)return fail();
        r->spriteId=definition.spriteId;r->maskId=definition.textureMaskId;
        if(!lifecycle_.birth(free->key,*r))return fail();
        return free->effect.create(cameraX,viewY,[&](auto w,const auto& e){return publish(*free,w,e);})||fail();
    }
    template<class CameraX> bool step(InstanceHandle key,CameraX cameraX){
        if(!good_)return false;
        auto* s=slot(key);auto* r=s?record(*s):nullptr;if(!r)return fail();
        if(!r->active||r->destroyRequested)return true;
        // Import published coordinates: external setters are authoritative.
        std::memcpy(&s->effect.x.bits,&r->preciseX,4);std::memcpy(&s->effect.y.bits,&r->preciseY,4);
        return s->effect.step(cameraX,[&](auto w,const auto& e){return publish(*s,w,e);})||fail();
    }
    bool alarm(InstanceHandle key){
        if(!good_)return false;
        auto* s=slot(key);auto* r=s?record(*s):nullptr;if(!r)return fail();
        if(!r->active||r->destroyRequested)return true;
        if(r->alarms.get(0)!=0)return fail();
        if(!s->effect.fireAlarm(0)||!lifecycle_.markDestroyed(key,*r))return fail();
        r->destroyRequested=true;return true;
    }
    bool retire(InstanceHandle key){
        if(!good_)return false;
        auto* s=slot(key);if(!s||!record(*s))return fail();
        if(!records_.erase(s->id,[&](auto h,const auto& r){return lifecycle_.retire(h,r);}))return fail();
        *s={};return true;
    }
};
}
#endif
