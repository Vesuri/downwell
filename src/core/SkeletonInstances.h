#ifndef DOWNWELL_CORE_SKELETON_INSTANCES_H
#define DOWNWELL_CORE_SKELETON_INSTANCES_H
#include "SkeletonActor.h"
#include "RegisteredSkeletonThrow.h"
#include "InstanceLifecycle.h"
namespace downwell {
// Fixed, generation-addressed ownership. Source numeric IDs are allocated by
// the room, never derived from a local slot. Engine active != actor.active.
template<unsigned Capacity> class SkeletonInstances {
    static_assert(Capacity>0,"nonempty skeleton storage required");
public:
    struct Record {
        unsigned lifetimeId=0,sourceId=0;
        static constexpr int objectId=120;
        RunnerImageValue x{},y{};
        SkeletonActor state;
        RegisteredSkeletonThrow throwing;
        bool active=true,destroyRequested=false;
    };
    using Binding=RecordLifecycleBinding<Record>;
    explicit SkeletonInstances(Binding binding):binding_(binding){}
    SkeletonInstances(const SkeletonInstances&)=delete;
    SkeletonInstances& operator=(const SkeletonInstances&)=delete;
    bool valid()const{return good_;}
    unsigned size()const{return count_;}
    const Record* find(InstanceHandle key)const{
        return key.generation&&key.slot<Capacity&&records_[key.slot].lifetimeId==key.generation?&records_[key.slot]:nullptr;
    }
    bool create(unsigned id,int x,int y,InstanceHandle& result){
        if(!good_)return false;
        if(!id||id>0x7fffffffu||!next_||executing_.generation||!binding_.hasOwner())return fail();
        unsigned slot=Capacity;
        for(unsigned i=0;i<Capacity;++i){
            if(records_[i].lifetimeId&&records_[i].sourceId==id)return fail();
            if(!records_[i].lifetimeId&&slot==Capacity)slot=i;
        }
        if(slot==Capacity)return fail();
        Record birth;
        if(!birth.x.assignInteger(x)||!birth.y.assignInteger(y))return fail();
        birth.lifetimeId=next_++;birth.sourceId=id;
        records_[slot]=birth;++count_;result={slot,birth.lifetimeId};executing_=result;
        auto& r=records_[slot];
        // Publish constructor geometry before inherited move_snap. The native
        // slot survives a failed publication; the owning simulation must stop.
        bool ok=binding_.birthAt(result,r,r.x,r.y)&&good_;
        SkeletonActor created;
        if(ok)ok=created.create(x,y);
        RunnerImageValue snappedX,snappedY;
        if(ok)ok=snappedX.assignInteger(created.x)&&snappedY.assignInteger(created.y);
        if(ok)ok=publishPosition(result,r,snappedX,r.y);
        if(ok)ok=publishPosition(result,r,r.x,snappedY);
        if(ok)r.state=created;
        executing_={};return good_=good_&&ok;
    }
    bool activate(InstanceHandle key,bool active){
        auto* r=resolve(key);
        if(!r||r->destroyRequested||!binding_.activation(key,*r,active))return fail();
        r->active=active;return true;
    }
    bool markDestroyed(InstanceHandle key){
        auto* r=resolve(key);if(!r)return fail();
        if(r->destroyRequested)return true;
        if(!binding_.markDestroyed(key,*r))return fail();
        r->destroyRequested=true;r->state.destroyRequested=true;return true;
    }
    template<class Event> bool dispatch(InstanceHandle key,Event event){
        auto* r=resolve(key);if(!r||executing_.generation)return fail();
        if(!r->active||r->destroyRequested)return true;
        executing_=key;const bool ok=event(r->state,r->throwing,good_);
        executing_={};return good_=good_&&ok;
    }
    bool advanceAnimation(InstanceHandle key){
        auto* r=resolve(key);if(!r||executing_.generation)return fail();
        auto next=static_cast<const SkeletonAnimation&>(r->state);bool crossed;RunnerImageValue index;
        executing_=key;
        const bool ok=next.advanceImage(crossed)&&index.assign(next.imageIndex)&&binding_.imageIndex(key,*r,index);
        if(ok&&good_){r->state.timeline=next.timeline;r->state.imageIndex=next.imageIndex;}
        executing_={};return good_=good_&&ok;
    }
    bool advanceAlarm(InstanceHandle key,unsigned index){
        if(index>=4)return fail();
        return dispatch(key,[&](auto& state,auto&,bool&){
            if(!state.advanceAlarm(index)||index!=2)return true;
            return binding_.imageIndex(key,*find(key),{});
        });
    }
    bool retire(InstanceHandle key){
        auto* r=resolve(key);
        if(!r||!r->destroyRequested||executing_.generation||!binding_.retire(key,*r))return fail();
        *r={};--count_;return true;
    }
    bool clear(){
        if(executing_.generation)return fail();
        for(unsigned i=0;i<Capacity;++i)if(records_[i].lifetimeId){
            if(!binding_.retire({i,records_[i].lifetimeId},records_[i]))return fail();
            records_[i]={};--count_;
        }
        good_=true;return true;
    }
private:
    bool publishPosition(InstanceHandle key,Record& r,RunnerImageValue x,RunnerImageValue y){
        if(!binding_.publishedPosition(key,r,x,y))return false;r.x=x;r.y=y;return true;
    }
    bool fail(){good_=false;return false;}
    Record* resolve(InstanceHandle key){return good_?const_cast<Record*>(find(key)):nullptr;}
    Record records_[Capacity]{};Binding binding_;
    unsigned next_=1,count_=0;bool good_=true;InstanceHandle executing_{};
};
}
#endif
