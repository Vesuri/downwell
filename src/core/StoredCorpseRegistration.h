#ifndef DOWNWELL_CORE_STORED_CORPSE_REGISTRATION_H
#define DOWNWELL_CORE_STORED_CORPSE_REGISTRATION_H
#include "CorpseAnimation.h"
#include "DeadBodyCreation.h"
#include "InstanceStore.h"
#include "InstanceLifecycle.h"
#include "ObjectPack.h"
#include <cstring>
namespace downwell {
// Object 263's shared state, registered in the enclosing room's ID stream.
// Keep this owner at a fixed address and clear it before room teardown.
template<unsigned WorldCapacity,unsigned Capacity> class StoredCorpseRegistration {
    static_assert(Capacity>0,"nonempty corpse registration required");
    struct Entry {InstanceHandle key{};unsigned id=0;CorpseAnimation state;};
    Entry entries_[Capacity]{};
    InstanceStore<WorldCapacity>& records_;const ObjectPack* objects_;
    InstanceLifecycleBinding lifecycle_;std::uint32_t& nextId_;const int& room_;bool& good_;
    bool dispatching_=false;
    bool fail(){good_=false;return false;}
    static float stored(RunnerImageValue v){float f;std::memcpy(&f,&v.bits,4);return f;}
    Entry* entry(InstanceHandle h){
        for(auto& e:entries_)if(e.id&&e.key.slot==h.slot&&e.key.generation==h.generation)return &e;
        return nullptr;
    }
    InstanceRecord* record(const Entry& e){
        auto* r=records_.resolve(e.key);return r&&r->instanceId==e.id&&r->objectId==263?r:nullptr;
    }
    bool builtins(Entry& e){
        auto* r=record(e);if(!r)return fail();
        r->imageSpeed=stored(e.state.imageSpeed);r->alarms=e.state.alarms;return true;
    }
    bool position(Entry& e,RunnerImageValue x,RunnerImageValue y){
        auto* r=record(e);std::int32_t ix,iy;
        if(!good_||!r||!x.roundPixel(ix)||!y.roundPixel(iy)||
            !lifecycle_.publishedPosition(e.key,*r,x,y))return fail();
        r->x=ix;r->y=iy;r->preciseX=stored(x);r->preciseY=stored(y);return true;
    }
public:
    StoredCorpseRegistration(InstanceStore<WorldCapacity>& records,const ObjectPack* objects,
        InstanceLifecycleBinding lifecycle,std::uint32_t& nextId,const int& room,bool& good)
        :records_(records),objects_(objects),lifecycle_(lifecycle),nextId_(nextId),room_(room),good_(good){}
    StoredCorpseRegistration(const StoredCorpseRegistration&)=delete;
    StoredCorpseRegistration& operator=(const StoredCorpseRegistration&)=delete;
    bool belongsTo(const InstanceStore<WorldCapacity>& records)const{return &records_==&records;}
    bool valid()const{return good_;}
    InstanceHandle handle(const InstanceRecord& r){
        for(auto& e:entries_)if(e.id==r.instanceId&&record(e)==&r)return e.key;
        return {};
    }
    const CorpseAnimation* state(InstanceHandle h){auto* e=entry(h);return e&&record(*e)?&e->state:nullptr;}
    bool readShootable(InstanceHandle key,MotionBinary64& health,bool& hitEffect){
        const auto* value=state(key);if(!good_||!value)return fail();
        health=value->health;hitEffect=value->hitEffect;return true;
    }
    bool writeShootableHealth(InstanceHandle key,MotionBinary64 health){
        auto* e=entry(key);if(!good_||!e||!record(*e)||!health.valid())return fail();
        e->state.health=health;return true;
    }
    bool writeShootableHit(InstanceHandle key){
        auto* e=entry(key);if(!good_||!e||!record(*e))return fail();e->state.hit=true;return true;
    }
    InstanceHandle sourceHandle(unsigned id){
        for(auto& e:entries_)if(id&&e.id==id&&record(e))return e.key;
        return {};
    }
    template<class Random,class Tile> bool create(MotionBinary64 x,MotionBinary64 y,bool rip,
        Random& random,Tile tile,InstanceHandle& output){
        output={};if(!good_||dispatching_)return fail();
        Entry* free=nullptr;for(auto& e:entries_)if(!e.id&&!free)free=&e;
        PackedObject definition;RunnerImageValue px,py;std::int32_t ix,iy;
        if(!free||!objects_||!objects_->findObject(263,definition)||!nextId_||nextId_>0x7fffffffu||
            !px.assign(x)||!py.assign(y)||!px.roundPixel(ix)||!py.roundPixel(iy))return fail();
        if(records_.create(nextId_,263,room_,definition.persistent(),definition.creationDepth)!=CreateInstanceResult::success)return fail();
        free->id=nextId_++;free->key=records_.handleAt(records_.size()-1);free->state={};
        free->state.x=px;free->state.y=py;
        auto* r=record(*free);if(!r)return fail();
        r->x=ix;r->y=iy;r->preciseX=stored(px);r->preciseY=stored(py);
        r->spriteId=definition.spriteId;r->maskId=definition.textureMaskId;
        if(!lifecycle_.birthAt(free->key,*r,px,py))return fail();
        struct Queries {
            StoredCorpseRegistration& owner;Entry& e;Tile& tiles;
            bool wall(MotionBinary64 x,MotionBinary64 y,bool& hit){
                auto* r=owner.record(e);bool result=false;
                if(!r||!owner.builtins(e)||!owner.lifecycle_.placeMeeting(e.key,*r,57,x,y,result))return owner.fail();
                hit=result;return true;
            }
            bool tile(MotionBinary64 x,MotionBinary64 y,bool& hit){
                bool result=false;if(!tiles(e.key,100,x,y,result))return owner.fail();
                hit=result;return true;
            }
            bool publish(RunnerImageValue x,RunnerImageValue y){return owner.position(e,x,y);}
        } queries{*this,*free,tile};
        if(!free->state.create(x,y,rip,random,queries)||!builtins(*free)||!good_)return fail();
        output=free->key;return true;
    }
    bool sprite(InstanceHandle h,int sprite){
        auto* e=entry(h);if(!good_||!e||!record(*e))return fail();
        return e->state.setSprite(sprite,[&](const auto& state){
            auto* r=record(*e);if(!r||!lifecycle_.sprite(h,*r,sprite))return fail();
            r->spriteId=sprite;r->imageIndex=stored(state.imageIndex);return true;
        })||fail();
    }
    bool facing(InstanceHandle h,MotionBinary64 value){
        auto* e=entry(h);auto* r=e?record(*e):nullptr;RunnerImageValue next;
        if(!good_||!r||!next.assign(value))return fail();
        const auto old=e->state.imageXscale.bits;
        if(next.bits==old||!((next.bits|old)&0x7fffffffu))return true;
        if(!lifecycle_.transform(h,*r,RunnerTransformField::scaleX,next,{0x3f800000u},0xffffffu))return fail();
        e->state.imageXscale=next;r->scaleX=stored(next);return true;
    }
    template<class Source,class Random,class Tile> bool createFromDeath(Source& source,int spriteId,MotionBinary64 impact,
        int explosion,bool rip,Random& random,Tile tile){
        if(!good_)return false;
        struct Events {
            StoredCorpseRegistration& owner;Random& random;Tile& tile;bool rip;
            bool create(int object,MotionBinary64 x,MotionBinary64 y,std::uint32_t& id){
                InstanceHandle h;if(object!=263||!owner.create(x,y,rip,random,tile,h))return false;
                auto* e=owner.entry(h);if(!e)return false;id=e->id;return true;
            }
            Entry* selected(unsigned id){auto* e=owner.entry(owner.sourceHandle(id));return owner.good_?e:nullptr;}
            bool sprite(unsigned id,int v){return owner.sprite(owner.sourceHandle(id),v);}
            bool facing(unsigned id,MotionBinary64 v){return owner.facing(owner.sourceHandle(id),v);}
            bool verticalSpeed(unsigned id,MotionBinary64 v){auto* e=selected(id);if(!e)return false;e->state.ysp=v;return true;}
            bool horizontalSpeed(unsigned id,MotionBinary64 v){auto* e=selected(id);if(!e)return false;e->state.xsp=v;return true;}
            bool explosion(unsigned id,int v){auto* e=selected(id);if(!e)return false;e->state.explosionFx=v;return true;}
        } events{*this,random,tile,rip};
        return createDeadBody(source,spriteId,impact,explosion,events)||fail();
    }
    bool advanceAnimation(InstanceHandle h){
        auto* e=entry(h);auto* r=e?record(*e):nullptr;
        if(!good_||!r||r->spriteId!=e->state.spriteId)return fail();
        auto next=e->state;
        std::memcpy(&next.imageIndex.bits,&r->imageIndex,4);
        std::memcpy(&next.imageSpeed.bits,&r->imageSpeed,4);
        if(!next.advanceImage()||!lifecycle_.imageIndex(h,*r,next.imageIndex))return fail();
        e->state.timeline=next.timeline;e->state.imageIndex=next.imageIndex;e->state.imageSpeed=next.imageSpeed;
        r->imageIndex=stored(next.imageIndex);return true;
    }
    class Publisher {
        StoredCorpseRegistration& owner_;InstanceHandle key_;
    public:
        Publisher(StoredCorpseRegistration& owner,InstanceHandle key):owner_(owner),key_(key){}
        bool position(RunnerImageValue x,RunnerImageValue y){
            auto* e=owner_.entry(key_);return (e&&owner_.position(*e,x,y))||owner_.fail();
        }
        bool meeting(int object,MotionBinary64 x,MotionBinary64 y,bool& hit){
            auto* e=owner_.entry(key_);auto* r=e?owner_.record(*e):nullptr;bool result=false;
            if(!owner_.good_||!r||!owner_.lifecycle_.placeMeeting(key_,*r,object,x,y,result))return owner_.fail();
            hit=result;return true;
        }
        bool destroy(){
            auto* e=owner_.entry(key_);auto* r=e?owner_.record(*e):nullptr;
            if(!owner_.good_||!r)return owner_.fail();
            if(!r->destroyRequested&&!owner_.lifecycle_.markDestroyed(key_,*r))return owner_.fail();
            r->destroyRequested=e->state.destroyRequested=true;return true;
        }
    };
    template<class Execute> bool dispatchPublished(InstanceHandle h,Execute execute){
        auto* e=entry(h);auto* r=e?record(*e):nullptr;
        if(!good_||!r||dispatching_)return fail();
        if(!r->active||r->destroyRequested)return true;
        dispatching_=true;
        struct Reset {bool& flag;~Reset(){flag=false;}} reset{dispatching_};
        if(r->spriteId!=e->state.spriteId)return fail();
        e->state.alarms=r->alarms;
        std::memcpy(&e->state.imageSpeed.bits,&r->imageSpeed,4);
        std::memcpy(&e->state.imageIndex.bits,&r->imageIndex,4);
        std::memcpy(&e->state.imageXscale.bits,&r->scaleX,4);
        std::memcpy(&e->state.x.bits,&r->preciseX,4);std::memcpy(&e->state.y.bits,&r->preciseY,4);
        Publisher publish(*this,h);
        const bool ok=execute(e->state,publish,good_);
        r=record(*e);if(!r)return fail();
        const bool published=stored(e->state.x)==r->preciseX&&stored(e->state.y)==r->preciseY&&
            e->state.spriteId==r->spriteId&&stored(e->state.imageIndex)==r->imageIndex&&
            stored(e->state.imageXscale)==r->scaleX&&e->state.destroyRequested==r->destroyRequested;
        return (builtins(*e)&&ok&&good_&&published&&e->state.valid)||fail();
    }
    bool endStep(InstanceHandle h,MotionBinary64 viewY,bool mainRoom){
        return dispatchPublished(h,[&](auto& state,auto& publish,bool& good){
            struct Events {Publisher& publish;bool& good;void destroy(){if(!publish.destroy())good=false;}} events{publish,good};
            return state.endStep(mainRoom,viewY,events)&&good;
        });
    }
    bool retire(InstanceHandle h){
        auto* e=entry(h);auto* r=e?record(*e):nullptr;
        if(!good_||dispatching_||!r||!r->destroyRequested||!e->state.destroyRequested||
            !records_.erase(e->id,[&](auto key,const auto& record){return lifecycle_.retire(key,record);}))return fail();
        *e={};return true;
    }
    bool clear(){
        if(dispatching_)return fail();
        for(auto& e:entries_)if(e.id){
            if(!record(e)||!records_.erase(e.id,[&](auto h,const auto& r){return lifecycle_.retire(h,r);}))return fail();
            e={};
        }
        return true;
    }
};
}
#endif
