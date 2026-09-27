#include "core/TimeChangeEffects.h"
#include <cassert>
#include <cstdio>
using namespace downwell;
struct Registration {
    int& speed;unsigned births=0,publishes=0,destroys=0,retires=0;bool active=true;
    unsigned failure=0;
    bool birth(unsigned h,RunnerImageValue x,RunnerImageValue y,std::uint32_t& id){
        assert(h&&speed==60&&x.bits==0x43704000u&&y.bits==0x43308000u);
        ++births;id=105186;return failure!=1;
    }
    bool publish(unsigned,unsigned id,const TimeChangeEffect& effect){
        assert(id==105186&&effect.alarm1>=0);++publishes;return failure!=2;
    }
    bool eventEligible(unsigned,unsigned,bool& eligible){eligible=active;return failure!=3;}
    bool destroyed(unsigned,unsigned,const TimeChangeEffect& effect){assert(effect.destroyRequested);++destroys;return failure!=4;}
    bool retire(unsigned,unsigned,const TimeChangeEffect& effect){assert(effect.destroyRequested);++retires;return failure!=5;}
};
int main(){
    const auto x=RunnerImageValue{0x43704000u}.promoted(),y=RunnerImageValue{0x43308000u}.promoted();
    for(unsigned failure=0;failure<7;++failure){
        int registeredSpeed=60;Registration world{registeredSpeed};world.failure=failure;
        TimeChangeEffects<1> registered;assert(registered.bind(world));unsigned pauses=0;
        auto pause=[&](unsigned ms){assert(ms==30&&registeredSpeed==30&&world.births==1&&world.publishes==1);
            ++pauses;return failure!=6;};
        assert(!registered.create(registeredSpeed,pause)&&world.births==0); // No guessed private coordinates.
        const bool created=registered.createAt(x,y,registeredSpeed,pause);
        if(failure==1||failure==2||failure==6){
            assert(!created&&!registered.succeeded()&&world.births==1);
            assert(registeredSpeed==(failure==1?60:30)&&pauses==(failure==6?1u:0u));
            assert(!registered.clear());continue;
        }
        assert(created&&registered.sourceId(1)==105186);
        const auto publications=world.publishes;
        registered.advanceAlarm(registered.beginPhase(),2,registeredSpeed);
        assert(world.publishes==publications&&registered.succeeded());
        if(failure==0){
            world.active=false;for(unsigned i=0;i<20;++i)registered.advanceAlarm(registered.beginPhase(),1,registeredSpeed);
            assert(registeredSpeed==30&&world.publishes==1);world.active=true;
        }
        for(unsigned i=0;i<14;++i)registered.advanceAlarm(registered.beginPhase(),1,registeredSpeed);
        if(failure==3||failure==4){assert(!registered.succeeded()&&!registered.clear());continue;}
        assert(registered.succeeded()&&registered.size()==0&&world.destroys==1&&world.retires==0);
        assert(!registered.createAt(x,y,registeredSpeed,pause)&&world.births==1);
        assert(registered.retireRequested()==(failure!=5)&&world.retires==1);
        if(failure==5)assert(!registered.succeeded()&&!registered.retireRequested()&&world.retires==1);
        else assert(registered.sourceId(1)==0);
    }
    TimeChangeEffects<3> owner;int speed=60;unsigned pauses=0;
    auto pause=[&](unsigned ms){assert(ms==30 && speed==30);++pauses;return true;};
    const auto empty=owner.beginPhase();assert(owner.create(speed,pause));
    for(unsigned i=0;i<4;++i)owner.advanceAlarm(empty,1,speed);
    assert(speed==30 && owner.size()==1);
    for(unsigned tick=1;tick<=14;++tick){
        const auto phase=owner.beginPhase();owner.advanceAlarm(phase,0,speed);owner.advanceAlarm(phase,1,speed);
        assert(speed==int(tick<12?30+5*(tick/2):60));assert(owner.size()==unsigned(tick<14));
    }
    assert(owner.create(speed,pause) && owner.create(speed,pause));
    for(unsigned tick=1;tick<=8;++tick){
        const auto phase=owner.beginPhase();owner.advanceAlarm(phase,0,speed);owner.advanceAlarm(phase,1,speed);
        assert(speed==int(tick<6?30+10*(tick/2):60));
    }
    assert(owner.size()==0);
    assert(owner.create(speed,pause) && owner.create(speed,pause) && owner.create(speed,pause));
    assert(!owner.create(speed,pause) && pauses==6);
    const auto stale=owner.beginPhase();owner.clear();assert(owner.create(speed,pause));
    owner.advanceAlarm(stale,1,speed);owner.advanceAlarm(stale,1,speed);assert(speed==30);
    const auto phase=owner.beginPhase();owner.advanceAlarm(phase,1,speed);owner.advanceAlarm(phase,1,speed);assert(speed==35);
    owner.clear();assert(!owner.create(speed,[](unsigned){return false;}));assert(owner.size()==1 && speed==30);
    std::puts("Time-change ownership: single/overlapping ramps, phase cutoffs, capacity, reset and partial failure passed");
}
