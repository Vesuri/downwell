#ifndef DOWNWELL_CORE_CORPSE_EXPIRY_INSTANCES_H
#define DOWNWELL_CORE_CORPSE_EXPIRY_INSTANCES_H
#include "CorpseExpiryEffect.h"
#include "InstanceLifecycle.h"
#include "FailureSignal.h"
namespace downwell {
// Native typed lifetimes for object 242. Source IDs come from the enclosing
// room creation stream; generation handles identify storage only.
template<unsigned Capacity> class CorpseExpiryInstances {
    static_assert(Capacity>0,"nonempty expiry instance capacity required");
public:
    struct Record {
        std::uint32_t lifetimeId=0,sourceId=0;
        RunnerImageValue x{},y{};
        CorpseExpiryEffect state;
        bool active=true,destroyRequested=false;
        static constexpr int objectId=242;
    };
    using Binding=RecordLifecycleBinding<Record>;
    explicit CorpseExpiryInstances(Binding binding={}):binding_(binding){}
    CorpseExpiryInstances(const CorpseExpiryInstances&)=delete;
    CorpseExpiryInstances& operator=(const CorpseExpiryInstances&)=delete;
    bool valid()const{return good_;}
    void observeFailure(bool& owner){good_.observe(owner);}
    unsigned size()const{return count_;}
    const Record* find(InstanceHandle key)const{
        return key.generation&&key.slot<Capacity&&records_[key.slot].lifetimeId==key.generation?
            &records_[key.slot]:nullptr;
    }
    InstanceHandle sourceHandle(std::uint32_t id)const{
        for(unsigned i=0;id&&i<Capacity;++i)if(records_[i].lifetimeId&&records_[i].sourceId==id)
            return {i,records_[i].lifetimeId};
        return {};
    }
    bool create(std::uint32_t id,MotionBinary64 x,MotionBinary64 y,InstanceHandle& output){
        output={};if(!good_||!next_||!id||id>0x7fffffffu||sourceHandle(id).generation)return fail();
        unsigned slot=0;while(slot<Capacity&&records_[slot].lifetimeId)++slot;
        RunnerImageValue px,py;
        if(slot==Capacity||!px.assign(x)||!py.assign(y))return fail();
        auto& r=records_[slot];r={};r.lifetimeId=next_++;r.sourceId=id;r.x=px;r.y=py;
        r.state.x=px;r.state.y=py;r.state.imageSpeed={0x3f800000u};++count_;
        const InstanceHandle key{slot,r.lifetimeId};
        if(!binding_.birthAt(key,r,px,py)||!r.state.create(x,y))return fail();
        output=key;return true;
    }
    bool sprite(InstanceHandle key,int sprite,unsigned frames){
        auto* r=resolve(key);if(!r||r->destroyRequested)return fail();
        return r->state.setSprite(sprite,frames,[&](const auto& state){
            return binding_.sprite(key,*r,sprite)&&binding_.imageIndex(key,*r,state.imageIndex);
        })||fail();
    }
    bool advanceAnimation(InstanceHandle key){
        auto* r=resolve(key);if(!r)return fail();if(!r->active||r->destroyRequested)return true;
        if(!r->state.advanceImage([&](const auto& state){return binding_.imageIndex(key,*r,state.imageIndex);}))return fail();
        if(r->state.destroyRequested){
            if(!binding_.markDestroyed(key,*r))return fail();r->destroyRequested=true;
        }
        return true;
    }
    bool activate(InstanceHandle key,bool active){
        auto* r=resolve(key);if(!r||r->destroyRequested||!binding_.activation(key,*r,active))return fail();
        r->active=active;return true;
    }
    bool retire(InstanceHandle key){
        auto* r=resolve(key);
        if(!r||!r->destroyRequested||!r->state.destroyRequested||!binding_.retire(key,*r))return fail();
        *r={};--count_;return true;
    }
    bool clear(){
        for(unsigned i=0;i<Capacity;++i)if(records_[i].lifetimeId){
            if(!binding_.retire({i,records_[i].lifetimeId},records_[i]))return fail();
            records_[i]={};--count_;
        }
        return true;
    }
private:
    Record* resolve(InstanceHandle key){return good_?const_cast<Record*>(find(key)):nullptr;}
    bool fail(){good_=false;return false;}
    Binding binding_;Record records_[Capacity]{};std::uint32_t next_=1;
    unsigned count_=0;FailureSignal<> good_=true;
};
}
#endif
