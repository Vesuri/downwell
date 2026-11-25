#ifndef DOWNWELL_CORE_STORED_SMOKE_REGISTRATION_H
#define DOWNWELL_CORE_STORED_SMOKE_REGISTRATION_H
#include "SmokeEffect.h"
#include "SmokeDirection.h"
#include "SmokeTrigNarrow.h"
#include "SkeletonEffectBursts.h"
#include "InstanceStore.h"
#include "InstanceLifecycle.h"
#include "ObjectPack.h"
#include <cstring>
namespace downwell {
// Borrowed room registration. No family-local source IDs, eager removal, or
// private scheduling: the enclosing room dispatches each admitted event.
template<unsigned WorldCapacity,unsigned Capacity> class StoredSmokeRegistration {
    struct Entry {InstanceHandle key{};unsigned id=0;SmokeEffect state;};
    Entry entries_[Capacity]{};
    InstanceStore<WorldCapacity>& records_;const ObjectPack* objects_;
    InstanceLifecycleBinding lifecycle_;std::uint32_t& nextId_;const int& room_;bool& good_;
    bool fail(){good_=false;return false;}
    static float stored(RunnerImageValue v){float f;std::memcpy(&f,&v.bits,4);return f;}
    Entry* entry(const InstanceRecord& r){
        for(auto& e:entries_)if(e.id==r.instanceId&&records_.resolve(e.key)==&r&&r.objectId==225)return &e;
        return nullptr;
    }
    bool erase(Entry& e){
        if(!records_.resolve(e.key)||!records_.erase(e.id,[&](auto h,const auto& r){return lifecycle_.retire(h,r);}))return fail();
        e={};return true;
    }
public:
    StoredSmokeRegistration(InstanceStore<WorldCapacity>& records,const ObjectPack* objects,
        InstanceLifecycleBinding lifecycle,std::uint32_t& nextId,const int& room,bool& good)
        :records_(records),objects_(objects),lifecycle_(lifecycle),nextId_(nextId),room_(room),good_(good){}
    StoredSmokeRegistration(const StoredSmokeRegistration&)=delete;
    StoredSmokeRegistration& operator=(const StoredSmokeRegistration&)=delete;
    bool belongsTo(const InstanceStore<WorldCapacity>& records)const{return &records_==&records;}
    bool valid()const{return good_;}
    bool setDirection(std::uint32_t id,MotionBinary64 value){
        auto* r=records_.find(id);auto* e=r?entry(*r):nullptr;
        if(!good_||!e||!value.valid())return fail();e->state.direction=value;return true;
    }
    bool setSpeed(std::uint32_t id,MotionBinary64 value){
        auto* r=records_.find(id);auto* e=r?entry(*r):nullptr;
        if(!good_||!e||!value.valid())return fail();e->state.speed=value;return true;
    }
    const SmokeEffect* state(const InstanceRecord& r){auto* e=entry(r);return e?&e->state:nullptr;}
    template<class Random> bool skeletonBurst(SmokeBurstCaller& caller,MotionBinary64 x,MotionBinary64 y,Random& random){
        if(!good_)return false;
        return smokeBurstPublished<5,1>(caller,x,y,random,[&](int object,auto px,auto py){
            std::uint32_t id=0;return object==225&&create(px,py,random,id);
        })||fail();
    }
    template<class Random> bool create(MotionBinary64 x,MotionBinary64 y,Random& random,std::uint32_t& output){
        output=0;if(!good_)return false;
        Entry* free=nullptr;for(auto& e:entries_)if(!e.id&&!free)free=&e;
        PackedObject definition;RunnerImageValue px,py;std::int32_t ix,iy;
        if(!free||!objects_||!objects_->findObject(225,definition)||definition.spriteId!=84||
           !nextId_||nextId_>0x7fffffffu||!px.assign(x)||!py.assign(y)||
           !px.roundPixel(ix)||!py.roundPixel(iy))return fail();
        if(records_.create(nextId_,225,room_,definition.persistent(),definition.creationDepth)!=CreateInstanceResult::success)return fail();
        free->id=nextId_++;free->key=records_.handleAt(records_.size()-1);free->state={};
        auto* r=records_.resolve(free->key);if(!r)return fail();
        r->x=ix;r->y=iy;r->preciseX=stored(px);r->preciseY=stored(py);
        r->spriteId=definition.spriteId;r->maskId=definition.textureMaskId;
        if(!lifecycle_.birthAt(free->key,*r,px,py)||!free->state.create(x,y,random))return fail();
        r->imageSpeed=stored(free->state.imageSpeed);output=free->id;return true;
    }
    template<class Random> bool step(const InstanceRecord& source,Random& random){
        auto* e=entry(source);if(!good_||!e)return fail();
        if(!source.active||source.destroyRequested)return true;
        struct Publish {
            StoredSmokeRegistration& owner;Entry& e;
            bool imageSpeed(RunnerImageValue value){
                auto* r=owner.records_.resolve(e.key);if(!r)return owner.fail();
                r->imageSpeed=stored(value);return true;
            }
            bool position(RunnerImageValue x,RunnerImageValue y){
                auto* r=owner.records_.resolve(e.key);std::int32_t ix,iy;
                if(!r||!x.roundPixel(ix)||!y.roundPixel(iy)||
                   !owner.lifecycle_.publishedPosition(e.key,*r,x,y))return owner.fail();
                r->x=ix;r->y=iy;r->preciseX=stored(x);r->preciseY=stored(y);return true;
            }
        } publish{*this,*e};
        return e->state.stepPublished(random,[](auto speed,auto direction,auto& x,auto& y){
            return SmokeDirection::vector(speed,direction,x,y,smokeTrigNarrowed);
        },publish,smokeEscapedDistance,good_);
    }
    bool advanceAnimation(const InstanceRecord& source){
        auto* e=entry(source);if(!good_||!e)return fail();
        if(!source.active||source.destroyRequested)return true;
        auto* r=records_.resolve(e->key);
        if(r->spriteId!=84)return fail();
        std::memcpy(&e->state.imageSpeed.bits,&r->imageSpeed,4);
        if(!e->state.advanceImage()||!lifecycle_.imageIndex(e->key,*r,e->state.imageIndex))return fail();
        r->imageIndex=stored(e->state.imageIndex);
        if(e->state.destroyRequested){
            if(!lifecycle_.markDestroyed(e->key,*r))return fail();r->destroyRequested=true;
        }
        return true;
    }
    // Called by the ordered room collision dispatcher once for each wall.
    bool wallContact(const InstanceRecord& source){
        auto* e=entry(source);return (good_&&e&&e->state.collideWall())||fail();
    }
    bool alarmEvent(const InstanceRecord& source,unsigned index){
        auto* e=entry(source);if(!good_||!e||index!=0)return fail();e->state.alarm0();return true;
    }
    bool retire(const InstanceRecord& source){
        auto* e=entry(source);
        return (good_&&e&&source.destroyRequested&&e->state.destroyRequested&&erase(*e))||fail();
    }
    bool clear(){for(auto& e:entries_)if(e.id&&!erase(e))return false;return true;}
};
}
#endif
