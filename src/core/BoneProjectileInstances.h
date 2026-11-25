#ifndef DOWNWELL_CORE_BONE_PROJECTILE_INSTANCES_H
#define DOWNWELL_CORE_BONE_PROJECTILE_INSTANCES_H
#include "BoneProjectile.h"
#include "InstanceLifecycle.h"
#include "CallbackFailureSignal.h"
namespace downwell {
// Registered ownership, separate from the scalar BoneProjectileStore oracle.
// The room allocates numeric IDs; local generations never stand in for them.
template<unsigned Capacity> class BoneProjectileInstances {
    static_assert(Capacity>0,"nonempty bone storage required");
public:
    struct Record {
        unsigned lifetimeId=0,sourceId=0;
        static constexpr int objectId=269;
        int spriteId=460;
        RunnerImageValue x{},y{};
        BoneProjectile state;
        // ScriptSpawnEffects lifetime, never a VM numeric ID. The stored
        // bridge additionally retains the actual child's source ID.
        unsigned myFxLocal=0,myFxSourceId=0;
        bool active=true,destroyRequested=false; // Engine flags, not state.active.
    };
    using Binding=RecordLifecycleBinding<Record>;
    struct Storage { Record records[Capacity]{}; };
    bool hasStorage()const{return records_!=nullptr;}
    bool bindStorage(Storage* storage){
        if(!good_||count_||executing_.generation)return fail();
        if(storage)for(const auto& record:storage->records)if(record.lifetimeId)return fail();
        records_=storage?storage->records:nullptr;return true;
    }
    explicit BoneProjectileInstances(Binding binding):binding_(binding){}
    BoneProjectileInstances(const BoneProjectileInstances&)=delete;
    BoneProjectileInstances& operator=(const BoneProjectileInstances&)=delete;
    bool valid()const{return good_;}
    void observeFailure(bool& owner,CallbackFailureScopes& scopes){good_.observe(owner,scopes);}
    unsigned size()const{return count_;}
    const Record* find(InstanceHandle h)const{
        return records_&&h.generation&&h.slot<Capacity&&records_[h.slot].lifetimeId==h.generation?&records_[h.slot]:nullptr;
    }
    template<class Random> bool create(unsigned sourceId,int x,int y,Random& random,InstanceHandle& result){
        if(!good_)return false;
        if(!records_||!binding_.hasOwner()||!sourceId||sourceId>0x7fffffffu||!next_||executing_.generation)return fail();
        unsigned slot=Capacity;
        for(unsigned i=0;i<Capacity;++i){
            if(records_[i].lifetimeId&&records_[i].sourceId==sourceId)return fail();
            if(!records_[i].lifetimeId&&slot==Capacity)slot=i;
        }
        if(slot==Capacity)return fail();
        Record birth;std::int32_t px,py;
        if(!birth.x.assignInteger(x)||!birth.y.assignInteger(y)||
            !birth.x.roundPixel(px)||!birth.y.roundPixel(py))return fail();
        birth.lifetimeId=next_++;birth.sourceId=sourceId;
        records_[slot]=birth;++count_;const InstanceHandle key{slot,birth.lifetimeId};result=key;
        executing_=key;
        auto& r=records_[slot];
        // Resource geometry and membership exist before the three Create draws.
        const bool ok=binding_.birthAt(key,r,r.x,r.y)&&good_&&r.state.create(px,py,random);
        executing_={};return good_=good_&&ok;
    }
    bool markDestroyed(InstanceHandle key){
        auto* r=resolve(key);if(!r)return fail();
        if(r->destroyRequested)return true;
        if(!binding_.markDestroyed(key,*r))return fail();
        r->destroyRequested=true;r->state.destroyRequested=true;return true;
    }
    bool assignEffect(InstanceHandle key,unsigned effectLocal,unsigned sourceId=0){
        auto* r=resolve(key);
        if(!r||!effectLocal||sourceId>0x7fffffffu)return fail();
        r->myFxLocal=effectLocal;r->myFxSourceId=sourceId;return true;
    }
    // Cross-instance VM field writes are not a bone event. They remain valid
    // while inactive or marked, including during synchronous child callbacks;
    // only retirement invalidates the selected lifetime. No RNG or membership
    // change accompanies objHp/hit assignment.
    bool writeHealth(InstanceHandle key,MotionBinary64 health){
        auto* r=resolve(key);if(!r||!health.valid())return fail();
        r->state.health=health;return true;
    }
    bool writeHit(InstanceHandle key){
        auto* r=resolve(key);if(!r)return fail();r->state.hit=true;return true;
    }
    bool activate(InstanceHandle key,bool active){
        auto* r=resolve(key);
        if(!r||r->destroyRequested||!binding_.activation(key,*r,active))return fail();
        r->active=active;return true;
    }
    class Publisher {
        BoneProjectileInstances& store_;InstanceHandle key_;
    public:
        Publisher(BoneProjectileInstances& store,InstanceHandle key):store_(store),key_(key){}
        bool imageSpeed(RunnerImageValue){return store_.resolve(key_)!=nullptr||store_.fail();}
        bool imageIndex(RunnerImageValue index){
            auto* r=store_.resolve(key_);
            return (r&&store_.binding_.imageIndex(key_,*r,index))||store_.fail();
        }
        bool position(RunnerImageValue x,RunnerImageValue y){
            auto* r=store_.resolve(key_);
            if(!r||!store_.binding_.publishedPosition(key_,*r,x,y))return store_.fail();
            r->x=x;r->y=y;return true;
        }
        bool mask(int mask){
            auto* r=store_.resolve(key_);return (r&&store_.binding_.mask(key_,*r,mask))||store_.fail();
        }
        bool destroy(){return store_.markDestroyed(key_);}
    };
    template<class Event> bool dispatch(InstanceHandle key,Event event){
        auto* r=resolve(key);if(!r||executing_.generation)return fail();
        if(!r->active||r->destroyRequested)return true;
        executing_=key;Publisher publish(*this,key);
        auto status=good_.borrow();
        const bool ok=event(r->state,publish,status.value());executing_={};return good_=good_&&ok;
    }
    bool advanceAnimation(InstanceHandle key){
        // The caller selected this lifetime from the reconciled room list.
        // Pending deactivation must not suppress that list's final animation.
        auto* r=resolve(key);if(!r||executing_.generation)return fail();
        executing_=key;Publisher publish(*this,key);
        auto status=good_.borrow();
        const bool ok=r->state.advanceImagePublished(publish,status.value());executing_={};
        return good_=good_&&ok;
    }
    bool retire(InstanceHandle key){
        auto* r=resolve(key);
        if(!r||!r->destroyRequested||same(key,executing_)||!binding_.retire(key,*r))return fail();
        *r={};--count_;return true;
    }
    bool clear(){
        if(executing_.generation)return fail();
        for(unsigned i=0;records_&&i<Capacity;++i)if(records_[i].lifetimeId){
            const InstanceHandle key{i,records_[i].lifetimeId};
            if(!binding_.retire(key,records_[i]))return fail();
            records_[i]={};--count_;
        }
        good_=true;return true;
    }
private:
    static bool same(InstanceHandle a,InstanceHandle b){return a.slot==b.slot&&a.generation==b.generation;}
    bool fail(){good_=false;return false;}
    Record* resolve(InstanceHandle h){return good_?const_cast<Record*>(find(h)):nullptr;}
    Record* records_=nullptr;Binding binding_;
    unsigned next_=1,count_=0;CallbackFailureSignal good_=true;InstanceHandle executing_{};
};
}
#endif
