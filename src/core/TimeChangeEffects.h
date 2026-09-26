#ifndef DOWNWELL_CORE_TIME_CHANGE_EFFECTS_H
#define DOWNWELL_CORE_TIME_CHANGE_EFFECTS_H
#include "TimeChangeEffect.h"
namespace downwell {
// No Step or visible sprite. The world supplies one shared room-speed value.
template<unsigned Capacity> class TimeChangeEffects {
    static_assert(Capacity>0,"Time-change storage must be nonempty");
public:
    using Handle=std::uint32_t;
    struct Phase{Handle newest;};
    struct Slot{Handle handle=0;TimeChangeEffect effect;};
    unsigned size()const{return count_;}
    Phase beginPhase()const{return {next_?next_-1:0xffffffffu};}
    void clear(){count_=0;} // Do not reuse identities across room reset.
    template<class Pause> bool create(int& speed,Pause pause){
        if(count_==Capacity || !next_)return false;
        // Insert before synchronous Create. A failing pause leaves the source
        // assignments and instance present; caller must latch the failure.
        for(unsigned i=count_;i>0;--i)slots_[i]=slots_[i-1];
        ++count_;slots_[0].handle=next_++;
        return slots_[0].effect.create(speed,pause);
    }
    void advanceAlarm(Phase phase,unsigned index,int& speed){
        unsigned kept=0;
        for(unsigned i=0;i<count_;++i){
            auto& slot=slots_[i];
            if(slot.handle<=phase.newest)slot.effect.advanceAlarm(index,speed);
            if(!slot.effect.destroyRequested)slots_[kept++]=slot;
        }
        count_=kept;
    }
private:
    Slot slots_[Capacity];unsigned count_=0;Handle next_=1;
};
}
#endif
