#ifndef DOWNWELL_CORE_STORED_CORPSE_EXPIRY_REGISTRATION_H
#define DOWNWELL_CORE_STORED_CORPSE_EXPIRY_REGISTRATION_H
#include "CorpseExpiryEffect.h"
#include "InstanceStore.h"
#include "InstanceLifecycle.h"
#include "ObjectPack.h"
#include <cstring>
namespace downwell {
// fxJobutsu is a real room lifetime, not an effect-local numeric identity.
// Keep this bridge alive until its records are retired, including failed births.
template<unsigned WorldCapacity,unsigned Capacity> class StoredCorpseExpiryRegistration {
    static_assert(Capacity>0,"nonempty expiry registration required");
    struct Entry {InstanceHandle key{};unsigned id=0;CorpseExpiryEffect state;};
    Entry entries_[Capacity]{};
    InstanceStore<WorldCapacity>& records_;const ObjectPack* objects_;
    InstanceLifecycleBinding lifecycle_;std::uint32_t& nextId_;const int& room_;bool& good_;
    bool fail(){good_=false;return false;}
    static float stored(RunnerImageValue v){float f;std::memcpy(&f,&v.bits,4);return f;}
    Entry* entry(unsigned id){for(auto& e:entries_)if(id&&e.id==id)return &e;return nullptr;}
    InstanceRecord* record(const Entry& e){
        auto* r=records_.resolve(e.key);return r&&r->instanceId==e.id&&r->objectId==242?r:nullptr;
    }
    bool erase(Entry& e){
        if(!record(e)||!records_.erase(e.id,[&](auto h,const auto& r){return lifecycle_.retire(h,r);}))return fail();
        e={};return true;
    }
public:
    StoredCorpseExpiryRegistration(InstanceStore<WorldCapacity>& records,const ObjectPack* objects,
        InstanceLifecycleBinding lifecycle,std::uint32_t& nextId,const int& room,bool& good)
        :records_(records),objects_(objects),lifecycle_(lifecycle),nextId_(nextId),room_(room),good_(good){}
    StoredCorpseExpiryRegistration(const StoredCorpseExpiryRegistration&)=delete;
    StoredCorpseExpiryRegistration& operator=(const StoredCorpseExpiryRegistration&)=delete;
    bool belongsTo(const InstanceStore<WorldCapacity>& records)const{return &records_==&records;}
    bool valid()const{return good_;}
    const CorpseExpiryEffect* state(unsigned id){auto* e=entry(id);return e&&record(*e)?&e->state:nullptr;}
    bool create(MotionBinary64 x,MotionBinary64 y,std::uint32_t& output){
        output=0;if(!good_)return false;
        Entry* free=nullptr;for(auto& e:entries_)if(!e.id&&!free)free=&e;
        PackedObject definition;RunnerImageValue px,py;std::int32_t ix,iy;
        if(!free||!objects_||!objects_->findObject(242,definition)||definition.spriteId!=102||
            !nextId_||nextId_>0x7fffffffu||!px.assign(x)||!py.assign(y)||
            !px.roundPixel(ix)||!py.roundPixel(iy))return fail();
        if(records_.create(nextId_,242,room_,definition.persistent(),definition.creationDepth)!=CreateInstanceResult::success)return fail();
        free->id=nextId_++;free->key=records_.handleAt(records_.size()-1);free->state={};
        free->state.x=px;free->state.y=py;free->state.imageSpeed={0x3f800000u};
        auto* r=record(*free);if(!r)return fail();
        r->x=ix;r->y=iy;r->preciseX=stored(px);r->preciseY=stored(py);
        r->spriteId=definition.spriteId;r->maskId=definition.textureMaskId;
        if(!lifecycle_.birthAt(free->key,*r,px,py)||!free->state.create(x,y))return fail();
        r=record(*free);if(!r)return fail();r->imageSpeed=stored(free->state.imageSpeed);
        output=free->id;return true;
    }
    // The caller supplies audited consecutive unit-key sprite frame counts.
    bool sprite(unsigned id,int sprite,unsigned frames){
        auto* e=entry(id);auto* r=e?record(*e):nullptr;if(!good_||!r)return fail();
        std::memcpy(&e->state.imageIndex.bits,&r->imageIndex,4);
        return e->state.setSprite(sprite,frames,[&](const auto& state){
            if(!lifecycle_.sprite(e->key,*r,sprite))return fail();r->spriteId=sprite;
            if(!lifecycle_.imageIndex(e->key,*r,state.imageIndex))return fail();
            r->imageIndex=stored(state.imageIndex);return true;
        })||fail();
    }
    bool advanceAnimation(const InstanceRecord& source){
        auto* e=entry(source.instanceId);auto* r=e?record(*e):nullptr;
        if(!good_||r!=&source)return fail();if(!r->active||r->destroyRequested)return true;
        if(r->spriteId!=e->state.spriteId)return fail();
        std::memcpy(&e->state.imageIndex.bits,&r->imageIndex,4);
        std::memcpy(&e->state.imageSpeed.bits,&r->imageSpeed,4);
        if(!e->state.advanceImage([&](const auto& state){
            if(!lifecycle_.imageIndex(e->key,*r,state.imageIndex))return fail();
            r->imageIndex=stored(state.imageIndex);return true;
        }))return fail();
        if(e->state.destroyRequested){
            if(!lifecycle_.markDestroyed(e->key,*r))return fail();r->destroyRequested=true;
        }
        return true;
    }
    bool retire(const InstanceRecord& source){
        auto* e=entry(source.instanceId);auto* r=e?record(*e):nullptr;
        return (good_&&r==&source&&r->destroyRequested&&e->state.destroyRequested&&erase(*e))||fail();
    }
    bool clear(){for(auto& e:entries_)if(e.id&&!erase(e))return false;return true;}
    // Room alarm countdown has already happened. Complete child Create and
    // the source sprite override before requesting parent destruction.
    template<class Corpse,class Publish,class Frames>
    bool alarmEvent(Corpse& corpse,unsigned index,Publish& publish,Frames frames){
        if(!good_)return false;
        struct Events {
            StoredCorpseExpiryRegistration& owner;Publish& publish;Frames& frames;
            bool create(int object,RunnerImageValue x,RunnerImageValue y,std::uint32_t& id){
                return object==242&&owner.create(x.promoted(),y.promoted(),id);
            }
            bool sprite(unsigned id,int sprite){return owner.sprite(id,sprite,frames(sprite));}
            void destroy(){if(!publish.destroy())owner.fail();}
        } events{*this,publish,frames};
        return (corpse.alarmEvent(index,events)&&good_)||fail();
    }
};
}
#endif
