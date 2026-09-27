#ifndef DOWNWELL_CORE_TIME_SHARD_EFFECTS_H
#define DOWNWELL_CORE_TIME_SHARD_EFFECTS_H
#include "TimeShardAnimation.h"
namespace downwell {
// Object-238 storage. Capture phase cutoffs at GLOBAL phase entry. World
// callbacks must not mutate this store or retain references beyond a call.
template<unsigned Capacity> class TimeShardEffects {
    static_assert(Capacity>0,"Shard storage must be nonempty");
public:
    using Handle=std::uint32_t;
    struct Phase{Handle newest;};
    struct Slot{Handle handle=0;std::uint32_t sourceId=0;TimeShardAnimation effect;};
    // Borrowed world registration. The world owns the numeric instance ID;
    // our monotonic handle only identifies an effect lifetime/phase cutoff.
    // Registration precedes synchronous Create and therefore its six RNG words.
    // Callbacks must not mutate this store. Failed callbacks are terminal: do
    // not replay a partial Create or recycle a still-registered lifetime.
    template<class World> bool bind(World& world){
        if(count_||context_||!succeeded_)return false;
        context_=&world;
        birth_=[](void* p,Handle h,const TimeShardAnimation& effect,std::uint32_t& id){
            return static_cast<World*>(p)->birth(h,effect.x,effect.y,id);
        };
        created_=[](void* p,Handle h,std::uint32_t id,const TimeShardAnimation& effect){
            return static_cast<World*>(p)->created(h,id,effect);
        };
        retire_=[](void* p,Handle h,std::uint32_t id,const TimeShardAnimation& effect){
            return static_cast<World*>(p)->retire(h,id,effect);
        };
        return true;
    }
    bool succeeded()const{return succeeded_;}
    template<class World> bool bindPhases(World& world){
        if(!bind(world))return false;
        deferCleanup_=true;
        population_=[](void* p,unsigned& count){return static_cast<World*>(p)->population(count);};
        eligible_=[](void* p,Handle h,std::uint32_t id,bool& eligible){
            return static_cast<World*>(p)->eventEligible(h,id,eligible);
        };
        animationEligible_=[](void* p,Handle h,std::uint32_t id,bool& eligible){
            return static_cast<World*>(p)->animationEligible(h,id,eligible);
        };
        animation_=[](void* p,Handle h,std::uint32_t id,const TimeShardAnimation& effect){
            return static_cast<World*>(p)->publishAnimation(h,id,effect);
        };
        step_=[](void* p,Handle h,std::uint32_t id,const TimeShardAnimation& effect){
            return static_cast<World*>(p)->publishStep(h,id,effect);
        };
        alarm_=[](void* p,Handle h,std::uint32_t id,const TimeShardAnimation& effect){
            return static_cast<World*>(p)->publishAlarm(h,id,effect);
        };
        destroyed_=[](void* p,Handle h,std::uint32_t id,const TimeShardAnimation& effect){
            return static_cast<World*>(p)->destroyed(h,id,effect);
        };
        return true;
    }
    template<class World> bool phasesBoundTo(const World& world)const{return context_==&world&&animation_&&step_;}
    std::uint32_t sourceId(Handle handle)const{
        for(unsigned i=0;i<count_;++i){const auto& slot=slots_[order_[i]];
            if(slot.handle==handle)return slot.sourceId;
        }
        return 0;
    }
    Phase beginPhase()const{return {next_?next_-1:0xffffffffu};}
    unsigned size()const{
        unsigned alive=0;
        for(unsigned i=0;i<count_;++i)if(!slots_[order_[i]].effect.destroyRequested)++alive;
        return alive;
    }
    TimeShardAnimation* get(Handle handle){
        if(handle)for(unsigned i=0;i<count_;++i){auto& slot=slots_[order_[i]];
            if(slot.handle==handle && !slot.effect.destroyRequested)return &slot.effect;
        }
        return nullptr;
    }
    bool clear(){
        if(!succeeded_)return false;
        for(unsigned i=0;i<count_;++i)slots_[order_[i]].effect.destroyRequested=true;
        return retireRequested();
    }
    template<class Random> Handle create(MotionBinary64 x,MotionBinary64 y,bool lowSpec,Random& random){
        if(!succeeded_||(!deferCleanup_&&!retireRequested()))return 0;
        if(!next_ || count_==Capacity)return 0;
        for(unsigned index=0;index<Capacity;++index)if(!slots_[index].handle){
            TimeShardAnimation initial;
            if(!initial.x.assign(x)||!initial.y.assign(y))return 0;
            auto& slot=slots_[index];slot.effect=initial;
            slot.handle=next_++;slot.sourceId=slot.handle;
            for(unsigned i=count_;i>0;--i)order_[i]=order_[i-1];
            order_[0]=index;++count_;
            if(birth_&&!birth_(context_,slot.handle,slot.effect,slot.sourceId)){
                succeeded_=false;return 0;
            }
            if(!slot.sourceId||slot.sourceId>0x7fffffffu){succeeded_=false;return 0;}
            for(unsigned i=1;i<count_;++i)if(slots_[order_[i]].sourceId==slot.sourceId){
                succeeded_=false;return 0;
            }
            if(!slot.effect.create(x,y,lowSpec,random)||
               (created_&&!created_(context_,slot.handle,slot.sourceId,slot.effect))){
                succeeded_=false;return 0;
            }
            return slot.handle;
        }
        return 0;
    }
    // Global scheduler owns the outer alarm-index loop, including other types.
    void advanceAlarm(Phase phase,unsigned index){
        if(!succeeded_)return;
        for(unsigned i=0;i<count_;++i){auto& slot=slots_[order_[i]];
            if(slot.handle<=phase.newest&&!slot.effect.destroyRequested){
                bool eligible=true;
                if(!eventEligible(slot,eligible))return;
                if(!eligible)continue;
                slot.effect.advanceAlarm(index);
                if(slot.effect.destroyRequested&&destroyed_&&!destroyed_(context_,slot.handle,slot.sourceId,slot.effect)){
                    succeeded_=false;return;
                }
                if(!slot.effect.destroyRequested&&alarm_&&!alarm_(context_,slot.handle,slot.sourceId,slot.effect)){
                    succeeded_=false;return;
                }
            }
        }
        if(!deferCleanup_)retireRequested();
    }
    bool advanceAnimation(Phase phase){
        if(!succeeded_)return false;
        for(unsigned i=0;i<count_;++i){auto& slot=slots_[order_[i]];
            if(slot.handle<=phase.newest&&!slot.effect.destroyRequested){
                bool eligible=true;
                if(animationEligible_&&!animationEligible_(context_,slot.handle,slot.sourceId,eligible)){
                    succeeded_=false;return false;
                }
                if(!eligible)continue;
                if(!slot.effect.advanceImage()||
                   (animation_&&!animation_(context_,slot.handle,slot.sourceId,slot.effect))){
                    succeeded_=false;return false;
                }
            }
        }
        return true;
    }
    template<class Random,class Meeting,class Outside>
    bool step(Phase phase,bool stopped,Random& random,Meeting meeting,Outside outside){
        if(!succeeded_)return false;
        for(unsigned i=0;i<count_;++i){auto& slot=slots_[order_[i]];
            if(slot.handle>phase.newest || slot.effect.destroyRequested)continue;
            bool eligible=true;
            if(!eventEligible(slot,eligible))return false;
            if(!eligible)continue;
            if(!slot.effect.step(stopped,random,
                [&](auto x,auto y,bool& hit){return meeting(slot.handle,static_cast<const TimeShardAnimation&>(slot.effect),x,y,hit);},
                [&](){
                    unsigned count=size();
                    if(population_&&(!population_(context_,count)||count>0x7fffffffu))return -1;
                    return static_cast<int>(count);
                },
                [&](const auto& effect,bool& retire){return outside(slot.handle,effect,retire);},
                [&](){return !destroyed_||destroyed_(context_,slot.handle,slot.sourceId,slot.effect);})||
               (step_&&!step_(context_,slot.handle,slot.sourceId,slot.effect))){
                succeeded_=false;return false;
            }
        }
        return deferCleanup_||retireRequested();
    }
    template<class Random> bool playerContact(Handle handle,MotionBinary64 xsp,MotionBinary64 ysp,Random& random){
        if(!succeeded_)return false;
        auto* effect=get(handle);
        return !effect || effect->playerContact(xsp,ysp,random);
    }
    // A global room-list sweep selects one lifetime, not an entire family.
    // Both keys must match: numeric IDs are external and slots may be reused.
    // Reject live/stale selections before publishing any retirement.
    bool retireMarked(Handle handle,std::uint32_t id){
        if(!succeeded_)return false;
        for(unsigned i=0;i<count_;++i){auto& slot=slots_[order_[i]];
            if(slot.handle!=handle)continue;
            if(slot.sourceId!=id||!slot.effect.destroyRequested){succeeded_=false;return false;}
            if(retire_&&!retire_(context_,handle,id,slot.effect)){succeeded_=false;return false;}
            slot.handle=slot.sourceId=0;
            for(unsigned j=i+1;j<count_;++j)order_[j-1]=order_[j];
            --count_;return true;
        }
        succeeded_=false;return false;
    }
    bool retireRequested(){
        if(!succeeded_)return false;
        unsigned kept=0;
        for(unsigned i=0;i<count_;++i){auto& slot=slots_[order_[i]];
            if(slot.effect.destroyRequested){
                if(retire_&&!retire_(context_,slot.handle,slot.sourceId,slot.effect)){
                    // Keep the failed and unvisited registrations intact;
                    // successful earlier retirements must not be retried.
                    for(unsigned j=i;j<count_;++j)order_[kept++]=order_[j];
                    count_=kept;succeeded_=false;return false;
                }
                slot.handle=slot.sourceId=0;
            }
            else order_[kept++]=order_[i];
        }
        count_=kept;return true;
    }
    // Unfiltered visitation supports state hashing and both drawFx modes.
    // Draw eligibility is dflash<=0 (GameMaker truth), NOT C++ !dflash.
    template<class Visit> void visitNewest(Visit visit)const{
        for(unsigned i=0;i<count_;++i){const auto& slot=slots_[order_[i]];
            if(!slot.effect.destroyRequested)visit(slot.handle,slot.effect);
        }
    }
private:
    bool eventEligible(const Slot& slot,bool& eligible){
        if(eligible_&&!eligible_(context_,slot.handle,slot.sourceId,eligible)){
            succeeded_=false;return false;
        }
        return true;
    }
    Slot slots_[Capacity];unsigned order_[Capacity]{},count_=0;Handle next_=1;
    bool succeeded_=true;
    // Source frame cleanup is explicit (normally Draw entry), not each phase
    // or the next Create. Unbound component fixtures retain eager cleanup.
    bool deferCleanup_=false;
    void* context_=nullptr;
    bool (*population_)(void*,unsigned&)=nullptr;
    bool (*eligible_)(void*,Handle,std::uint32_t,bool&)=nullptr;
    bool (*animationEligible_)(void*,Handle,std::uint32_t,bool&)=nullptr;
    bool (*birth_)(void*,Handle,const TimeShardAnimation&,std::uint32_t&)=nullptr;
    bool (*created_)(void*,Handle,std::uint32_t,const TimeShardAnimation&)=nullptr;
    bool (*retire_)(void*,Handle,std::uint32_t,const TimeShardAnimation&)=nullptr;
    bool (*animation_)(void*,Handle,std::uint32_t,const TimeShardAnimation&)=nullptr;
    bool (*step_)(void*,Handle,std::uint32_t,const TimeShardAnimation&)=nullptr;
    bool (*alarm_)(void*,Handle,std::uint32_t,const TimeShardAnimation&)=nullptr;
    bool (*destroyed_)(void*,Handle,std::uint32_t,const TimeShardAnimation&)=nullptr;
};
}
#endif
