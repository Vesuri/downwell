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
    std::vector<unsigned> retiredIds;
    unsigned animations=0,steps=0;
    bool publishAnimation(std::uint32_t,std::uint32_t,const TimeShardAnimation&){++animations;return fail!=4;}
    bool publishStep(std::uint32_t,std::uint32_t,const TimeShardAnimation&){++steps;return fail!=5;}
    unsigned alarmPublications=0;
    bool publishAlarm(std::uint32_t,std::uint32_t,const TimeShardAnimation&){++alarmPublications;return fail!=6;}
    unsigned destroys=0;
    bool active=true;
    bool animationMember=true;
    bool animationEligible(unsigned,unsigned,bool& eligible){eligible=animationMember;return fail!=10;}
    bool eventEligible(unsigned,unsigned,bool& eligible){eligible=active;return fail!=9;}
    bool population(unsigned& count){count=creates-destroys;return fail!=8;}
    bool destroyed(std::uint32_t,std::uint32_t,const TimeShardAnimation&){++destroys;return fail!=7;}
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
        retiredIds.push_back(id);
        return fail!=3 && retires!=failRetireAt;
    }
};
int main(){
    {
        GameMakerRandom random(42);World world(random);TimeShardEffects<2> effects;
        assert(effects.bindPhases(world));const auto a=effects.create({},{},false,random),b=effects.create({},{},false,random);
        auto expected=*effects.get(a);const auto other=*effects.get(b);
        assert(expected.advanceImage()&&effects.advanceAnimationFor(a,effects.sourceId(a)));
        assert(world.animations==1&&effects.get(a)->timeline.bits==expected.timeline.bits&&
            effects.get(b)->timeline.bits==other.timeline.bits&&effects.get(b)->imageIndex.bits==other.imageIndex.bits);
        world.animationMember=false;
        assert(effects.advanceAnimationFor(a,effects.sourceId(a))&&world.animations==1);
        world.animationMember=true;world.fail=4;
        assert(!effects.advanceAnimationFor(a,effects.sourceId(a))&&!effects.succeeded()&&world.animations==2);
        assert(!effects.advanceAnimationFor(b,effects.sourceId(b))&&world.animations==2);
    }
    for(unsigned fault=0;fault<7;++fault){
        GameMakerRandom random(42);World world(random);TimeShardEffects<1> effects;
        assert(effects.bindPhases(world));const auto h=effects.create({},{},false,random);
        AlarmBank alarms;alarms.set(0,0);alarms.set(2,fault==0?1:0);
        if(fault>=4)world.fail=fault==4?9:fault==5?6:7;
        assert(!effects.dispatchAlarm(fault==1?h+1:h,fault==2?999:effects.sourceId(h),alarms,
            fault==3?3:fault==6?0:2));
        assert(!effects.succeeded()&&random.drawCount()==6&&world.alarmPublications==unsigned(fault>=5)&&
            world.destroys==unsigned(fault==6));
        assert(!effects.dispatchAlarm(h,effects.sourceId(h),alarms,0)&&world.alarmPublications==unsigned(fault>=5));
    }
    for(unsigned failure=0;failure<4;++failure){
        GameMakerRandom random(42);World world(random);TimeShardEffects<1> effects;
        assert(effects.bindPhases(world));const auto h=effects.create({}, {},false,random);
        effects.get(h)->noTouch=false;const auto draws=random.drawCount();
        const MotionBinary64 invalid{0x7ff00000u,0};
        if(failure==2)world.fail=6; // Publication fails after both velocity draws.
        if(failure==3)world.fail=9; // Eligibility fails before any mutation.
        assert(!effects.playerContact(h,failure==0?invalid:MotionBinary64{},
            failure==1?invalid:MotionBinary64{},random));
        const unsigned consumed=failure==0||failure==3?0:failure==1?1:2;
        assert(!effects.succeeded()&&random.drawCount()==draws+consumed);
        assert(!effects.playerContact(h,{},{},random)&&random.drawCount()==draws+consumed);
        assert(!effects.clear());
    }
    {
        GameMakerRandom random(42);World world(random);TimeShardEffects<1> effects;
        assert(effects.bindPhases(world));const auto h=effects.create({}, {},false,random);
        effects.get(h)->noTouch=false;world.active=false;const auto draws=random.drawCount();
        assert(effects.playerContact(h,{},{},random)&&random.drawCount()==draws&&!effects.get(h)->noTouch);
        world.active=true;
        assert(effects.playerContact(h,{},{},random)&&random.drawCount()==draws+2&&effects.get(h)->alarm2==5);
        world.fail=6; // Cooldown guard must not attempt redundant alarm publication.
        assert(effects.playerContact(h,{},{},random)&&effects.succeeded()&&random.drawCount()==draws+2);
    }
    {
        GameMakerRandom random(42);World world(random);TimeShardEffects<1> effects;
        assert(effects.bindPhases(world));const auto h=effects.create({}, {},false,random);
        auto* shard=effects.get(h);shard->alarm0=2;
        const auto before=shard->xx;const auto draws=random.drawCount();
        world.active=false;effects.advanceAlarm(effects.beginPhase(),0);
        assert(effects.advanceAnimation(effects.beginPhase())&&world.animations==1);
        world.animationMember=false;
        assert(effects.advanceAnimation(effects.beginPhase())&&world.animations==1);
        assert(effects.step(effects.beginPhase(),false,random,
            [](auto,const auto&,auto,auto,bool&){assert(false);return false;},
            [](auto,const auto&,bool&){assert(false);return false;}));
        assert(shard->alarm0==2&&shard->xx.high==before.high&&shard->xx.low==before.low&&
            random.drawCount()==draws&&world.steps==0);
        world.active=true;effects.advanceAlarm(effects.beginPhase(),0);
        assert(shard->alarm0==1&&effects.succeeded());
        world.fail=9;effects.advanceAlarm(effects.beginPhase(),0);
        assert(!effects.succeeded()&&shard->alarm0==1&&world.destroys==0);
    }
    {
        GameMakerRandom random(42);World world(random);TimeShardEffects<3> effects;
        assert(effects.bindPhases(world));
        const auto a=effects.create({}, {},false,random),b=effects.create({}, {},false,random),
                   c=effects.create({}, {},false,random);
        effects.get(a)->alarm0=effects.get(b)->alarm0=effects.get(c)->alarm0=1;
        effects.advanceAlarm(effects.beginPhase(),0);
        assert(world.retires==0&&effects.size()==0);
        // Room depth/activation order need not be this family's newest order.
        assert(effects.retireMarked(b,105187)&&effects.sourceId(a)==105186&&effects.sourceId(c)==105188);
        assert(effects.retireMarked(a,105186)&&effects.retireMarked(c,105188));
        assert((world.retiredIds==std::vector<unsigned>{105187,105186,105188}));
        const auto replacement=effects.create({}, {},false,random);
        assert(replacement>c&&effects.sourceId(replacement)==105189);
        assert(!effects.retireMarked(b,105187)&&world.retires==3&&effects.sourceId(replacement)==105189);
        assert(!effects.succeeded()&&!effects.clear());
    }
    for(unsigned bad=0;bad<3;++bad){
        GameMakerRandom random(42);World world(random);TimeShardEffects<1> effects;
        assert(effects.bindPhases(world));const auto h=effects.create({}, {},false,random);
        if(bad!=0){effects.get(h)->alarm0=1;effects.advanceAlarm(effects.beginPhase(),0);}
        if(bad==2)world.failRetireAt=1;
        assert(!effects.retireMarked(h,bad==1?105187:105186));
        assert(!effects.succeeded()&&effects.sourceId(h)==105186&&world.retires==(bad==2?1u:0u));
        assert(!effects.retireMarked(h,105186)&&world.retires==(bad==2?1u:0u));
    }
    for(bool reject:{false,true}){
        GameMakerRandom random(42);World world(random);TimeShardEffects<3> effects;
        assert(effects.bindPhases(world));const auto h=effects.create({}, {},false,random);
        effects.get(h)->shardLimit=0;world.fail=reject?7:0;
        unsigned outsideCalls=0;
        const bool ok=effects.step(effects.beginPhase(),true,random,
            [](auto,const auto&,auto,auto,bool& hit){hit=false;return true;},
            [&](auto,const auto& s,bool& retire){++outsideCalls;assert(s.destroyRequested&&world.destroys==1);
                retire=true;return true;});
        assert(ok!=reject&&world.destroys==1);
        assert(outsideCalls==(reject?0u:1u)&&world.retires==0);
        if(reject)assert(!effects.succeeded()&&!effects.clear());
        else {
            assert(!effects.get(h)&&effects.sourceId(h)==105186);
            assert(effects.retireRequested()&&world.retires==1&&effects.sourceId(h)==0);
        }
    }
    {
        GameMakerRandom random(42);World world(random);TimeShardEffects<1> effects;
        assert(effects.bindPhases(world));const auto h=effects.create({}, {},false,random);
        effects.get(h)->alarm0=1;effects.advanceAlarm(effects.beginPhase(),0);
        assert(effects.succeeded()&&effects.size()==0&&world.destroys==1&&world.retires==0);
        const auto draws=random.drawCount();
        assert(!effects.create({}, {},false,random)&&random.drawCount()==draws&&world.births==1);
        assert(effects.advanceAnimation(effects.beginPhase())&&world.animations==0);
        effects.advanceAlarm(effects.beginPhase(),2);assert(world.destroys==1&&world.retires==0);
        assert(effects.retireRequested()&&world.retires==1);
        const auto fresh=effects.create({}, {},false,random);assert(fresh>h&&effects.sourceId(fresh)==105187);
        assert(effects.clear());
    }
    for(unsigned failure:{4u,5u,6u,8u,9u,10u}){
        GameMakerRandom random(42);World world(random);TimeShardEffects<3> effects;
        assert(effects.bindPhases(world)&&effects.phasesBoundTo(world));
        auto old=effects.beginPhase();assert(effects.create({}, {},false,random));
        assert(effects.advanceAnimation(old)&&world.animations==0);
        world.fail=failure;
        if(failure==4||failure==10)assert(!effects.advanceAnimation(effects.beginPhase()));
        else if(failure==5||failure==8||failure==9)assert(!effects.step(effects.beginPhase(),true,random,
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
