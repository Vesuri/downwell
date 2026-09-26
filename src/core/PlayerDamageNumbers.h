#ifndef DOWNWELL_CORE_PLAYER_DAMAGE_NUMBERS_H
#define DOWNWELL_CORE_PLAYER_DAMAGE_NUMBERS_H
#include "PlayerDamageNumber.h"
namespace downwell {
// Room-owned object-221 instances. Capture phase cutoffs at GLOBAL phase
// entry; callbacks cannot mutate the owner or retain effect references.
template<unsigned Capacity> class PlayerDamageNumbers {
    static_assert(Capacity>0,"nonempty damage-number capacity required");
public:
    using Handle=std::uint32_t;
    struct BirthSequence{Handle next=1;};
    BirthSequence birthSequence()const{return {next_};}
    bool restoreEmptyBirthSequence(BirthSequence sequence){
        if(count_||!good_||(!next_&&sequence.next)||(sequence.next&&sequence.next<next_))return false;
        next_=sequence.next;return true;
    }
    struct Phase{Handle newest;};
    bool valid()const{return good_;}
    unsigned size()const{return count_;}
    const PlayerDamageNumber* atNewest(unsigned index)const{
        if(!good_||index>=count_)return nullptr;
        const auto& effect=slots_[order_[index]].effect;
        return effect.valid&&!effect.destroyRequested?&effect:nullptr;
    }
    Phase beginPhase()const{return {next_?next_-1:0xffffffffu};}
    PlayerDamageNumber* get(Handle h){
        if(good_&&h)for(unsigned i=0;i<count_;++i){auto& s=slots_[order_[i]];
            if(s.handle==h&&s.effect.valid&&!s.effect.destroyRequested)return &s.effect;
        }
        return nullptr;
    }
    template<class Hooks> Handle create(MotionBinary64 x,MotionBinary64 y,Hooks& hooks){
        if(!good_)return 0;
        if(!next_||count_==Capacity){good_=false;return 0;}
        RunnerImageValue px,py;
        if(!px.assign(x)||!py.assign(y)){good_=false;return 0;}
        unsigned index=0;while(index<Capacity&&slots_[index].handle)++index;
        if(index==Capacity){good_=false;return 0;}
        auto& slot=slots_[index];slot.effect={};slot.effect.x=px;slot.effect.y=py;slot.handle=next_++;
        for(unsigned i=count_;i>0;--i)order_[i]=order_[i-1];
        order_[0]=index;++count_;
        // Birth hook receives only constructor coordinates, not Create's user
        // variables. No parent PreCreate action exists for this object.
        if(!hooks.birth(slot.handle,px,py)||!slot.effect.create(x,y)){good_=false;return 0;}
        return slot.handle;
    }
    template<class Hooks> Handle createFromPlayer(MotionBinary64 x,MotionBinary64 y,
        MotionBinary64 amount,MotionBinary64 direction,Hooks& hooks){
        if(!good_)return 0;
        if(!y.add(MotionBinary64::fromInteger(-4))){good_=false;return 0;}
        const auto h=create(x,y,hooks);
        auto* effect=get(h);
        if(!effect||!effect->assignDamage(amount,direction)){good_=false;return 0;}
        return h;
    }
    template<class Hooks> bool advanceAlarm(Phase phase,unsigned index,Hooks& hooks){
        if(!good_)return false;
        for(unsigned i=0;i<count_;++i){auto& slot=slots_[order_[i]];
            if(slot.handle>phase.newest)continue;
            slot.effect.advanceAlarm(index);
            if(slot.effect.destroyRequested){
                if(!hooks.retire(slot.handle,static_cast<const PlayerDamageNumber&>(slot.effect))){good_=false;return false;}
                slot.handle=0;
            }
        }
        compact();return true;
    }
    template<class LengthdirX,class Hooks> bool step(Phase phase,LengthdirX lengthdirX,Hooks& hooks){
        if(!good_)return false;
        for(unsigned i=0;i<count_;++i){auto& slot=slots_[order_[i]];
            if(slot.handle>phase.newest)continue;
            if(!slot.effect.step(lengthdirX,[&](bool x,const auto& effect){return hooks.position(slot.handle,x,effect);})){good_=false;return false;}
        }
        return true;
    }
    template<class Hooks> bool clear(Hooks& hooks){
        // Cleanup remains available after failure and resumes after completed
        // retirements. It never reuses lifetime keys or retries simulation.
        for(unsigned i=0;i<count_;++i){auto& slot=slots_[order_[i]];
            if(slot.handle){
                if(!hooks.retire(slot.handle,static_cast<const PlayerDamageNumber&>(slot.effect))){good_=false;return false;}
                slot.handle=0;
            }
        }
        count_=0;good_=true;return true;
    }
    template<class Visit> bool visitNewest(Visit visit)const{
        if(!good_)return false;
        for(unsigned i=0;i<count_;++i){const auto& slot=slots_[order_[i]];
            if(!slot.effect.valid)return false;
            if(!slot.effect.destroyRequested&&!visit(slot.handle,slot.effect))return false;
        }
        return true;
    }
private:
    void compact(){
        unsigned kept=0;
        for(unsigned i=0;i<count_;++i)if(slots_[order_[i]].handle)order_[kept++]=order_[i];
        count_=kept;
    }
    struct Slot{Handle handle=0;PlayerDamageNumber effect;};
    Slot slots_[Capacity];unsigned order_[Capacity]{},count_=0;
    Handle next_=1;bool good_=true;
};
}
#endif
