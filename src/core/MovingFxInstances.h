#ifndef DOWNWELL_CORE_MOVING_FX_INSTANCES_H
#define DOWNWELL_CORE_MOVING_FX_INSTANCES_H
#include "MovingFx.h"
#include "InstanceLifecycle.h"
#include "FailureSignal.h"
namespace downwell {
// Native object-220 ownership. No parallel generic room record; the caller
// supplies a source ID from the complete room allocation stream.
template<unsigned Capacity> class MovingFxInstances {
public:
    struct Record:MovingFx {
        unsigned sourceId=0,lifetimeId=0;
        bool active=true,busy=false;
        static constexpr int objectId=220;
    };
    using Binding=RecordLifecycleBinding<Record>;
    explicit MovingFxInstances(Binding binding):binding_(binding){}
    MovingFxInstances(const MovingFxInstances&)=delete;
    MovingFxInstances& operator=(const MovingFxInstances&)=delete;
    bool valid()const{return good_;}
    void observeFailure(bool& owner){good_.observe(owner);}
    unsigned size()const{return count_;}
    const Record* find(InstanceHandle key)const{
        return key.generation&&key.slot<Capacity&&records_[key.slot].lifetimeId==key.generation?&records_[key.slot]:nullptr;
    }
    bool create(unsigned source,MotionBinary64 x,MotionBinary64 y,InstanceHandle& output){
        output={};if(!good_||!binding_.hasOwner()||!source||source>0x7fffffffu||!next_)return fail();
        unsigned slot=Capacity;
        for(unsigned i=0;i<Capacity;++i){
            if(records_[i].lifetimeId&&records_[i].sourceId==source)return fail();
            if(!records_[i].lifetimeId&&slot==Capacity)slot=i;
        }
        RunnerImageValue px,py;
        if(slot==Capacity||!px.assign(x)||!py.assign(y))return fail();
        auto& r=records_[slot];r={};r.sourceId=source;r.lifetimeId=next_++;r.x=px;r.y=py;r.busy=true;++count_;
        const InstanceHandle key{slot,r.lifetimeId};
        const bool ok=binding_.birthAt(key,r,px,py)&&r.MovingFx::create(px,py);r.busy=false;
        if(!ok||!good_)return fail();output=key;return true;
    }
    bool sprite(InstanceHandle key,int sprite,unsigned frames){
        auto* r=resolve(key);if(!r)return fail();
        return r->setSprite(sprite,frames,[&](const auto& value){
            return binding_.sprite(key,*r,value.spriteId)&&binding_.imageIndex(key,*r,value.imageIndex);
        })||fail();
    }
    bool imageSpeed(InstanceHandle key,MotionBinary64 value){
        auto* r=resolve(key);RunnerImageValue speed;
        if(!r||!speed.assign(value))return fail();r->imageSpeed=speed;return true;
    }
    bool imageAngle(InstanceHandle key,MotionBinary64 value){
        auto* r=resolve(key);RunnerImageValue angle;
        if(!r||!angle.assign(value)||!binding_.transform(key,*r,RunnerTransformField::angle,angle,{0x3f800000u},0xffffffu))return fail();
        r->angle=angle;return true;
    }
    bool velocity(InstanceHandle key,bool vertical,MotionBinary64 value){
        auto* r=resolve(key);if(!r||!value.valid())return fail();(vertical?r->ysp:r->xsp)=value;return true;
    }
    template<class Query> bool step(InstanceHandle key,Query query){
        auto* r=resolve(key);if(!r)return fail();if(!r->active||r->destroyRequested)return true;
        r->busy=true;
        struct Ops {
            MovingFxInstances& owner;Record& r;InstanceHandle key;Query& query;
            bool timeStopBound(bool& allowed){return query(static_cast<const Record&>(r),allowed);}
            bool imageSpeed(RunnerImageValue value,RunnerImageValue& accepted){r.imageSpeed=value;accepted=value;return true;}
            bool position(bool vertical,RunnerImageValue value,RunnerImageValue& accepted){
                if(!owner.binding_.publishedPosition(key,r,vertical?r.x:value,vertical?value:r.y))return false;
                accepted=value;return true;
            }
        } ops{*this,*r,key,query};
        const bool ok=r->MovingFx::step(ops);r->busy=false;return (good_&&ok)||fail();
    }
    bool advanceAnimation(InstanceHandle key){
        auto* r=resolve(key);if(!r)return fail();if(!r->active||r->destroyRequested)return true;
        r->busy=true;
        const bool ok=r->advanceImage([&](const auto& value){return binding_.imageIndex(key,*r,value.imageIndex);});
        const bool ended=r->destroyRequested;r->destroyRequested=false;
        const bool marked=ok&&(!ended||binding_.markDestroyed(key,*r));
        if(marked&&ended)r->destroyRequested=true;
        r->busy=false;return (good_&&marked)||fail();
    }
    bool activate(InstanceHandle key,bool active){
        auto* r=resolve(key);if(!r||r->destroyRequested||!binding_.activation(key,*r,active))return fail();
        r->active=active;return true;
    }
    bool retire(InstanceHandle key){
        auto* r=resolve(key);if(!r||!r->destroyRequested||!binding_.retire(key,*r))return fail();
        *r={};--count_;return true;
    }
    bool clear(){
        if(!good_)return false;
        for(const auto& r:records_)if(r.busy)return fail();
        for(unsigned slot=0;slot<Capacity;++slot){
            auto& r=records_[slot];if(!r.lifetimeId)continue;
            const InstanceHandle key{slot,r.lifetimeId};
            if(!r.destroyRequested){
                if(!binding_.markDestroyed(key,r))return fail();
                r.destroyRequested=true;
            }
            if(!retire(key))return false;
        }
        return true;
    }
private:
    Record records_[Capacity]{};Binding binding_;unsigned next_=1,count_=0;FailureSignal<> good_=true;
    bool fail(){good_=false;return false;}
    Record* resolve(InstanceHandle key){
        return good_&&find(key)&&!records_[key.slot].busy?&records_[key.slot]:nullptr;
    }
};
}
#endif
