#ifndef DOWNWELL_CORE_STORED_LEVEL_NUMBER_REGISTRATION_H
#define DOWNWELL_CORE_STORED_LEVEL_NUMBER_REGISTRATION_H
#include "LevelNumber.h"
#include "InstanceStore.h"
#include "InstanceLifecycle.h"
#include "ObjectPack.h"
#include <cstring>
namespace downwell {
// Fixed-address private text state paired with real room records. Numeric IDs
// come from the enclosing source allocator, never a family-local counter.
// Keep the registry at a fixed address and clear it before room teardown.
template<unsigned WorldCapacity,unsigned Capacity,unsigned TextCapacity=64>
class StoredLevelNumberRegistration {
    static_assert(Capacity>0,"level-number ownership requires capacity");
public:
    using State=LevelNumber<TextCapacity>;
private:
    struct Entry {InstanceHandle key{};unsigned id=0;State state;};
    Entry entries_[Capacity]{};
    InstanceStore<WorldCapacity>& records_;const ObjectPack* objects_;
    InstanceLifecycleBinding lifecycle_;std::uint32_t& nextId_;const int& room_;bool& good_;
    bool busy_=false;
    bool fail(){good_=false;return false;}
    Entry* entry(InstanceHandle h){
        for(auto& e:entries_)if(e.id&&e.key.slot==h.slot&&e.key.generation==h.generation)return &e;
        return nullptr;
    }
    InstanceRecord* record(const Entry& e)const{
        auto* r=records_.resolve(e.key);return r&&r->instanceId==e.id&&r->objectId==372?r:nullptr;
    }
    static float stored(RunnerImageValue v){float f;std::memcpy(&f,&v.bits,4);return f;}
public:
    StoredLevelNumberRegistration(InstanceStore<WorldCapacity>& records,const ObjectPack* objects,
        InstanceLifecycleBinding lifecycle,std::uint32_t& nextId,const int& room,bool& good)
        :records_(records),objects_(objects),lifecycle_(lifecycle),nextId_(nextId),room_(room),good_(good){}
    StoredLevelNumberRegistration(const StoredLevelNumberRegistration&)=delete;
    bool belongsTo(const InstanceStore<WorldCapacity>& records)const{return &records_==&records;}
    bool valid()const{
        if(!good_)return false;
        for(const auto& e:entries_)if(e.id&&(!record(e)||!e.state.good))return false;
        return true;
    }
    InstanceHandle handle(const InstanceRecord& r){
        for(auto& e:entries_)if(e.id==r.instanceId&&record(e)==&r)return e.key;
        return {};
    }
    const State* state(InstanceHandle h){auto* e=entry(h);return e&&record(*e)?&e->state:nullptr;}
    template<class Text> bool create(MotionBinary64 x,MotionBinary64 y,Text initialize,InstanceHandle& output){
        output={};if(!good_||busy_)return fail();
        Entry* free=nullptr;for(auto& e:entries_)if(!e.id&&!free)free=&e;
        PackedObject definition;RunnerImageValue px,py;std::int32_t ix,iy;
        if(!free||!objects_||!objects_->findObject(372,definition)||!nextId_||nextId_>0x7fffffffu||
           !px.assign(x)||!py.assign(y)||!px.promoted().roundPixel(ix)||!py.promoted().roundPixel(iy))return fail();
        if(records_.create(nextId_,372,room_,definition.persistent(),definition.creationDepth)!=CreateInstanceResult::success)return fail();
        free->id=nextId_++;free->key=records_.handleAt(records_.size()-1);free->state={};
        auto* r=record(*free);if(!r)return fail();
        r->x=ix;r->y=iy;r->preciseX=stored(px);r->preciseY=stored(py);
        r->spriteId=definition.spriteId;r->maskId=definition.textureMaskId;
        if(!lifecycle_.birthAt(free->key,*r,px,py))return fail();
        busy_=true;const bool okay=initialize(free->key,free->state);busy_=false;
        r=record(*free);if(!r)return fail();r->alarms=free->state.alarms;
        if(!okay||!free->state.good||!free->state.created||!good_)return fail();
        output=free->key;return true;
    }
    bool alarm(InstanceHandle h,unsigned index){
        auto* e=entry(h);auto* r=e?record(*e):nullptr;
        if(!good_||busy_||!r)return fail();
        if(!r->active||r->destroyRequested)return true;
        e->state.alarms=r->alarms;
        const bool okay=e->state.fireAlarm(index,[&]{
            if(!lifecycle_.markDestroyed(h,*r)||!good_)return false;
            r->destroyRequested=true;return true;
        });
        r->alarms=e->state.alarms;return (okay&&good_)||fail();
    }
    template<class Ops> bool draw(InstanceHandle h,Ops& ops){
        auto* e=entry(h);auto* r=e?record(*e):nullptr;
        if(!good_||busy_||!r)return fail();
        if(!r->active||r->destroyRequested)return true;
        busy_=true;const bool okay=e->state.draw(ops);busy_=false;
        return (okay&&good_)||fail();
    }
    bool retire(InstanceHandle h){
        auto* e=entry(h);auto* r=e?record(*e):nullptr;
        if(!good_||busy_||!r||!r->destroyRequested||!e->state.destroyRequested)return fail();
        return erase(*e);
    }
    bool clear(){
        if(!good_||busy_)return fail();
        for(auto& e:entries_)if(e.id&&!erase(e))return false;
        return true;
    }
private:
    bool erase(Entry& e){
        if(!record(e)||!records_.erase(e.id,[&](auto h,const auto& r){return lifecycle_.retire(h,r);}))return fail();
        e={};return true;
    }
};
}
#endif
