#ifndef DOWNWELL_CORE_TIME_CHANGE_EFFECTS_H
#define DOWNWELL_CORE_TIME_CHANGE_EFFECTS_H
#include "TimeChangeEffect.h"
#include "RunnerImageValue.h"
namespace downwell {
// No Step or visible sprite. The world supplies one shared room-speed value.
template<unsigned Capacity> class TimeChangeEffects {
    static_assert(Capacity>0,"Time-change storage must be nonempty");
public:
    using Handle=std::uint32_t;
    struct Phase{Handle newest;};
    struct Slot{Handle handle=0;std::uint32_t sourceId=0;TimeChangeEffect effect;};
    // Borrowed fixed-address registration. Do not copy a bound owner; callbacks
    // must not mutate this store. Retire all registrations before world teardown.
    template<class World> bool bind(World& world){
        if(count_||context_||!succeeded_)return false;
        context_=&world;
        birth_=[](void* p,Handle h,RunnerImageValue x,RunnerImageValue y,std::uint32_t& id){
            return static_cast<World*>(p)->birth(h,x,y,id);
        };
        publish_=[](void* p,Handle h,std::uint32_t id,const TimeChangeEffect& e){return static_cast<World*>(p)->publish(h,id,e);};
        eligible_=[](void* p,Handle h,std::uint32_t id,bool& eligible){return static_cast<World*>(p)->eventEligible(h,id,eligible);};
        destroyed_=[](void* p,Handle h,std::uint32_t id,const TimeChangeEffect& e){return static_cast<World*>(p)->destroyed(h,id,e);};
        retire_=[](void* p,Handle h,std::uint32_t id,const TimeChangeEffect& e){return static_cast<World*>(p)->retire(h,id,e);};
        return true;
    }
    bool succeeded()const{return succeeded_;}
    template<class World> bool boundTo(const World& world)const{return context_==&world;}
    unsigned size()const{unsigned n=0;for(unsigned i=0;i<count_;++i)if(!slots_[i].effect.destroyRequested)++n;return n;}
    std::uint32_t sourceId(Handle h)const{for(unsigned i=0;i<count_;++i)if(slots_[i].handle==h)return slots_[i].sourceId;return 0;}
    Phase beginPhase()const{return {next_?next_-1:0xffffffffu};}
    bool clear(){
        if(!succeeded_)return false;
        for(unsigned i=0;i<count_;++i)slots_[i].effect.destroyRequested=true;
        return retireRequested();
    } // Do not reuse identities across room reset.
    template<class Pause> bool create(int& speed,Pause pause){
        // A registered caller must supply the source private xx/yy explicitly.
        if(context_)return false;
        return createAt({}, {},speed,pause);
    }
    template<class Pause> bool createAt(MotionBinary64 x,MotionBinary64 y,int& speed,Pause pause){
        if(!succeeded_)return false;
        if(count_==Capacity || !next_)return fail();
        RunnerImageValue px,py;if(!px.assign(x)||!py.assign(y))return fail();
        return createNarrowed(px,py,speed,pause);
    }
    // createAt on a Q16 position (the compact world, W3.20).
    template<class Pause> bool createAtQ16(std::int32_t x16,std::int32_t y16,int& speed,Pause pause){
        if(!succeeded_)return false;
        if(count_==Capacity || !next_)return fail();
        RunnerImageValue px,py;if(!px.assignQ16(x16)||!py.assignQ16(y16))return fail();
        return createNarrowed(px,py,speed,pause);
    }
private:
    template<class Pause> bool createNarrowed(RunnerImageValue px,RunnerImageValue py,int& speed,Pause pause){
        // Insert before synchronous Create. A failing pause leaves the source
        // assignments and instance present; caller must latch the failure.
        for(unsigned i=count_;i>0;--i)slots_[i]=slots_[i-1];
        ++count_;slots_[0]={};auto& slot=slots_[0];slot.handle=next_++;slot.sourceId=slot.handle;
        if(birth_&&!birth_(context_,slot.handle,px,py,slot.sourceId))return fail();
        if(!slot.sourceId||slot.sourceId>0x7fffffffu)return fail();
        for(unsigned i=1;i<count_;++i)if(slots_[i].sourceId==slot.sourceId)return fail();
        if(!slot.effect.create(speed,[&](unsigned ms){
            return (!publish_||publish_(context_,slot.handle,slot.sourceId,slot.effect))&&pause(ms);
        }))return fail();
        return true;
    }
public:
    void advanceAlarm(Phase phase,unsigned index,int& speed){
        if(!succeeded_||index>1)return;
        for(unsigned i=0;i<count_;++i){
            auto& slot=slots_[i];
            if(slot.handle>phase.newest||slot.effect.destroyRequested)continue;
            bool eligible=true;
            if(eligible_&&!eligible_(context_,slot.handle,slot.sourceId,eligible)){fail();return;}
            if(!eligible)continue;
            slot.effect.advanceAlarm(index,speed);
            if(publish_&&!publish_(context_,slot.handle,slot.sourceId,slot.effect)){fail();return;}
            if(slot.effect.destroyRequested){
                if(destroyed_&&!destroyed_(context_,slot.handle,slot.sourceId,slot.effect)){fail();return;}
            }
        }
        if(!context_)retireRequested();
    }
    // World-selected fired event, not a family-wide countdown. The registered
    // record owns alarm storage while using this path; import both counters
    // before the handler so publication cannot restore a stale sibling alarm.
    bool dispatchAlarm(Handle handle,std::uint32_t id,const AlarmBank& alarms,unsigned index,int& speed){
        if(!succeeded_)return false;
        if(!context_||index>1||alarms.get(index)!=0)return fail();
        for(unsigned i=0;i<count_;++i){auto& slot=slots_[i];
            if(slot.handle!=handle)continue;
            if(slot.sourceId!=id||slot.effect.destroyRequested)return fail();
            bool eligible=true;
            if(!eligible_(context_,handle,id,eligible))return fail();
            if(!eligible)return true;
            slot.effect.alarm0=alarms.get(0);slot.effect.alarm1=alarms.get(1);
            slot.effect.fireAlarm(index,speed);
            if(!publish_(context_,handle,id,slot.effect))return fail();
            if(slot.effect.destroyRequested&&!destroyed_(context_,handle,id,slot.effect))return fail();
            return true;
        }
        return fail();
    }
    // Global cleanup selects one lifetime at a time across all object families.
    // Never sweep siblings here: their place in room-list order may be later.
    bool retireMarked(Handle handle,std::uint32_t id){
        if(!succeeded_)return false;
        for(unsigned i=0;i<count_;++i){auto& slot=slots_[i];
            if(slot.handle!=handle)continue;
            if(slot.sourceId!=id||!slot.effect.destroyRequested)return fail();
            if(retire_&&!retire_(context_,handle,id,slot.effect))return fail();
            for(unsigned j=i+1;j<count_;++j)slots_[j-1]=slots_[j];
            --count_;return true;
        }
        return fail();
    }
    bool retireRequested(){
        if(!succeeded_)return false;
        for(unsigned i=0;i<count_;){
            auto& slot=slots_[i];if(!slot.effect.destroyRequested){++i;continue;}
            if(retire_&&!retire_(context_,slot.handle,slot.sourceId,slot.effect))return fail();
            for(unsigned j=i+1;j<count_;++j)slots_[j-1]=slots_[j];
            --count_;
        }
        return true;
    }
private:
    bool fail(){succeeded_=false;return false;}
    Slot slots_[Capacity];unsigned count_=0;Handle next_=1;
    bool succeeded_=true;void* context_=nullptr;
    bool (*birth_)(void*,Handle,RunnerImageValue,RunnerImageValue,std::uint32_t&)=nullptr;
    bool (*publish_)(void*,Handle,std::uint32_t,const TimeChangeEffect&)=nullptr;
    bool (*eligible_)(void*,Handle,std::uint32_t,bool&)=nullptr;
    bool (*destroyed_)(void*,Handle,std::uint32_t,const TimeChangeEffect&)=nullptr;
    bool (*retire_)(void*,Handle,std::uint32_t,const TimeChangeEffect&)=nullptr;
};
}
#endif
