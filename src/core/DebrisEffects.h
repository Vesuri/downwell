#ifndef DOWNWELL_CORE_DEBRIS_EFFECTS_H
#define DOWNWELL_CORE_DEBRIS_EFFECTS_H
#include "DebrisAnimation.h"
namespace downwell {
// Fixed object-370 storage. Handles are owner-local, not Windows instance IDs.
// Capture cutoffs at global phase entry. Callbacks must not mutate this owner;
// pass adapters anew after copying a world, never retain borrowed references.
template<unsigned Capacity> class DebrisEffects {
    static_assert(Capacity>0,"Debris capacity must be nonzero");
public:
    using Handle=std::uint32_t;
    struct Phase{Handle newest;};
    struct NoLifecycle{
        bool birth(Handle,const DebrisAnimation&)const{return true;}
        bool retire(Handle,const DebrisAnimation&)const{return true;}
    };
    bool valid()const{return good_;}
    Phase beginPhase()const{return {next_?next_-1:0xffffffffu};}
    unsigned size()const{
        unsigned result=0;
        for(unsigned i=0;i<count_;++i){const auto& s=slots_[order_[i]];
            if(s.handle&&!s.effect.destroyRequested)++result;
        }
        return result;
    }
    DebrisAnimation* get(Handle h){
        if(good_&&h)for(unsigned i=0;i<count_;++i){auto& s=slots_[order_[i]];
            if(s.handle==h&&!s.effect.destroyRequested)return &s.effect;
        }
        return nullptr;
    }
    // Registered instances remain addressable after a destruction request.
    // Only the room's explicit cleanup may release/reuse their slots.
    const DebrisAnimation* retained(Handle h)const{
        if(good_&&h)for(unsigned i=0;i<count_;++i){const auto& s=slots_[order_[i]];
            if(s.handle==h)return &s.effect;
        }
        return nullptr;
    }
    // Keep caller-owned registry state: copying a lifecycle would retire only
    // its copy, leaving stale membership after the effect storage is reused.
    template<class Lifecycle=NoLifecycle> bool clear(Lifecycle&& lifecycle={}){
        for(unsigned i=0;i<count_;++i){auto& s=slots_[order_[i]];
            if(s.handle){if(!lifecycle.retire(s.handle,s.effect)){good_=false;return false;}s.handle=0;}
        }
        count_=0;good_=true;return true; // Never rewind lifetime keys.
    }
    template<class Random,class Lifecycle=NoLifecycle>
    Handle create(MotionBinary64 x,MotionBinary64 y,int area,Random& random,Lifecycle&& lifecycle={}){
        if(!good_||!retireRequested(lifecycle))return 0;
        return createRegistered(x,y,area,random,lifecycle);
    }
    template<class Random,class Lifecycle>
    Handle createRegistered(MotionBinary64 x,MotionBinary64 y,int area,Random& random,Lifecycle& lifecycle){
        if(!good_||!next_||count_==Capacity)return 0;
        for(unsigned index=0;index<Capacity;++index)if(!slots_[index].handle){
            auto& s=slots_[index];s.markPublished=false;
            if(!s.effect.create(x,y,area,random)){good_=false;return 0;}
            for(unsigned i=count_;i>0;--i)order_[i]=order_[i-1];
            order_[0]=index;++count_;s.handle=next_++;
            if(!lifecycle.birth(s.handle,s.effect)){good_=false;return 0;}
            return s.handle;
        }
        good_=false;return 0;
    }
    template<class Lifecycle>
    bool advanceRegisteredAnimationOne(Handle h,Phase phase,Lifecycle& lifecycle){
        auto* slot=find(h);if(!slot)return good_=false;
        if(h>phase.newest||slot->effect.destroyRequested)return true;
        if(!slot->effect.advanceImage()||!lifecycle.image(h,slot->effect))return good_=false;
        return true;
    }
    template<class Lifecycle>
    bool advanceRegisteredAlarmOne(Handle h,Phase phase,unsigned index,Lifecycle& lifecycle){
        auto* slot=find(h);if(!slot||index>2)return good_=false;
        if(h>phase.newest||slot->effect.destroyRequested)return true;
        slot->effect.advanceAlarm(index);
        return publishMark(*slot,lifecycle);
    }
    template<class Random,class Stopped,class Meeting,class Publish,class Lifecycle>
    bool stepRegisteredOne(Handle h,Phase phase,Random& random,Stopped stopped,
                           Meeting meeting,Publish publish,Lifecycle& lifecycle){
        auto* slot=find(h);if(!slot)return good_=false;
        auto& effect=slot->effect;
        if(h>phase.newest||effect.destroyRequested)return true;
        bool frozen=false;
        if(!stopped(h,static_cast<const DebrisAnimation&>(effect),frozen)||
           !effect.step(frozen,random,[&](auto x,auto y,bool& hit){
               return meeting(h,static_cast<const DebrisAnimation&>(effect),x,y,hit);
           },[&](auto x,auto y){
               return publish(h,static_cast<const DebrisAnimation&>(effect),x,y);
           }))return good_=false;
        return publishMark(*slot,lifecycle);
    }
    template<class Lifecycle>
    bool dispatchRegisteredAlarmEventOne(Handle h,const AlarmBank& bank,unsigned index,Lifecycle& lifecycle){
        auto* slot=find(h);
        if(!slot||index>2||bank.get(index)!=0)return good_=false;
        if(slot->effect.destroyRequested)return true;
        slot->effect.alarms=bank;
        slot->effect.alarmEvent(index);
        return publishMark(*slot,lifecycle);
    }
    template<class Lifecycle> bool retireMarked(Handle h,Lifecycle& lifecycle){
        auto* slot=find(h);
        if(!slot||!slot->effect.destroyRequested||!slot->markPublished||
           !lifecycle.retire(h,slot->effect))return good_=false;
        slot->handle=0;
        unsigned kept=0;
        for(unsigned i=0;i<count_;++i)if(slots_[order_[i]].handle)order_[kept++]=order_[i];
        count_=kept;return true;
    }
    bool advanceAnimation(Phase phase){
        if(!good_)return false;
        for(unsigned i=0;i<count_;++i){auto& s=slots_[order_[i]];
            if(s.handle<=phase.newest&&!s.effect.advanceImage()){good_=false;return false;}
        }
        return true;
    }
    template<class Lifecycle=NoLifecycle> bool advanceAlarm(Phase phase,unsigned index,Lifecycle&& lifecycle={}){
        if(!good_)return false;
        for(unsigned i=0;i<count_;++i){auto& s=slots_[order_[i]];
            if(s.handle<=phase.newest)s.effect.advanceAlarm(index);
        }
        return retireRequested(lifecycle);
    }
    // TimeStopBound is evaluated per effect, not once for the entire pool.
    template<class Random,class Stopped,class Meeting,class Publish,class Lifecycle=NoLifecycle>
    bool stepPublished(Phase phase,Random& random,Stopped stopped,Meeting meeting,
                       Publish publish,Lifecycle&& lifecycle={}){
        if(!good_)return false;
        for(unsigned i=0;i<count_;++i){auto& s=slots_[order_[i]];
            if(s.handle>phase.newest||s.effect.destroyRequested)continue;
            bool frozen=false;
            if(!stopped(s.handle,static_cast<const DebrisAnimation&>(s.effect),frozen)||
               !s.effect.step(frozen,random,[&](auto x,auto y,bool& hit){
                   return meeting(s.handle,static_cast<const DebrisAnimation&>(s.effect),x,y,hit);
               },[&](auto x,auto y){
                   return publish(s.handle,static_cast<const DebrisAnimation&>(s.effect),x,y);
               })){good_=false;return false;}
        }
        return retireRequested(lifecycle);
    }
    template<class Random,class Stopped,class Meeting,class Lifecycle=NoLifecycle>
    bool step(Phase phase,Random& random,Stopped stopped,Meeting meeting,Lifecycle&& lifecycle={}){
        return stepPublished(phase,random,stopped,meeting,
            [](Handle,const DebrisAnimation&,RunnerImageValue,RunnerImageValue){return true;},lifecycle);
    }
    template<class Lifecycle=NoLifecycle> bool retireRequested(Lifecycle&& lifecycle={}){
        if(!good_)return false;
        for(unsigned i=0;i<count_;++i){auto& s=slots_[order_[i]];
            if(s.effect.destroyRequested){
                if(!lifecycle.retire(s.handle,s.effect)){good_=false;return false;}
                s.handle=0;
            }
        }
        unsigned kept=0;
        for(unsigned i=0;i<count_;++i)if(slots_[order_[i]].handle)order_[kept++]=order_[i];
        count_=kept;return true;
    }
    // Unfiltered values for hashing and both drawFx routes; flash<=0 is the
    // GameMaker draw condition. Player contact is an empty source event.
    template<class Visit> bool visitNewest(Visit visit)const{
        if(!good_)return false;
        for(unsigned i=0;i<count_;++i){const auto& s=slots_[order_[i]];
            if(!s.effect.destroyRequested)visit(s.handle,s.effect);
        }
        return true;
    }
private:
    struct Slot{Handle handle=0;DebrisAnimation effect;bool markPublished=false;};
    Slot* find(Handle h){
        if(good_&&h)for(unsigned i=0;i<count_;++i)if(slots_[order_[i]].handle==h)return &slots_[order_[i]];
        return nullptr;
    }
    template<class Lifecycle> bool publishMark(Slot& slot,Lifecycle& lifecycle){
        if(slot.effect.destroyRequested&&!slot.markPublished){
            if(!lifecycle.destroyed(slot.handle,slot.effect))return good_=false;
            slot.markPublished=true;
        }
        return true;
    }
    Slot slots_[Capacity];unsigned order_[Capacity]{},count_=0;
    Handle next_=1;bool good_=true;
};
}
#endif
