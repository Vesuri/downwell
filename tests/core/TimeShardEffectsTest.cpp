#include "core/TimeShardEffects.h"
#include "core/GameMakerRandom.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace downwell;
struct World {
    GameMakerRandom& rng;
    explicit World(GameMakerRandom& random):rng(random){}
    unsigned births=0,creates=0,retires=0,fail=0,failRetireAt=0;
    std::uint32_t nextId=105186;
    std::vector<unsigned> events;
    unsigned animations=0,steps=0;
    bool publishAnimation(std::uint32_t,std::uint32_t,const TimeShardAnimation&){++animations;return fail!=4;}
    bool publishStep(std::uint32_t,std::uint32_t,const TimeShardAnimation&){++steps;return fail!=5;}
    bool publishAlarm(std::uint32_t,std::uint32_t,const TimeShardAnimation&){return fail!=6;}
    bool birth(std::uint32_t h,RunnerImageValue x,RunnerImageValue y,std::uint32_t& id){
        assert(h && x.promoted().valid() && y.promoted().valid());++births;events.push_back(1);
        assert(rng.drawCount()==6*creates);
        id=nextId++;
        return fail!=1;
    }
    bool created(unsigned h,unsigned id,const TimeShardAnimation& s){
        assert(h && id>=105186 && s.alarm0>=90);++creates;events.push_back(2);
        assert(rng.drawCount()==6*creates);return fail!=2;
    }
    bool retire(unsigned h,unsigned id,const TimeShardAnimation& s){
        assert(h && id>=105186 && s.destroyRequested);++retires;events.push_back(3);
        return fail!=3 && retires!=failRetireAt;
    }
};
int main(){
    for(unsigned failure:{4u,5u,6u}){
        GameMakerRandom random(42);World world(random);TimeShardEffects<3> effects;
        assert(effects.bindPhases(world)&&effects.phasesBoundTo(world));
        auto old=effects.beginPhase();assert(effects.create({}, {},false,random));
        assert(effects.advanceAnimation(old)&&world.animations==0);
        world.fail=failure;
        if(failure==4)assert(!effects.advanceAnimation(effects.beginPhase()));
        else if(failure==5)assert(!effects.step(effects.beginPhase(),true,random,
            [](auto,const auto&,auto,auto,bool& hit){hit=false;return true;},
            [](auto,const auto&,bool& retire){retire=false;return true;}));
        else effects.advanceAlarm(effects.beginPhase(),2);
        assert(!effects.succeeded()&&!effects.clear());
        const auto animations=world.animations;
        assert(!effects.advanceAnimation(effects.beginPhase())&&world.animations==animations);
    }
    {
        GameMakerRandom random(42);World world(random);TimeShardEffects<3> owned;
        assert(owned.bind(world));
        const auto a=owned.create({}, {},false,random),b=owned.create({}, {},false,random),
                   c=owned.create({}, {},false,random);
        world.failRetireAt=2;
        assert(!owned.clear());
        assert(owned.sourceId(c)==0 && owned.sourceId(b)==105187 && owned.sourceId(a)==105186);
        assert(world.retires==2 && !owned.clear() && world.retires==2);
        assert(!owned.advanceAnimation(owned.beginPhase()));
    }
    {
        GameMakerRandom random(42);World world(random);TimeShardEffects<3> owned;
        assert(owned.bind(world));assert(owned.create({}, {},false,random));
        world.nextId=105186; // Registration may not alias another live source ID.
        assert(!owned.create({}, {},false,random) && !owned.succeeded());
        assert(random.drawCount()==6 && world.births==2 && world.creates==1);
    }
    // World numeric IDs are independent from private phase handles. Register
    // before RNG, publish completed Create, retire before slot reuse/reset.
    for(unsigned fail=0;fail<=3;++fail){
        GameMakerRandom random(42);World world{random};TimeShardEffects<2> owned;
        assert(owned.bind(world));assert(!owned.bind(world));world.fail=fail;
        const auto h=owned.create(MotionBinary64::fromInteger(12),{},true,random);
        if(fail==1||fail==2){
            assert(!h&&!owned.succeeded());
            assert(random.drawCount()==(fail==1?0:6));
            assert(!owned.create({}, {},true,random));assert(!owned.clear());
            assert(world.births==1 && world.retires==0);continue;
        }
        assert(h==1 && owned.sourceId(h)==105186 && owned.get(h));
        owned.get(h)->alarm0=1;owned.advanceAlarm(owned.beginPhase(),0);
        assert(world.retires==1);
        if(fail==3){
            assert(!owned.succeeded() && owned.sourceId(h)==105186);
            assert(!owned.clear() && !owned.create({}, {},true,random));
            assert(world.retires==1);continue;
        }
        assert(owned.sourceId(h)==0);
        const auto replacement=owned.create({}, {},true,random);
        assert(replacement==2 && owned.sourceId(replacement)==105187);
        assert(owned.clear() && owned.sourceId(replacement)==0);
        assert((world.events==std::vector<unsigned>{1,2,3,1,2,3}));
    }
    TimeShardEffects<6> owner;GameMakerRandom rng(42);
    const auto empty=owner.beginPhase();
    const auto first=owner.create({}, {},true,rng);assert(first);
    const auto phase=owner.beginPhase();
    const auto second=owner.create({}, {},true,rng);assert(second>first);
    const auto initial=owner.get(first)->imageIndex.bits;
    assert(owner.advanceAnimation(empty));assert(owner.get(first)->imageIndex.bits==initial);
    owner.advanceAlarm(phase,2);assert(owner.get(first)->alarm2==29 && owner.get(second)->alarm2==30);
    std::vector<unsigned> visited;
    auto noWall=[&](auto handle,const auto&,auto,auto,bool& hit){visited.push_back(handle);hit=false;return true;};
    auto onScreen=[](auto,const auto&,bool& retire){retire=false;return true;};
    assert(owner.step(phase,true,rng,noWall,onScreen));
    assert(visited.size()==3);for(auto h:visited)assert(h==first);
    for(unsigned i=0;i<4;++i)assert(owner.create({}, {},true,rng));
    const auto draws=rng.drawCount();assert(owner.size()==6);
    assert(!owner.create({}, {},true,rng) && rng.drawCount()==draws);
    // Low-spec population limit is enforced by Step, not by Create. Newest
    // excess instances retire one at a time; later Steps see the reduced count.
    visited.clear();assert(owner.step(owner.beginPhase(),true,rng,noWall,onScreen));
    assert(owner.size()==3 && owner.get(first) && owner.get(second));
    assert(!owner.get(4) && !owner.get(5) && !owner.get(6));
    owner.get(second)->alarm0=1;owner.get(second)->alarm2=1;
    const auto alarms=owner.beginPhase();owner.advanceAlarm(alarms,0);owner.advanceAlarm(alarms,2);
    assert(!owner.get(second));assert(owner.playerContact(second,{}, {},rng));assert(rng.drawCount()==draws);
    auto* shard=owner.get(first);shard->noTouch=false;
    assert(owner.playerContact(first,{}, {},rng));assert(rng.drawCount()==draws+2);
    assert(shard->noTouch && shard->alarm2==5);
    const auto stale=owner.beginPhase();owner.clear();assert(owner.size()==0 && !owner.get(first));
    const auto replacement=owner.create({}, {},false,rng);assert(replacement>stale.newest);
    const auto life=owner.get(replacement)->alarm0;owner.advanceAlarm(stale,0);
    assert(owner.get(replacement)->alarm0==life);
    unsigned calls=0;
    assert(!owner.step(owner.beginPhase(),false,rng,[&](auto,const auto&,auto,auto,bool&){++calls;return false;},onScreen));
    assert(calls==1 && owner.get(replacement));
    assert(!owner.clear()&&!owner.succeeded());
    // Repeated slot reuse must never revalidate a stale handle/cutoff.
    TimeShardEffects<6> reuse;
    for(unsigned run=0;run<1024;++run){
        const auto old=reuse.beginPhase();const auto h=reuse.create({}, {},false,rng);assert(h>old.newest);
        auto* s=reuse.get(h);s->alarm0=2;
        reuse.advanceAlarm(old,0);assert(s->alarm0==2);
        const auto current=reuse.beginPhase();
        assert(reuse.advanceAnimation(current));reuse.advanceAlarm(current,0);
        assert(reuse.step(current,true,rng,noWall,onScreen));assert(s->alarm0==2);
        reuse.advanceAlarm(current,0);reuse.advanceAlarm(current,0);assert(!reuse.get(h));
    }
    std::puts("Shard ownership: phase cutoffs, live population, alarm retirement, contact RNG, failure and 1024 reuse cycles passed");
}
