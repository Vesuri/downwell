#ifndef DOWNWELL_CORE_STORED_LIMBO_SHARD_REGISTRATION_H
#define DOWNWELL_CORE_STORED_LIMBO_SHARD_REGISTRATION_H
#include "LimboShard.h"
#include "EffectSequenceAnimation.h"
#include "TimeStopBound.h"
#include "InstanceStore.h"
#include "InstanceLifecycle.h"
#include "ObjectPack.h"
#include <cstring>
namespace downwell {
// Borrowed fixed-address owner for object 379. Birth uses the enclosing
// room's source-ID allocator, before synchronous Create consumes any RNG.
template<unsigned WorldCapacity,unsigned Capacity> class StoredLimboShardRegistration {
    static_assert(Capacity>0,"nonempty limbo shard capacity required");
    struct Entry {InstanceHandle key{};std::uint32_t id=0;LimboShard state;RunnerImageValue timeline{};};
    Entry entries_[Capacity]{};
    InstanceStore<WorldCapacity>& records_;const ObjectPack* objects_;InstanceLifecycleBinding lifecycle_;
    std::uint32_t& nextId_;const int& room_;bool& good_;bool dispatching_=false;
    bool fail(){good_=false;return false;}
    static float stored(RunnerImageValue v){float f;std::memcpy(&f,&v.bits,4);return f;}
    Entry* entry(InstanceHandle h){
        for(auto& e:entries_)if(e.id&&e.key.slot==h.slot&&e.key.generation==h.generation)return &e;
        return nullptr;
    }
    InstanceRecord* record(const Entry& e){
        auto* r=records_.resolve(e.key);return r&&r->instanceId==e.id&&r->objectId==379?r:nullptr;
    }
    struct Publish {
        StoredLimboShardRegistration& owner;Entry& e;
        bool speed(RunnerImageValue v){auto* r=owner.record(e);if(!r)return owner.fail();r->imageSpeed=stored(v);return true;}
        bool image(RunnerImageValue v){
            auto* r=owner.record(e);if(!r||!owner.lifecycle_.imageIndex(e.key,*r,v))return owner.fail();
            r->imageIndex=stored(v);return true;
        }
        bool positionX(RunnerImageValue v){return position(v,e.state.y);}
        bool positionY(RunnerImageValue v){return position(e.state.x,v);}
        bool position(RunnerImageValue x,RunnerImageValue y){
            auto* r=owner.record(e);std::int32_t ix,iy;
            if(!r||!x.promoted().roundPixel(ix)||!y.promoted().roundPixel(iy)||
                !owner.lifecycle_.publishedPosition(e.key,*r,x,y))return owner.fail();
            r->x=ix;r->y=iy;r->preciseX=stored(x);r->preciseY=stored(y);return true;
        }
        bool alarm(unsigned index,int ticks){auto* r=owner.record(e);if(!r)return owner.fail();r->alarms.set(index,ticks);return true;}
        bool destroy(){
            auto* r=owner.record(e);if(!r||!owner.lifecycle_.markDestroyed(e.key,*r))return owner.fail();
            r->destroyRequested=true;return true;
        }
        bool meeting(MotionBinary64 x,MotionBinary64 y,int object,bool& hit){
            auto* r=owner.record(e);
            return (r&&owner.lifecycle_.placeMeeting(e.key,*r,object,x,y,hit))||owner.fail();
        }
        bool population(unsigned& count){return owner.lifecycle_.instanceNumber(379,count)||owner.fail();}
    };
    struct Dispatch {
        StoredLimboShardRegistration& owner;
        explicit Dispatch(StoredLimboShardRegistration& o):owner(o){owner.dispatching_=true;}
        ~Dispatch(){owner.dispatching_=false;}
    };
public:
    StoredLimboShardRegistration(InstanceStore<WorldCapacity>& records,const ObjectPack* objects,
        InstanceLifecycleBinding lifecycle,std::uint32_t& nextId,const int& room,bool& good)
        :records_(records),objects_(objects),lifecycle_(lifecycle),nextId_(nextId),room_(room),good_(good){}
    StoredLimboShardRegistration(const StoredLimboShardRegistration&)=delete;
    StoredLimboShardRegistration& operator=(const StoredLimboShardRegistration&)=delete;
    bool valid()const{return good_;}
    bool belongsTo(const InstanceStore<WorldCapacity>& records)const{return &records_==&records;}
    const LimboShard* state(InstanceHandle h){auto* e=entry(h);return e&&record(*e)?&e->state:nullptr;}
    InstanceHandle handle(const InstanceRecord& r){
        for(auto& e:entries_)if(e.id==r.instanceId&&record(e)==&r)return e.key;
        return {};
    }
    template<class Random> bool create(MotionBinary64 x,MotionBinary64 y,bool lowSpec,Random& random,InstanceHandle& output){
        output={};if(!good_||dispatching_)return fail();
        Entry* free=nullptr;for(auto& e:entries_)if(!e.id&&!free)free=&e;
        PackedObject definition;RunnerImageValue px,py;std::int32_t ix,iy;
        if(!free||!objects_||!objects_->findObject(379,definition)||definition.spriteId!=91||
            !nextId_||nextId_>0x7fffffffu||!px.assign(x)||!py.assign(y)||
            !px.promoted().roundPixel(ix)||!py.promoted().roundPixel(iy))return fail();
        Dispatch guard(*this);
        if(records_.create(nextId_,379,room_,definition.persistent(),definition.creationDepth)!=CreateInstanceResult::success)return fail();
        *free={};free->id=nextId_++;free->key=records_.handleAt(records_.size()-1);
        free->state.x=px;free->state.y=py;
        auto* r=record(*free);if(!r)return fail();
        r->x=ix;r->y=iy;r->preciseX=stored(px);r->preciseY=stored(py);
        r->spriteId=definition.spriteId;r->maskId=definition.textureMaskId;
        Publish publish{*this,*free};
        if(!lifecycle_.birthAt(free->key,*r,px,py)||!free->state.create(x,y,lowSpec,random,publish)||!good_)return fail();
        output=free->key;return true;
    }
    // Views reads current view state, including after a destruction mark.
    // outside implements checkOutOfViewV(0,0), not scrOutofview's margins.
    template<class Random,class Views> bool step(InstanceHandle key,bool stopped,int width,
        const LimboShard::Player& player,Random& random,Views& views){
        auto* e=entry(key);auto* r=e?record(*e):nullptr;
        if(!good_||dispatching_||!r)return fail();if(!r->active||r->destroyRequested)return true;
        Dispatch guard(*this);
        struct Environment:Publish {
            Views& views;
            Environment(StoredLimboShardRegistration& o,Entry& e,Views& v):Publish{o,e},views(v){}
            bool outside(RunnerImageValue x,RunnerImageValue y,bool& outside){return views.outside(x,y,outside);}
            bool viewTop(MotionBinary64& y){return views.viewTop(y);}
        } env{*this,*e,views};
        bool allowed=false;
        if(!TimeStopBound::evaluate(stopped,e->state.x.promoted(),e->state.y.promoted(),width,
            [&](auto x,auto y,bool& hit){return env.meeting(x,y,82,hit);},allowed))return fail();
        e->state.alarm2=r->alarms.get(2);
        return (e->state.step(allowed,player,random,env)&&good_)||fail();
    }
    template<class Random> bool bulletContact(InstanceHandle key,MotionBinary64 xsp,MotionBinary64 ysp,Random& random){
        auto* e=entry(key);auto* r=e?record(*e):nullptr;
        if(!good_||dispatching_||!r)return fail();if(!r->active||r->destroyRequested)return true;
        Dispatch guard(*this);Publish publish{*this,*e};e->state.alarm2=r->alarms.get(2);
        return (e->state.bulletContact(xsp,ysp,random,publish)&&good_)||fail();
    }
    bool advanceAnimation(InstanceHandle key){
        auto* e=entry(key);auto* r=e?record(*e):nullptr;
        if(!good_||dispatching_||!r||r->spriteId!=91)return fail();if(!r->active||r->destroyRequested)return true;
        Dispatch guard(*this);Publish publish{*this,*e};
        std::memcpy(&e->state.imageSpeed.bits,&r->imageSpeed,4);std::memcpy(&e->state.imageIndex.bits,&r->imageIndex,4);
        return (EffectSequenceAnimation::advance(e->timeline,e->state.imageSpeed,6,e->state.imageIndex)&&
            publish.image(e->state.imageIndex)&&good_)||fail();
    }
    bool activate(InstanceHandle key,bool active){
        auto* e=entry(key);auto* r=e?record(*e):nullptr;
        if(!good_||dispatching_||!r||r->destroyRequested)return fail();
        Dispatch guard(*this);
        if(!lifecycle_.activation(key,*r,active)||!good_)return fail();
        r->active=active;return true;
    }
    bool retire(InstanceHandle key){
        auto* e=entry(key);auto* r=e?record(*e):nullptr;
        if(!good_||dispatching_||!r||!r->destroyRequested||!e->state.destroyRequested)return fail();
        return erase(*e);
    }
    bool clear(){if(!good_||dispatching_)return fail();for(auto& e:entries_)if(e.id&&!erase(e))return false;return true;}
private:
    bool erase(Entry& e){
        Dispatch guard(*this);
        if(!record(e)||!records_.erase(e.id,[&](auto h,const auto& r){return lifecycle_.retire(h,r);}))return fail();
        e={};return true;
    }
};
}
#endif
