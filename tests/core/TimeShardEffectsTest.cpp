#include "core/TimeShardEffects.h"
#include "core/GameMakerRandom.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace downwell;
int main(){
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
    owner.clear();
    // Repeated slot reuse must never revalidate a stale handle/cutoff.
    for(unsigned run=0;run<1024;++run){
        const auto old=owner.beginPhase();const auto h=owner.create({}, {},false,rng);assert(h>old.newest);
        auto* s=owner.get(h);s->alarm0=2;
        owner.advanceAlarm(old,0);assert(s->alarm0==2);
        const auto current=owner.beginPhase();
        assert(owner.advanceAnimation(current));owner.advanceAlarm(current,0);
        assert(owner.step(current,true,rng,noWall,onScreen));assert(s->alarm0==2);
        owner.advanceAlarm(current,0);owner.advanceAlarm(current,0);assert(!owner.get(h));
    }
    std::puts("Shard ownership: phase cutoffs, live population, alarm retirement, contact RNG, failure and 1024 reuse cycles passed");
}
