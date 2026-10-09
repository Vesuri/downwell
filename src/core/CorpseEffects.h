#ifndef DOWNWELL_CORE_CORPSE_EFFECTS_H
#define DOWNWELL_CORE_CORPSE_EFFECTS_H
#include "CorpseAnimation.h"
#include "DeadBodyCreation.h"
namespace downwell {
// Owner-local monotonic handles, not Windows instance IDs. Global schedulers
// merge event order across families; a phase cutoff excludes subsequent births.
// Callbacks may create other families, but must not mutate this store. Pass
// fresh adapters after world copies; no borrowed callback is retained here.
template<unsigned Capacity> class CorpseEffects {
    static_assert(Capacity>0,"nonempty corpse capacity required");
public:
    using Handle=std::uint32_t;
    struct Phase{Handle newest;};
    bool valid()const{return good_;}
    Phase beginPhase()const{return {next_?next_-1:0xffffffffu};}
    // Lifetime lookup is separate from event eligibility: marked corpses and
    // partial failures remain owned until explicit room cleanup succeeds.
    const CorpseAnimation* find(Handle handle)const{
        if(handle)for(unsigned i=0;i<count_;++i){const auto& slot=slots_[order_[i]];
            if(slot.handle==handle)return &slot.effect;
        }
        return nullptr;
    }
    CorpseAnimation* get(Handle handle){
        if(good_&&handle)for(unsigned i=0;i<count_;++i){auto& slot=slots_[order_[i]];
            if(slot.handle==handle&&slot.effect.valid&&!slot.effect.destroyRequested)return &slot.effect;
        }
        return nullptr;
    }
    template<class Random,class Hooks>
    Handle create(MotionBinary64 x,MotionBinary64 y,bool rip,Random& random,Hooks& hooks){
        if(!good_||!next_||count_==Capacity)return 0;
        unsigned index=0;while(index<Capacity&&slots_[index].handle)++index;
        if(index==Capacity){good_=false;return 0;}
        auto& slot=slots_[index];slot.effect={};
        if(!slot.effect.x.assign(x)||!slot.effect.y.assign(y)){good_=false;return 0;}
        slot.handle=next_++;
        for(unsigned i=count_;i>0;--i)order_[i]=order_[i-1];
        order_[0]=index;++count_;
        // Keep the slot even on partial publication failure so clear can
        // release any external membership; never recycle it silently.
        if(!hooks.birth(slot.handle,static_cast<const CorpseAnimation&>(slot.effect))){good_=false;return 0;}
        auto&& queries=hooks.queries(slot.handle,slot.effect);
        if(!slot.effect.create(x,y,rip,random,queries)){good_=false;return 0;}
        return slot.handle;
    }
    // Shared scrDeadBody entry point. Custom fields stay in the owned corpse;
    // sprite/facing writes publish through the external collision adapter.
    // Caller mutations in source must be applied to the dying enemy by its
    // scheduler. Failure is terminal for this owner, including full capacity.
    template<class Random,class Hooks>
    bool createFromDeath(DeadBodySource& source,int sprite,MotionBinary64 impact,
                         int explosion,bool rip,Random& random,Hooks& hooks){
        if(!good_)return false;
        struct Events {
            CorpseEffects& owner;Random& random;Hooks& hooks;bool rip;
            bool create(int object,MotionBinary64 x,MotionBinary64 y,std::uint32_t& child){
                if(object!=263)return false;
                child=owner.create(x,y,rip,random,hooks);return child!=0;
            }
            bool sprite(Handle h,int next){
                auto* corpse=owner.get(h);
                return corpse&&corpse->setSprite(next,[&](const auto& state){return hooks.sprite(h,state);});
            }
            bool verticalSpeed(Handle h,MotionBinary64 value){
                auto* corpse=owner.get(h);if(!corpse)return false;corpse->ysp=value;return true;
            }
            bool facing(Handle h,MotionBinary64 value){
                auto* corpse=owner.get(h);RunnerImageValue next;
                if(!corpse||!next.assign(value))return false;
                const auto old=corpse->imageXscale.bits;
                if(next.bits==old||!((next.bits|old)&0x7fffffffu))return true;
                corpse->imageXscale=next;return hooks.facing(h,*corpse);
            }
            bool explosion(Handle h,int id){
                auto* corpse=owner.get(h);if(!corpse)return false;corpse->explosionFx=id;return true;
            }
            bool horizontalSpeed(Handle h,MotionBinary64 value){
                auto* corpse=owner.get(h);if(!corpse)return false;corpse->xsp=value;return true;
            }
        } events{*this,random,hooks,rip};
        if(!createDeadBody(source,sprite,impact,explosion,events)){good_=false;return false;}
        return true;
    }
    // One global phase at a time: animation, ONE alarm index, Step, contact or
    // End Step. The caller supplies the corresponding checked event adapter.
    template<class Run,class Hooks> bool dispatch(Phase phase,Run run,Hooks&){
        if(!good_)return false;
        for(unsigned i=0;i<count_;++i){auto& slot=slots_[order_[i]];
            if(slot.handle>phase.newest||slot.effect.destroyRequested)continue;
            if(!slot.effect.valid||!run(slot.handle,slot.effect)||!slot.effect.valid){good_=false;return false;}
        }
        return true;
    }
    // Only the enclosing world's cleanup boundary may retire marked records.
    // Birth and event dispatch must not release their slots or memberships.
    template<class Hooks> bool retireRequested(Hooks& hooks){
        if(!good_)return false;
        for(unsigned i=0;i<count_;++i){auto& slot=slots_[order_[i]];
            if(slot.effect.destroyRequested){
                if(!hooks.retire(slot.handle,static_cast<const CorpseAnimation&>(slot.effect))){good_=false;return false;}
                slot.handle=0;
            }
        }
        unsigned kept=0;
        for(unsigned i=0;i<count_;++i)if(slots_[order_[i]].handle)order_[kept++]=order_[i];
        count_=kept;return true;
    }
    template<class Hooks> bool clear(Hooks& hooks){
        for(unsigned i=0;i<count_;++i){auto& slot=slots_[order_[i]];
            if(slot.handle){
                if(!hooks.retire(slot.handle,static_cast<const CorpseAnimation&>(slot.effect))){good_=false;return false;}
                slot.handle=0;
            }
        }
        count_=0;good_=true;return true; // Do not rewind lifetime keys.
    }
    template<class Visit> bool visitNewest(Visit visit)const{
        if(!good_)return false;
        for(unsigned i=0;i<count_;++i){const auto& slot=slots_[order_[i]];
            if(!slot.effect.valid)return false;
            if(!slot.effect.destroyRequested)visit(slot.handle,slot.effect);
        }
        return true;
    }
private:
    struct Slot{Handle handle=0;CorpseAnimation effect;};
    Slot slots_[Capacity];unsigned order_[Capacity]{},count_=0;
    Handle next_=1;bool good_=true;
};
}
#endif
