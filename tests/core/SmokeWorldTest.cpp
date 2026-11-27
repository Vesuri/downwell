#include "core/GameplayWorld.h"
#include "core/StartupRuntime.h"
#include "core/EventScheduler.h"
#include "core/RoomPack.h"
#include "core/BuilderPatternPack.h"
#include <cassert>
#include <fstream>
#include <iterator>
#include <vector>
#include <cstdio>
using namespace downwell;
static std::vector<unsigned char> load(const char* name){std::ifstream f(name,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
static void equalRandom(const GameMakerRandom& a,const GameMakerRandom& b){assert(a.drawCount()==b.drawCount());for(unsigned i=0;i<16;++i)assert(a.stateWord(i)==b.stateWord(i));}
template<class World> static void flashFailure(World& world){
    unsigned bursts=0;
    while(world.emitFlashBurst({},{}))assert(++bursts<9);
    assert(!world.effectsSucceeded());
    const auto random=world.randomState();
    const auto births=world.flashEffects().birthSequence().next;
    assert(!world.emitFlashBurst({},{}));
    assert(!world.emitSmokeBurst({},{}));
    equalRandom(random,world.randomState());
    world.step(0);
    equalRandom(random,world.randomState());
    assert(world.flashEffects().birthSequence().next==births);
}
int main(){
    // Native world: fixture births during generation belong to a later Step.
    GameplayWorld world(0,true,0,false);
    world.configureEnemyRandom(GameMakerRandom(84));
    bool born=false;
    world.configureGenerationStep([](void* context,GameplayWorld& w,GameMakerRandom&,int){
        auto& born=*static_cast<bool*>(context);
        if(!born){born=true;return w.emitSmokeBurst(downwell::Fixed16::fromInteger(240),downwell::Fixed16::fromInteger(176));}
        return true;
    },&born);
    world.step(0);assert(born && world.effectsSucceeded() && world.smokeEffects().size()>=4);
    world.smokeEffects().visitNewest([](auto,const auto& e){assert(!e.allSet && e.frame16==0);});
    auto random=world.randomState();
    auto expected=world.smokeEffects();
    auto phase=expected.beginPhase();assert(expected.advanceAnimation(phase)&&expected.step(phase,random));
    world.step(0);assert(world.effectsSucceeded());equalRandom(world.randomState(),random);
    world.smokeEffects().visitNewest([&](auto handle,const auto& e){
        const auto* r=expected.get(handle);assert(r && e.allSet);
        assert(e.xx==r->xx && e.yy==r->yy);
        assert(e.frame16==r->frame16 && e.rate16==r->rate16);
    });
    for(unsigned i=0;i<100;++i)world.step(0);
    assert(world.effectsSucceeded() && world.smokeEffects().size()==0);

    // Retry reconstructs GameplayWorld in place. It must not reuse the old
    // world's effect keys, unlike a genuinely new independent identity domain.
    assert(world.emitSmokeBurst({}, {})&&world.emitFlashBurst({}, {}));
    const auto smokeSequence=world.smokeEffects().birthSequence();
    const auto flashSequence=world.flashEffects().birthSequence();
    world.restartFromResults(DeathMenuSequence::Action::Retry,84,0);
    assert(world.effectsSucceeded());
    assert(world.smokeEffects().birthSequence().next==smokeSequence.next);
    assert(world.flashEffects().birthSequence().next==flashSequence.next);
    assert(world.emitSmokeBurst({}, {})&&world.emitFlashBurst({}, {}));
    world.smokeEffects().visitNewest([&](auto key,const auto&){assert(key>=smokeSequence.next);});
    for(unsigned i=0;i<32;++i)if(world.flashEffects().slot(i).handle)
        assert(world.flashEffects().slot(i).handle>=flashSequence.next);
    flashFailure(world);

    // Scheduler cutoff must follow alarm births and precede ordinary-Step births.
    InstanceStore<1> instances;instances.create(1,1,1,false);auto* instance=instances.find(1);
    instance->alarms.set(0,1);
    SmokeEffects<8> owner;GameMakerRandom rng(12);
    SmokeEffects<8>::Phase cutoff{};unsigned alarmBirth=0,stepBirth=0;
    EventScheduler<1>::runStep(instances,[](const auto&){return AlarmBank::allEvents;},
        [&](auto&,StepEvent e){if(e.phase==StepEventPhase::alarm)alarmBirth=owner.create({}, {},rng);
            if(e.phase==StepEventPhase::step)stepBirth=owner.create({}, {},rng);},
        [&](StepEventPhase phase){if(phase==StepEventPhase::step)cutoff=owner.beginPhase();});
    assert(owner.step(cutoff,rng));assert(owner.get(alarmBirth)->allSet && !owner.get(stepBirth)->allSet);

    auto rooms=load("generated/downwell-rooms.dwrm"),objects=load("generated/downwell-objects.dwob"),patterns=load("generated/downwell-builder-patterns.dwbp");
    RoomPack roomPack(rooms.data(),rooms.size());ObjectPack objectPack(objects.data(),objects.size());
    BuilderPatternPack builderPack(patterns.data(),patterns.size());
    assert(roomPack.valid()&&objectPack.valid()&&builderPack.valid());
    StartupRuntime host(PlatformProfile::desktop,&roomPack,&objectPack,84,&builderPack);
    host.skipToGameForTesting();assert(host.inMainRoom()&&host.roomLoadSucceeded());
    auto hostRandom=host.randomState();SmokeEffects<32> hostOracle;
    assert(hostOracle.skeletonBurst(Fixed16::fromInteger(240),Fixed16::fromInteger(1250),hostRandom));
    assert(host.emitSmokeBurst(MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(1250)));
    equalRandom(host.randomState(),hostRandom);
    assert(host.smokeEffects().size()==hostOracle.size());
    host.step(false);assert(host.effectsSucceeded());
    host.smokeEffects().visitNewest([](auto,const auto& e){assert(e.allSet && e.frame16!=0);});
    for(unsigned i=0;i<100;++i)host.step(false);
    assert(host.effectsSucceeded() && host.smokeEffects().size()==0);
    unsigned attempts=0;
    while(host.emitSmokeBurst({},{})){assert(++attempts<10);}
    assert(!host.effectsSucceeded());
    const auto draws=host.randomDrawCount();assert(!host.emitSmokeBurst({},{}));assert(draws==host.randomDrawCount());
    const auto hostSequence=host.smokeEffects().birthSequence();
    host.reset(PlatformProfile::desktop);assert(host.effectsSucceeded()&&host.smokeEffects().size()==0);
    assert(host.smokeEffects().birthSequence().next==hostSequence.next);
    assert(host.emitSmokeBurst({},{}));
    host.smokeEffects().visitNewest([&](auto key,const auto&){assert(key>=hostSequence.next);});
    flashFailure(host);
    std::puts("Smoke worlds: Create RNG, generation/phase exclusion, next-step motion, host lifetime, exhaustion and reset passed");
}
