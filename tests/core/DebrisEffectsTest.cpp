#include "core/DebrisEffects.h"
#include "core/GameMakerRandom.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace downwell;
struct Hooks {
    std::vector<unsigned>& births;
    std::vector<unsigned>& deaths;
    bool rejectBirth=false,rejectRetire=false;
    bool birth(unsigned h,const DebrisAnimation& e){assert(e.alarms.get(0)==30);births.push_back(h);return !rejectBirth;}
    bool retire(unsigned h,const DebrisAnimation&){if(rejectRetire)return false;deaths.push_back(h);return true;}
};
int main(){
    DebrisEffects<3> owner;GameMakerRandom random(42);
    std::vector<unsigned> births,deaths,queries;Hooks hooks{births,deaths};
    const auto empty=owner.beginPhase();
    const auto first=owner.create({}, {},1,random,hooks);assert(first);
    const auto phase=owner.beginPhase();
    const auto second=owner.create({}, {},2,random,hooks);assert(second>first);
    const auto initial=owner.get(first)->imageIndex.bits;
    assert(owner.advanceAnimation(empty)&&owner.get(first)->imageIndex.bits==initial);
    assert(owner.advanceAlarm(phase,2,hooks));
    assert(owner.get(first)->alarms.get(2)==6&&owner.get(second)->alarms.get(2)==7);
    auto stopped=[&](auto h,const auto&,bool& freeze){freeze=h==first;return true;};
    auto noWall=[&](auto h,const auto&,auto,auto,bool& hit){queries.push_back(h);hit=false;return true;};
    assert(owner.step(phase,random,stopped,noWall,hooks)&&queries.empty());
    assert(owner.get(first)->alarms.get(2)==7); // Frozen alarm refund only for first.
    assert(owner.step(owner.beginPhase(),random,stopped,noWall,hooks)&&queries.size()==3);
    for(auto h:queries)assert(h==second);
    const auto third=owner.create({}, {},4,random,hooks);assert(third>second);
    const auto draws=random.drawCount();
    assert(!owner.create({}, {},1,random,hooks)&&owner.valid()&&random.drawCount()==draws);
    std::vector<unsigned> order;
    assert(owner.visitNewest([&](auto h,const auto&){order.push_back(h);}));
    assert((order==std::vector<unsigned>{third,second,first}));
    owner.get(first)->alarms.set(0,1);
    const auto old=owner.beginPhase();
    assert(owner.advanceAlarm(old,0,hooks)&&!owner.get(first)&&owner.size()==2);
    assert((deaths==std::vector<unsigned>{first}));
    const auto replacement=owner.create({}, {},1,random,hooks);assert(replacement>third);
    assert(owner.advanceAlarm(old,0,hooks)&&owner.get(replacement)->alarms.get(0)==30);
    assert(owner.clear(hooks)&&owner.size()==0&&!owner.get(second));
    assert((deaths==std::vector<unsigned>{first,replacement,third,second}));
    assert((births==std::vector<unsigned>{first,second,third,replacement}));
    for(unsigned cycle=0;cycle<1024;++cycle){
        const auto stale=owner.beginPhase();const auto h=owner.create({}, {},1,random);assert(h>stale.newest);
        owner.get(h)->alarms.set(0,1);
        assert(owner.advanceAlarm(stale,0)&&owner.get(h));
        assert(owner.advanceAnimation(owner.beginPhase()));
        assert(owner.advanceAlarm(owner.beginPhase(),0)&&!owner.get(h)&&!owner.size());
    }
    const auto h=owner.create({}, {},1,random);assert(h);
    unsigned calls=0;
    auto allowed=[](auto,const auto&,bool& freeze){freeze=false;return true;};
    assert(!owner.step(owner.beginPhase(),random,allowed,[&](auto,const auto&,auto,auto,bool&){++calls;return false;}));
    assert(calls==1&&!owner.valid()&&!owner.get(h));
    assert(!owner.advanceAnimation(owner.beginPhase())&&!owner.advanceAlarm(owner.beginPhase(),0));
    assert(!owner.step(owner.beginPhase(),random,allowed,noWall));
    assert(owner.clear()&&owner.valid());
    Hooks reject{births,deaths,true,false};
    assert(!owner.create({}, {},1,random,reject)&&!owner.valid());
    assert(owner.clear(hooks)&&owner.valid());
    const auto dying=owner.create({}, {},1,random);assert(dying);
    owner.get(dying)->destroyRequested=true;reject.rejectBirth=false;reject.rejectRetire=true;
    assert(!owner.retireRequested(reject)&&!owner.valid());
    assert(owner.clear(hooks)&&deaths.back()==dying);
    const auto blocked=owner.create({}, {},1,random);
    const auto earlier=owner.create({}, {},1,random);
    owner.get(blocked)->destroyRequested=true;owner.get(earlier)->destroyRequested=true;
    struct PartialRetirement {
        unsigned blocked;std::vector<unsigned>& deaths;
        bool retire(unsigned h,const DebrisAnimation&){if(h==blocked)return false;deaths.push_back(h);return true;}
    } partial{blocked,deaths};
    const auto before=deaths.size();
    assert(!owner.retireRequested(partial)&&!owner.valid()&&deaths.size()==before+1&&deaths.back()==earlier);
    assert(owner.clear(hooks)&&deaths.size()==before+2&&deaths.back()==blocked);
    // Cleanup must not republish an earlier successful retirement after failure.
    assert(owner.create({}, {},1,random));queries.clear();
    assert(!owner.step(owner.beginPhase(),random,[](auto,const auto&,bool&){return false;},noWall));
    assert(!owner.valid()&&queries.empty());
    const auto failedDraws=random.drawCount();
    assert(!owner.create({}, {},1,random)&&random.drawCount()==failedDraws);
    assert(owner.clear());
    std::puts("Debris ownership: phase cutoffs, per-instance freeze, capacity/RNG, callbacks, latched failures and 1024 reuse cycles passed");
}
