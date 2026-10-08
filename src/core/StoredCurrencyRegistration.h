#ifndef DOWNWELL_CORE_STORED_CURRENCY_REGISTRATION_H
#define DOWNWELL_CORE_STORED_CURRENCY_REGISTRATION_H
#include "CurrencyCreate.h"
#include "InstanceStore.h"
#include "InstanceLifecycle.h"
#include "ObjectPack.h"
#include "SequenceTimeline.h"
#include "SoundPlayScript.h"
#include "CurrencyStepEnvelope.h"
#include "TimeStopBound.h"
#include <cstring>
namespace downwell {
// Registered object-248 Create and script setters. Borrow at a fixed address
// and clear before room teardown. This does not replace the provisional gem
// motion array: animation, Step and Draw must be bound before live admission.
template<unsigned WorldCapacity,unsigned Capacity> class StoredCurrencyRegistration {
    struct Entry {InstanceHandle key{};unsigned id=0;CurrencyCreate state;};
    Entry entries_[Capacity]{};
    InstanceStore<WorldCapacity>& records_;const ObjectPack* objects_;
    InstanceLifecycleBinding lifecycle_;std::uint32_t& nextId_;const int& room_;bool& good_;
    bool fail(){good_=false;return false;}
    static float stored(RunnerImageValue v){float f;std::memcpy(&f,&v.bits,4);return f;}
    Entry* entry(InstanceHandle h){
        for(auto& e:entries_)if(e.id&&e.key.slot==h.slot&&e.key.generation==h.generation)return &e;
        return nullptr;
    }
    InstanceRecord* record(const Entry& e){
        auto* r=records_.resolve(e.key);return r&&r->instanceId==e.id&&r->objectId==248?r:nullptr;
    }
    bool publish(Entry& e,RunnerImageValue x,RunnerImageValue y){
        auto* r=record(e);std::int32_t px,py;
        if(!r||!x.promoted().roundPixel(px)||!y.promoted().roundPixel(py)||
            !lifecycle_.publishedPosition(e.key,*r,x,y))return fail();
        r=record(e);if(!r)return fail();
        r->x=px;r->y=py;r->preciseX=stored(x);r->preciseY=stored(y);return true;
    }
public:
    StoredCurrencyRegistration(InstanceStore<WorldCapacity>& records,const ObjectPack* objects,
        InstanceLifecycleBinding lifecycle,std::uint32_t& nextId,const int& room,bool& good)
        :records_(records),objects_(objects),lifecycle_(lifecycle),nextId_(nextId),room_(room),good_(good){}
    StoredCurrencyRegistration(const StoredCurrencyRegistration&)=delete;
    StoredCurrencyRegistration& operator=(const StoredCurrencyRegistration&)=delete;
    bool belongsTo(const InstanceStore<WorldCapacity>& records)const{return &records_==&records;}
    bool valid(){if(!good_)return false;for(auto& e:entries_)if(e.id&&!record(e))return false;return true;}
    InstanceHandle handle(const InstanceRecord& r){
        for(auto& e:entries_)if(e.id==r.instanceId&&record(e)==&r)return e.key;return {};
    }
    const CurrencyCreate* find(InstanceHandle h){auto* e=entry(h);return e&&record(*e)?&e->state:nullptr;}
    bool advanceAnimation(InstanceHandle h){
        auto* e=entry(h);auto* r=e?record(*e):nullptr;
        if(!good_||!r||(r->spriteId!=133&&r->spriteId!=134))return fail();
        RunnerImageValue index,speed;std::memcpy(&index.bits,&r->imageIndex,4);std::memcpy(&speed.bits,&r->imageSpeed,4);
        bool crossed=false;
        if(!SequenceTimeline::advance(index,speed,4,crossed))return fail();
        index.bits&=0x7fffffffu;
        if(!lifecycle_.imageIndex(h,*r,index))return fail();
        r=record(*e);if(!r)return fail();r->imageIndex=stored(index);return true;
    }
    // The global alarm pass owns countdowns. Import the record bank before
    // each event so Alarm 0's new Alarm 1 is visible to that same pass.
    template<class Event> bool dispatch(InstanceHandle h,Event event){
        auto* e=entry(h);auto* r=e?record(*e):nullptr;
        if(!good_||!r)return fail();e->state.alarms=r->alarms;
        const bool ok=event(e->state,good_);
        r=record(*e);if(!r)return fail();r->alarms=e->state.alarms;r->imageSpeed=stored(e->state.imageSpeed);
        return (ok&&good_)||fail();
    }
    template<class Event> bool alarm(InstanceHandle h,unsigned index,Event other){
        return dispatch(h,[&](auto& state,bool& good){
            if(index>=5||state.alarms.get(index)!=0)return false;
            if(index==0){state.obtainable=true;state.alarms.set(1,state.obtime);return true;}
            if(index==1){state.disappearing=true;state.alarms.set(2,state.distime);return true;}
            if(index==3){state.suckable=true;return true;}
            // Expiry needs explicit audio/environment inputs; Alarm 4 needs
            // its movement adapter. Absence is not successful dispatch.
            return other(state,index,good);
        });
    }
    bool mark(InstanceHandle h){
        auto* e=entry(h);auto* r=e?record(*e):nullptr;
        if(!good_||!r||!lifecycle_.markDestroyed(h,*r))return fail();r->destroyRequested=true;return true;
    }
    template<class Audio> bool expire(InstanceHandle h,MotionBinary64 volume,bool inWater,Audio& audio){
        auto* e=entry(h);auto* r=e?record(*e):nullptr;
        if(!good_||!r)return fail();
        if(e->state.attracted)return true;
        SoundPlayEnvironment env;env.gameVolume=volume;env.inWater=inWater;int voice=-1;
        if(!SoundPlayScript::plain(r->sound,env,audio,166,30,false,true,voice,good_))return false;
        return mark(h);
    }
    template<class Random,class Motion> bool step(InstanceHandle h,Random& random,bool stopped,int roomWidth,Motion& motion){
        auto* e=entry(h);if(!good_||!e||!record(*e))return fail();
        struct Ops {
            StoredCurrencyRegistration& owner;Entry& e;bool stopped;int width;Motion& motion;
            bool wall(MotionBinary64 x,MotionBinary64 y,bool& hit){
                auto* r=owner.record(e);return r&&owner.lifecycle_.placeMeeting(e.key,*r,57,x,y,hit);
            }
            bool timeAllowed(bool& allowed){
                return TimeStopBound::evaluate(stopped,e.state.x.promoted(),e.state.y.promoted(),width,
                    [&](auto x,auto y,bool& hit){auto* r=owner.record(e);return r&&owner.lifecycle_.placeMeeting(e.key,*r,82,x,y,hit);},allowed);
            }
            bool destroy(){return owner.mark(e.key);}
            bool freeMotion(CurrencyCreate& state){return motion.freeMotion(state);}
            bool attractedMotion(CurrencyCreate& state){return motion.attractedMotion(state);}
            bool publish(RunnerImageValue x,RunnerImageValue y){return owner.publish(e,x,y);}
        } ops{*this,*e,stopped,roomWidth,motion};
        return dispatch(h,[&](auto& state,bool& good){return CurrencyStepEnvelope::run(state,random,ops,good);});
    }
    bool retire(InstanceHandle h){
        auto* e=entry(h);
        if(!e||!record(*e)||!records_.erase(e->id,[&](auto key,const auto& r){return lifecycle_.retire(key,r);}))return fail();
        *e={};return true;
    }
    template<class Random,class Tile> bool create(MotionBinary64 x,MotionBinary64 y,int area,bool attractor,
        Random& random,Tile tile,InstanceHandle& output){
        if(!good_)return false;
        Entry* free=nullptr;for(auto& e:entries_)if(!e.id&&!free)free=&e;
        PackedObject definition;RunnerImageValue px,py;std::int32_t ix,iy;
        if(!free||!objects_||!objects_->findObject(248,definition)||!nextId_||nextId_>0x7fffffffu||
            !px.assign(x)||!py.assign(y)||!px.promoted().roundPixel(ix)||!py.promoted().roundPixel(iy))return fail();
        if(records_.create(nextId_,248,room_,definition.persistent(),definition.creationDepth)!=CreateInstanceResult::success)return fail();
        free->id=nextId_++;free->key=records_.handleAt(records_.size()-1);free->state={};
        auto* r=record(*free);if(!r)return fail();
        r->x=ix;r->y=iy;r->preciseX=stored(px);r->preciseY=stored(py);
        r->spriteId=definition.spriteId;r->maskId=definition.textureMaskId;
        if(!lifecycle_.birthAt(free->key,*r,px,py))return fail();
        struct Queries {
            StoredCurrencyRegistration& owner;Entry& e;Tile& tiles;
            bool wall(MotionBinary64 x,MotionBinary64 y,bool& hit){
                auto* r=owner.record(e);
                return (r&&owner.lifecycle_.placeMeeting(e.key,*r,57,x,y,hit))||owner.fail();
            }
            bool tile(MotionBinary64 x,MotionBinary64 y,bool& hit){return tiles(e.key,100,x,y,hit);}
            bool publish(RunnerImageValue x,RunnerImageValue y){return owner.publish(e,x,y);}
        } queries{*this,*free,tile};
        const bool created=free->state.create(x,y,area,attractor,random,queries);
        r=record(*free);if(!r)return fail();
        r->imageSpeed=stored(free->state.imageSpeed);r->alarms=free->state.alarms;
        if(!created||!good_)return fail();
        output=free->key;return true;
    }
    bool amount(InstanceHandle h,int n){auto* e=entry(h);if(!good_||!e||!record(*e))return fail();e->state.getAmount=n;return true;}
    bool size(InstanceHandle h,int n){auto* e=entry(h);if(!good_||!e||!record(*e))return fail();e->state.size=n;return true;}
    bool attracted(InstanceHandle h,bool n){auto* e=entry(h);if(!good_||!e||!record(*e))return fail();e->state.attracted=n;return true;}
    bool clear(){
        for(auto& e:entries_)if(e.id){
            if(!record(e)||!records_.erase(e.id,[&](auto h,const auto& r){return lifecycle_.retire(h,r);}))return fail();
            e={};
        }
        return true;
    }
};
}
#endif
