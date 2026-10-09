#ifndef DOWNWELL_CORE_FLASH_BALL_INSTANCES_H
#define DOWNWELL_CORE_FLASH_BALL_INSTANCES_H
#include "FlashBallEffects.h"
#include "InstanceLifecycle.h"
#include "FailureSignal.h"
namespace downwell {
// Object 224. Source IDs and phase order belong to the enclosing world;
// generation keys identify storage lifetimes only. Retirement is deferred.
template<unsigned Capacity> class FlashBallInstances {
    static_assert(Capacity>0,"nonempty flash capacity required");
public:
    struct Record {
        std::uint32_t lifetimeId=0,sourceId=0;
        RunnerImageValue x{},y{};
        FlashBallEffect state;
        bool active=true,destroyRequested=false;
        static constexpr int objectId=224;
    };
    using Binding=RecordLifecycleBinding<Record>;
    explicit FlashBallInstances(Binding binding):binding_(binding){}
    FlashBallInstances(const FlashBallInstances&)=delete;
    FlashBallInstances& operator=(const FlashBallInstances&)=delete;
    bool valid()const{return good_;}
    void observeFailure(bool& owner){good_.observe(owner);}
    unsigned size()const{return count_;}
    const Record* find(InstanceHandle key)const{
        return key.generation&&key.slot<Capacity&&records_[key.slot].lifetimeId==key.generation?&records_[key.slot]:nullptr;
    }
    InstanceHandle sourceHandle(std::uint32_t id)const{
        for(unsigned i=0;id&&i<Capacity;++i)
            if(records_[i].lifetimeId&&records_[i].sourceId==id)return {i,records_[i].lifetimeId};
        return {};
    }
private:
    struct Dispatch {
        FlashBallInstances& owner;
        explicit Dispatch(FlashBallInstances& o):owner(o){owner.dispatching_=true;}
        ~Dispatch(){owner.dispatching_=false;}
    };
    struct Publish {
        FlashBallInstances& owner;InstanceHandle key;Record& record;
        bool imageSpeed(RunnerImageValue){return owner.good_;} // Record owns speed.
        bool imageIndex(RunnerImageValue value){return owner.binding_.imageIndex(key,record,value)&&owner.good_;}
        bool position(RunnerImageValue x,RunnerImageValue y){
            if(!owner.binding_.publishedPosition(key,record,x,y)||!owner.good_)return false;
            record.x=x;record.y=y;return true;
        }
        bool destroy(){
            if(!owner.binding_.markDestroyed(key,record)||!owner.good_)return false;
            record.destroyRequested=true;return true;
        }
    };
public:
    template<class Random> bool create(std::uint32_t id,MotionBinary64 x,MotionBinary64 y,
                                      Random& random,InstanceHandle& output){
        output={};
        if(!good_||dispatching_||!next_||!id||id>0x7fffffffu||sourceHandle(id).generation)return fail();
        unsigned slot=0;while(slot<Capacity&&records_[slot].lifetimeId)++slot;
        RunnerImageValue px,py;if(slot==Capacity||!px.assign(x)||!py.assign(y))return fail();
        Dispatch guard(*this);
        auto& r=records_[slot];r={};r.lifetimeId=next_++;r.sourceId=id;r.x=px;r.y=py;
        r.state.x=px;r.state.y=py;r.state.imageIndex={};r.state.timeline={};
        r.state.imageSpeed={0x3f800000u};r.state.vrange=0;++count_;
        const InstanceHandle key{slot,r.lifetimeId};Publish publish{*this,key,r};
        if(!binding_.birthAt(key,r,px,py)||!good_||!r.state.createPublished(random,publish)||!good_)return fail();
        output=key;return true;
    }
    bool advanceAnimation(InstanceHandle key){
        auto* r=resolve(key);if(!r||dispatching_)return fail();
        if(!r->active||r->destroyRequested)return true;
        Dispatch guard(*this);Publish publish{*this,key,*r};
        return (r->state.advanceImage(publish)&&good_)||fail();
    }
    bool activate(InstanceHandle key,bool active){
        auto* r=resolve(key);if(!r||dispatching_||r->destroyRequested)return fail();
        Dispatch guard(*this);
        if(!binding_.activation(key,*r,active)||!good_)return fail();
        r->active=active;return true;
    }
    bool retire(InstanceHandle key){
        auto* r=resolve(key);
        if(!r||dispatching_||!r->destroyRequested||!r->state.destroyRequested)return fail();
        Dispatch guard(*this);if(!binding_.retire(key,*r)||!good_)return fail();
        *r={};--count_;return true;
    }
    bool clear(){
        if(!good_||dispatching_)return fail();Dispatch guard(*this);
        for(unsigned i=0;i<Capacity;++i)if(records_[i].lifetimeId){
            if(!binding_.retire({i,records_[i].lifetimeId},records_[i])||!good_)return fail();
            records_[i]={};--count_;
        }
        return true;
    }
private:
    Record* resolve(InstanceHandle key){return good_?const_cast<Record*>(find(key)):nullptr;}
    bool fail(){good_=false;return false;}
    Binding binding_;Record records_[Capacity]{};std::uint32_t next_=1;
    unsigned count_=0;FailureSignal<> good_=true;bool dispatching_=false;
};
}
#endif
