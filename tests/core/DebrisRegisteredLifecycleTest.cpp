#include "core/DebrisEffects.h"
#include "core/GameMakerRandom.h"
#ifdef DOWNWELL_NATIVE_QUERY_PROBE
struct Lifecycle {
    bool birth(unsigned,const downwell::DebrisAnimation&);
    bool image(unsigned,const downwell::DebrisAnimation&);
    bool destroyed(unsigned,const downwell::DebrisAnimation&);
    bool retire(unsigned,const downwell::DebrisAnimation&);
};
bool nativeRegisteredDebris(downwell::DebrisEffects<2>& owner,downwell::GameMakerRandom& random,Lifecycle& life){
    const auto h=owner.createRegistered({}, {},1,random,life);const auto phase=owner.beginPhase();
    return h&&owner.advanceRegisteredAnimationOne(h,phase,life)&&
        owner.advanceRegisteredAlarmOne(h,phase,0,life)&&owner.stepRegisteredOne(h,phase,random,
            [](auto,const auto&,bool& frozen){frozen=false;return true;},
            [](auto,const auto&,auto,auto,bool& hit){hit=true;return true;},
            [](auto,const auto&,auto,auto){return true;},life)&&owner.retireMarked(h,life);
}
#else
#include <cassert>
#include <cstdio>
#include <vector>
using namespace downwell;
struct Lifecycle {
    std::vector<unsigned> births,images,marks,deaths;
    bool rejectMark=false,rejectImage=false,rejectRetire=false;
    bool birth(unsigned h,const DebrisAnimation&){births.push_back(h);return true;}
    bool image(unsigned h,const DebrisAnimation&){images.push_back(h);return !rejectImage;}
    bool destroyed(unsigned h,const DebrisAnimation& e){assert(e.destroyRequested);marks.push_back(h);return !rejectMark;}
    bool retire(unsigned h,const DebrisAnimation&){deaths.push_back(h);return !rejectRetire;}
};
int main(){
    for(bool byAlarm:{false,true}){
        DebrisEffects<1> owner;GameMakerRandom random(42);Lifecycle life;
        const auto before=owner.beginPhase();const auto h=owner.createRegistered({}, {},1,random,life);
        assert(h);auto* effect=owner.get(h);assert(effect);const auto original=*effect;
        unsigned queries=0,writes=0,freezes=0;
        auto stopped=[&](auto key,const auto&,bool& frozen){assert(key==h);++freezes;frozen=false;return true;};
        auto meeting=[&](auto key,const auto&,auto,auto,bool& hit){assert(key==h);++queries;hit=true;return true;};
        auto publish=[&](auto key,const auto&,auto,auto){assert(key==h);++writes;return true;};
        assert(owner.advanceRegisteredAnimationOne(h,before,life)&&life.images.empty());
        assert(owner.advanceRegisteredAlarmOne(h,before,0,life)&&effect->alarms.get(0)==30);
        assert(owner.stepRegisteredOne(h,before,random,stopped,meeting,publish,life)&&!queries&&!writes&&!freezes);
        const auto phase=owner.beginPhase();
        assert(owner.advanceRegisteredAnimationOne(h,phase,life)&&life.images==std::vector<unsigned>{h});
        if(byAlarm){effect->alarms.set(0,1);assert(owner.advanceRegisteredAlarmOne(h,phase,0,life));}
        else{effect->disappearing=true;assert(owner.stepRegisteredOne(h,phase,random,stopped,meeting,publish,life));
            assert(writes==2&&queries==8&&effect->flash==1);}
        assert(!owner.get(h)&&owner.retained(h)==effect&&effect->destroyRequested&&life.marks==std::vector<unsigned>{h});
        const auto draws=random.drawCount();const auto q=queries,w=writes,f=freezes;
        assert(!owner.createRegistered({}, {},1,random,life)&&owner.valid()&&random.drawCount()==draws&&life.deaths.empty());
        assert(owner.advanceRegisteredAnimationOne(h,phase,life)&&life.images.size()==1);
        assert(owner.advanceRegisteredAlarmOne(h,phase,0,life)&&life.marks.size()==1);
        assert(owner.stepRegisteredOne(h,phase,random,stopped,meeting,publish,life)&&queries==q&&writes==w&&freezes==f);
        assert(owner.retireMarked(h,life)&&!owner.retained(h)&&life.deaths==std::vector<unsigned>{h});
        const auto next=owner.createRegistered({}, {},1,random,life);assert(next>h&&owner.retained(next));
        assert(owner.advanceRegisteredAnimationOne(next,phase,life)&&owner.get(next)->timeline.bits==original.timeline.bits);
    }
    for(unsigned failure=0;failure<3;++failure){
        DebrisEffects<1> owner;GameMakerRandom random(7);Lifecycle life;
        const auto h=owner.createRegistered({}, {},1,random,life);auto* retained=owner.get(h);assert(retained);
        const auto phase=owner.beginPhase();
        if(failure==0){life.rejectImage=true;assert(!owner.advanceRegisteredAnimationOne(h,phase,life));}
        else{
            retained->alarms.set(0,1);life.rejectMark=failure==1;
            assert(owner.advanceRegisteredAlarmOne(h,phase,0,life)==(failure==2));
            if(failure==2){life.rejectRetire=true;assert(!owner.retireMarked(h,life));}
            assert(retained->destroyRequested);
        }
        assert(!owner.valid());const auto marks=life.marks.size(),images=life.images.size();const auto draws=random.drawCount();
        assert(!owner.advanceRegisteredAnimationOne(h,phase,life)&&!owner.advanceRegisteredAlarmOne(h,phase,0,life));
        assert(!owner.createRegistered({}, {},1,random,life)&&random.drawCount()==draws&&life.marks.size()==marks&&life.images.size()==images);
        life.rejectRetire=false;assert(owner.clear(life)&&owner.valid());
    }
    std::puts("Registered debris: deferred destruction, retained identity, cleanup-only reuse, phase exclusion and terminal publication failures passed");
}
#endif
