#ifndef DOWNWELL_CORE_STORED_SCRIPT_SPAWN_REGISTRATION_H
#define DOWNWELL_CORE_STORED_SCRIPT_SPAWN_REGISTRATION_H
#include "InstanceStore.h"
#include "InstanceLifecycle.h"
#include "ObjectPack.h"
#include "ScriptSpawnEffect.h"
#include <cstring>
namespace downwell {
// Borrowed registration for object 219. Keep at a fixed address and clear all
// effects before room teardown. This bridges identities/setters, not scheduling.
template<unsigned WorldCapacity,unsigned Capacity> class StoredScriptSpawnRegistration {
    struct Entry {unsigned local=0,id=0;InstanceHandle key{};};
    Entry entries_[Capacity]{};
    InstanceStore<WorldCapacity>& records_;const ObjectPack* objects_;
    InstanceLifecycleBinding lifecycle_;std::uint32_t& nextId_;const int& room_;bool& succeeded_;
    bool fail(){succeeded_=false;return false;}
    Entry* entry(unsigned local){for(auto& e:entries_)if(local&&e.local==local)return &e;return nullptr;}
    InstanceRecord* record(const Entry& e){
        auto* r=records_.resolve(e.key);return r&&r->instanceId==e.id&&r->objectId==219?r:nullptr;
    }
    static float stored(RunnerImageValue n){float f;std::memcpy(&f,&n.bits,4);return f;}
public:
    StoredScriptSpawnRegistration(InstanceStore<WorldCapacity>& records,const ObjectPack* objects,
        InstanceLifecycleBinding lifecycle,std::uint32_t& nextId,const int& room,bool& succeeded)
        :records_(records),objects_(objects),lifecycle_(lifecycle),nextId_(nextId),room_(room),succeeded_(succeeded){}
    StoredScriptSpawnRegistration(const StoredScriptSpawnRegistration&)=delete;
    StoredScriptSpawnRegistration& operator=(const StoredScriptSpawnRegistration&)=delete;
    bool belongsTo(const InstanceStore<WorldCapacity>& records)const{return &records_==&records;}
    unsigned sourceId(unsigned local){auto* e=entry(local);return succeeded_&&e&&record(*e)?e->id:0;}
    bool birth(unsigned local,const ScriptSpawnEffect& effect){
        if(!succeeded_)return false;
        Entry* free=nullptr;
        for(auto& e:entries_){if(local&&e.local==local)return fail();if(!e.local&&!free)free=&e;}
        PackedObject definition;std::int32_t x,y;
        if(!local||!free||!objects_||!objects_->findObject(219,definition)||
           !effect.x.promoted().roundPixel(x)||!effect.y.promoted().roundPixel(y)||
           !nextId_||nextId_>0x7fffffffu||effect.spriteId!=-1||effect.frames||
           effect.imageSpeed.bits!=0x3f800000u||effect.angle.bits||effect.depth.bits)return fail();
        const auto id=nextId_;
        if(records_.create(id,219,room_,definition.persistent(),definition.creationDepth)!=CreateInstanceResult::success)return fail();
        ++nextId_;*free={local,id,records_.handleAt(records_.size()-1)};
        auto* r=record(*free);if(!r)return fail();
        r->x=x;r->y=y;r->preciseX=stored(effect.x);r->preciseY=stored(effect.y);
        r->spriteId=definition.spriteId;r->maskId=definition.textureMaskId;
        return lifecycle_.birthAt(free->key,*r,effect.x,effect.y)||fail();
    }
    bool sprite(unsigned local,const ScriptSpawnEffect& effect){
        if(!succeeded_)return false;
        auto* e=entry(local);auto* r=e?record(*e):nullptr;
        if(!r||!lifecycle_.sprite(e->key,*r,effect.spriteId))return fail();
        r->spriteId=effect.spriteId;
        if(!lifecycle_.imageIndex(e->key,*r,effect.imageIndex))return fail();
        r->imageIndex=stored(effect.imageIndex);return true;
    }
    bool speed(unsigned local,const ScriptSpawnEffect& effect){
        if(!succeeded_)return false;
        auto* e=entry(local);auto* r=e?record(*e):nullptr;if(!r)return fail();
        r->imageSpeed=stored(effect.imageSpeed);return true;
    }
    bool angle(unsigned local,const ScriptSpawnEffect& effect){
        if(!succeeded_)return false;
        auto* e=entry(local);auto* r=e?record(*e):nullptr;
        if(!r||!lifecycle_.transform(e->key,*r,RunnerTransformField::angle,effect.angle,
            {0x3f800000u},0xffffffu))return fail();
        r->rotation=stored(effect.angle);return true;
    }
    bool image(unsigned local,const ScriptSpawnEffect& effect){
        if(!succeeded_)return false;
        auto* e=entry(local);auto* r=e?record(*e):nullptr;
        if(!r||!lifecycle_.imageIndex(e->key,*r,effect.imageIndex))return fail();
        r->imageIndex=stored(effect.imageIndex);return true;
    }
    // The admitted muzzle depth is unchanged zero. Nonzero depth needs the
    // world's pending-depth-list setter; never pretend that is implemented.
    bool depth(unsigned local,const ScriptSpawnEffect& effect){
        if(!succeeded_)return false;
        auto* e=entry(local);auto* r=e?record(*e):nullptr;
        return (r&&r->depth==0&&(effect.depth.bits&0x7fffffffu)==0)||fail();
    }
    bool retire(unsigned local,const ScriptSpawnEffect&){
        if(!succeeded_)return false;
        auto* e=entry(local);if(!e||!record(*e))return fail();
        if(!records_.erase(e->id,[&](InstanceHandle h,const InstanceRecord& r){return lifecycle_.retire(h,r);}))return fail();
        *e={};return true;
    }
    bool destroyed(unsigned local,const ScriptSpawnEffect&){
        if(!succeeded_)return false;
        auto* e=entry(local);auto* r=e?record(*e):nullptr;
        if(!r||!lifecycle_.markDestroyed(e->key,*r))return fail();
        r->destroyRequested=true;return true;
    }
    template<class Effects> bool dispatchAnimation(InstanceHandle key,const InstanceRecord& r,Effects& effects,
        typename Effects::Phase phase){
        if(!succeeded_||!effects.boundTo(*this)||records_.resolve(key)!=&r)return fail();
        for(const auto& e:entries_)if(e.local&&e.key.slot==key.slot&&e.key.generation==key.generation){
            if(record(e)!=&r)return fail();
            return effects.advanceRegisteredOne(e.local,phase,*this)||fail();
        }
        return fail();
    }
    template<class Effects> bool retireMarked(InstanceHandle key,const InstanceRecord& r,Effects& effects){
        if(!succeeded_||!effects.boundTo(*this)||records_.resolve(key)!=&r||!r.destroyRequested)return fail();
        for(const auto& e:entries_)if(e.local&&e.key.slot==key.slot&&e.key.generation==key.generation){
            if(record(e)!=&r)return fail();
            return effects.retireMarked(e.local,*this)||fail();
        }
        return fail();
    }
};
}
#endif
