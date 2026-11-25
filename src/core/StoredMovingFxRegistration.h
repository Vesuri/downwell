#ifndef DOWNWELL_CORE_STORED_MOVING_FX_REGISTRATION_H
#define DOWNWELL_CORE_STORED_MOVING_FX_REGISTRATION_H
#include "MovingFx.h"
#include "InstanceStore.h"
#include "InstanceLifecycle.h"
#include "ObjectPack.h"
#include "TimeStopBound.h"
#include <cstring>
namespace downwell {
// Object 220's source identity and synchronous Create/setter publication.
// Keep the owner alive until its registered room lifetimes have retired.
template<unsigned WorldCapacity,unsigned Capacity> class StoredMovingFxRegistration {
    struct Entry {InstanceHandle key{};unsigned id=0;MovingFx state;};
    Entry entries_[Capacity]{};
    InstanceStore<WorldCapacity>& records_;const ObjectPack* objects_;
    InstanceLifecycleBinding lifecycle_;unsigned& nextId_;const int& room_;bool& good_;
    bool fail(){good_=false;return false;}
    static float stored(RunnerImageValue value){float f;std::memcpy(&f,&value.bits,4);return f;}
    Entry* entry(InstanceHandle key){
        for(auto& e:entries_)if(e.id&&e.key.slot==key.slot&&e.key.generation==key.generation){
            const auto* r=records_.resolve(key);
            if(r&&r->objectId==220&&r->instanceId==e.id)return &e;
        }
        return nullptr;
    }
public:
    StoredMovingFxRegistration(InstanceStore<WorldCapacity>& records,const ObjectPack* objects,
        InstanceLifecycleBinding lifecycle,unsigned& nextId,const int& room,bool& good)
        :records_(records),objects_(objects),lifecycle_(lifecycle),nextId_(nextId),room_(room),good_(good){}
    StoredMovingFxRegistration(const StoredMovingFxRegistration&)=delete;
    StoredMovingFxRegistration& operator=(const StoredMovingFxRegistration&)=delete;
    bool belongsTo(const InstanceStore<WorldCapacity>& records)const{return &records_==&records;}
    bool valid()const{return good_;}
    InstanceHandle handle(const InstanceRecord& record){
        for(auto& e:entries_)if(e.id==record.instanceId&&records_.resolve(e.key)==&record)return e.key;
        return {};
    }
    const MovingFx* state(InstanceHandle key){auto* e=entry(key);return e?&e->state:nullptr;}
    unsigned sourceId(InstanceHandle key){auto* e=entry(key);return e?e->id:0;}
    bool create(MotionBinary64 x,MotionBinary64 y,InstanceHandle& output){
        output={};if(!good_)return false;
        Entry* free=nullptr;for(auto& e:entries_)if(!e.id&&!free)free=&e;
        PackedObject definition;RunnerImageValue px,py;std::int32_t ix,iy;
        if(!free||!objects_||!objects_->findObject(220,definition)||definition.parentId!=-1||definition.spriteId!=-1||
           !nextId_||nextId_>0x7fffffffu||!px.assign(x)||!py.assign(y)||
           !px.roundPixel(ix)||!py.roundPixel(iy))return fail();
        if(records_.create(nextId_,220,room_,definition.persistent(),definition.creationDepth)!=CreateInstanceResult::success)return fail();
        *free={};free->id=nextId_++;free->key=records_.handleAt(records_.size()-1);
        auto* r=records_.resolve(free->key);if(!r)return fail();
        r->x=ix;r->y=iy;r->preciseX=stored(px);r->preciseY=stored(py);
        r->spriteId=definition.spriteId;r->maskId=definition.textureMaskId;
        if(!lifecycle_.birthAt(free->key,*r,px,py)||!free->state.create(px,py)||!good_)return fail();
        output=free->key;return true;
    }
    bool sprite(InstanceHandle key,int id,unsigned frames){
        auto* e=entry(key);if(!good_||!e)return fail();
        return e->state.setSprite(id,frames,[&](const auto& state){
            auto* r=records_.resolve(key);
            if(!r||!lifecycle_.sprite(key,*r,state.spriteId))return fail();
            r=records_.resolve(key);if(!r)return fail();r->spriteId=state.spriteId;
            if(!lifecycle_.imageIndex(key,*r,state.imageIndex))return fail();
            r=records_.resolve(key);if(!r)return fail();r->imageIndex=stored(state.imageIndex);return true;
        })||fail();
    }
    bool imageSpeed(InstanceHandle key,MotionBinary64 value){
        auto* e=entry(key);RunnerImageValue storedValue;
        if(!good_||!e||!storedValue.assign(value))return fail();
        records_.resolve(key)->imageSpeed=stored(storedValue);e->state.imageSpeed=storedValue;return true;
    }
    bool imageAngle(InstanceHandle key,MotionBinary64 value){
        auto* e=entry(key);RunnerImageValue angle;
        if(!good_||!e||!angle.assign(value)||!lifecycle_.transform(key,*records_.resolve(key),
            RunnerTransformField::angle,angle,{0x3f800000u},0xffffffu))return fail();
        auto* r=records_.resolve(key);if(!r)return fail();r->rotation=stored(angle);e->state.angle=angle;return true;
    }
    bool velocity(InstanceHandle key,bool vertical,MotionBinary64 value){
        auto* e=entry(key);if(!good_||!e||!value.valid())return fail();
        (vertical?e->state.ysp:e->state.xsp)=value;return true;
    }
    bool timeAllowed(InstanceHandle key,bool stopped,int width,bool& allowed){
        auto* e=entry(key);if(!good_||!e)return fail();
        return TimeStopBound::evaluate(stopped,e->state.x.promoted(),e->state.y.promoted(),width,
            [&](auto x,auto y,bool& hit){
                auto* r=records_.resolve(key);return r&&lifecycle_.placeMeeting(key,*r,82,x,y,hit);
            },allowed)||fail();
    }
    template<class Query> bool step(InstanceHandle key,Query query){
        auto* e=entry(key);if(!good_||!e)return fail();
        auto* r=records_.resolve(key);if(!r->active||r->destroyRequested)return true;
        std::memcpy(&e->state.imageSpeed.bits,&r->imageSpeed,4);
        struct Ops {
            StoredMovingFxRegistration& owner;Entry& e;Query& query;
            bool timeStopBound(bool& allowed){return query(e.key,e.state,allowed);}
            bool imageSpeed(RunnerImageValue value,RunnerImageValue& accepted){
                if(!owner.imageSpeed(e.key,value.promoted()))return false;accepted=value;return true;
            }
            bool position(bool vertical,RunnerImageValue value,RunnerImageValue& accepted){
                const auto x=vertical?e.state.x:value,y=vertical?value:e.state.y;
                auto* r=owner.records_.resolve(e.key);std::int32_t ix,iy;
                if(!r||!x.roundPixel(ix)||!y.roundPixel(iy)||
                    !owner.lifecycle_.publishedPosition(e.key,*r,x,y))return false;
                r=owner.records_.resolve(e.key);if(!r)return false;
                r->x=ix;r->y=iy;r->preciseX=stored(x);r->preciseY=stored(y);accepted=value;return true;
            }
        } ops{*this,*e,query};
        return e->state.step(ops)||fail();
    }
    bool advanceAnimation(InstanceHandle key){
        auto* e=entry(key);if(!good_||!e)return fail();
        auto* r=records_.resolve(key);if(!r->active||r->destroyRequested)return true;
        std::memcpy(&e->state.imageSpeed.bits,&r->imageSpeed,4);
        if(!e->state.advanceImage([&](const auto& state){
            auto* r=records_.resolve(key);
            if(!r||!lifecycle_.imageIndex(key,*r,state.imageIndex))return false;
            r=records_.resolve(key);if(!r)return false;r->imageIndex=stored(state.imageIndex);return true;
        }))return fail();
        if(e->state.destroyRequested){
            r=records_.resolve(key);if(!r||!lifecycle_.markDestroyed(key,*r))return fail();
            r=records_.resolve(key);if(!r)return fail();r->destroyRequested=true;
        }
        return true;
    }
    bool retire(InstanceHandle key){
        auto* e=entry(key);if(!good_||!e||!records_.resolve(key)->destroyRequested)return fail();
        if(!records_.erase(e->id,[&](auto h,const auto& r){return lifecycle_.retire(h,r);}))return fail();
        *e={};return true;
    }
};
}
#endif
