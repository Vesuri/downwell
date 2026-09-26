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
    struct Slot{Handle handle=0;TimeShardAnimation effect;};
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
    void clear(){for(unsigned i=0;i<count_;++i)slots_[order_[i]].handle=0;count_=0;}
    template<class Random> Handle create(MotionBinary64 x,MotionBinary64 y,bool lowSpec,Random& random){
        retireRequested();
        if(!next_ || count_==Capacity)return 0;
        for(unsigned index=0;index<Capacity;++index)if(!slots_[index].handle){
            if(!slots_[index].effect.create(x,y,lowSpec,random))return 0;
            for(unsigned i=count_;i>0;--i)order_[i]=order_[i-1];
            order_[0]=index;++count_;return slots_[index].handle=next_++;
        }
        return 0;
    }
    // Global scheduler owns the outer alarm-index loop, including other types.
    void advanceAlarm(Phase phase,unsigned index){
        for(unsigned i=0;i<count_;++i){auto& slot=slots_[order_[i]];
            if(slot.handle<=phase.newest)slot.effect.advanceAlarm(index);
        }
        retireRequested();
    }
    bool advanceAnimation(Phase phase){
        for(unsigned i=0;i<count_;++i){auto& slot=slots_[order_[i]];
            if(slot.handle<=phase.newest && !slot.effect.advanceImage())return false;
        }
        return true;
    }
    template<class Random,class Meeting,class Outside>
    bool step(Phase phase,bool stopped,Random& random,Meeting meeting,Outside outside){
        for(unsigned i=0;i<count_;++i){auto& slot=slots_[order_[i]];
            if(slot.handle>phase.newest || slot.effect.destroyRequested)continue;
            if(!slot.effect.step(stopped,random,
                [&](auto x,auto y,bool& hit){return meeting(slot.handle,static_cast<const TimeShardAnimation&>(slot.effect),x,y,hit);},
                [&](){return int(size());},
                [&](const auto& effect,bool& retire){return outside(slot.handle,effect,retire);}))return false;
        }
        retireRequested();return true;
    }
    template<class Random> bool playerContact(Handle handle,MotionBinary64 xsp,MotionBinary64 ysp,Random& random){
        auto* effect=get(handle);
        return !effect || effect->playerContact(xsp,ysp,random);
    }
    void retireRequested(){
        unsigned kept=0;
        for(unsigned i=0;i<count_;++i){auto& slot=slots_[order_[i]];
            if(slot.effect.destroyRequested)slot.handle=0;
            else order_[kept++]=order_[i];
        }
        count_=kept;
    }
    // Unfiltered visitation supports state hashing and both drawFx modes.
    // Draw eligibility is dflash<=0 (GameMaker truth), NOT C++ !dflash.
    template<class Visit> void visitNewest(Visit visit)const{
        for(unsigned i=0;i<count_;++i){const auto& slot=slots_[order_[i]];
            if(!slot.effect.destroyRequested)visit(slot.handle,slot.effect);
        }
    }
private:
    Slot slots_[Capacity];unsigned order_[Capacity]{},count_=0;Handle next_=1;
};
}
#endif
