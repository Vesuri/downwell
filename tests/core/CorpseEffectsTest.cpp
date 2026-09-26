#include "core/CorpseEffects.h"
#include "core/GameMakerRandom.h"
#include <cassert>
#include <cstdio>
#include <vector>
#include <algorithm>
using namespace downwell;
struct Hooks {
    std::vector<unsigned> born,retired;
    bool failBirth=false,failQuery=false,failRetire=false;
    unsigned current=0;
    unsigned spriteWrites=0,facingWrites=0;
    bool failSprite=false,failFacing=false;
    bool sprite(unsigned h,const CorpseAnimation& c){assert(h==current&&c.spriteId==206);++spriteWrites;return !failSprite;}
    bool facing(unsigned h,const CorpseAnimation& c){assert(h==current&&c.imageXscale.bits==0xbf800000u);++facingWrites;return !failFacing;}
    bool birth(unsigned h,const CorpseAnimation& c){born.push_back(h);current=h;assert(c.spriteId==172&&c.imageIndex.bits==0);return !failBirth;}
    Hooks& queries(unsigned h,CorpseAnimation&){assert(h==current&&!born.empty());return *this;}
    bool wall(MotionBinary64,MotionBinary64,bool& hit){assert(current==born.back());hit=false;return !failQuery;}
    bool tile(MotionBinary64,MotionBinary64,bool& hit){hit=false;return true;}
    bool publish(RunnerImageValue,RunnerImageValue){return true;}
    bool retire(unsigned h,const CorpseAnimation&){if(failRetire)return false;retired.push_back(h);return true;}
};
struct Events{unsigned destroyed=0;void destroy(){++destroyed;}};
int main(){
    CorpseEffects<2> owner;Hooks hooks;GameMakerRandom random(42);
    const auto before=owner.beginPhase();const auto a=owner.create({}, {},false,random,hooks);assert(a);
    const auto phase=owner.beginPhase();const auto b=owner.create({}, {},true,random,hooks);assert(b>a);
    const auto draws=random.drawCount();assert(!owner.create({}, {},false,random,hooks)&&random.drawCount()==draws&&owner.valid());
    std::vector<unsigned> order;
    assert(owner.dispatch(before,[&](auto,const auto&){assert(false);return true;},hooks));
    assert(owner.dispatch(phase,[&](auto h,auto& c){order.push_back(h);return c.advanceImage();},hooks));
    assert(order==std::vector<unsigned>{a});
    order.clear();assert(owner.visitNewest([&](auto h,const auto&){order.push_back(h);}));
    assert((order==std::vector<unsigned>{b,a}));
    owner.get(a)->destroyRequested=true;assert(owner.retireRequested(hooks)&&!owner.get(a));
    const auto stale=owner.beginPhase();const auto c=owner.create({}, {},false,random,hooks);assert(c>b);
    order.clear();assert(owner.dispatch(stale,[&](auto h,auto&){order.push_back(h);return true;},hooks));
    assert(order==std::vector<unsigned>{b});
    Events events;
    assert(owner.dispatch(owner.beginPhase(),[&](auto,auto& corpse){return corpse.endStep(true,MotionBinary64::fromInteger(1000),events);},hooks));
    assert(events.destroyed==2&&!owner.get(b)&&!owner.get(c));
    for(unsigned i=0;i<256;++i){
        auto old=owner.beginPhase();auto h=owner.create({}, {},false,random,hooks);assert(h>old.newest);
        assert(owner.clear(hooks)&&!owner.get(h));
    }
    for(unsigned failure=0;failure<3;++failure){
        hooks.failBirth=failure==0;hooks.failQuery=failure==1;
        const auto h=owner.create({}, {},false,random,hooks);
        if(failure<2)assert(!h&&!owner.valid());
        else assert(!owner.dispatch(owner.beginPhase(),[](auto,auto&){return false;},hooks)&&!owner.valid());
        assert(!owner.dispatch(owner.beginPhase(),[](auto,auto&){assert(false);return true;},hooks));
        hooks.failBirth=hooks.failQuery=false;hooks.failRetire=true;
        assert(!owner.clear(hooks)&&!owner.valid());hooks.failRetire=false;
        assert(owner.clear(hooks)&&owner.valid());
    }
    std::sort(hooks.retired.begin(),hooks.retired.end());
    assert(hooks.born==hooks.retired); // Every published lifetime retired once.
    for(unsigned failure=0;failure<4;++failure){
        CorpseEffects<1> owned;Hooks adapter;GameMakerRandom rng(17);
        DeadBodySource source;source.imageXscale=MotionBinary64::fromInteger(-1);
        adapter.failSprite=failure==1;adapter.failFacing=failure==2;
        if(failure==3)assert(owned.create({}, {},false,rng,adapter));
        const auto before=rng.drawCount();
        assert(owned.createFromDeath(source,206,MotionBinary64::fromInteger(9),102,true,rng,adapter)==(failure==0));
        assert(rng.drawCount()==before+(failure==3?0:3));
        if(!failure){
            const auto* corpse=owned.get(source.myDeadBody);assert(corpse);
            assert(corpse->ysp.high==0x40100000u&&corpse->xsp.high==0xc0180000u);
            assert(source.xsp.high==0xbff80000u&&corpse->health==1&&corpse->hitEffect==1);
            assert(adapter.spriteWrites==1&&adapter.facingWrites==1);
        }else{
            assert(!owned.valid()&&!owned.get(source.myDeadBody));
            assert(!owned.createFromDeath(source,206,{},102,false,rng,adapter));
            assert(rng.drawCount()==before+(failure==3?0:3));
        }
        assert(owned.clear(adapter));assert(adapter.born==adapter.retired);
    }
    {
        CorpseEffects<1> owned;Hooks adapter;GameMakerRandom rng(17);DeadBodySource source;
        source.imageXscale=MotionBinary64::fromInteger(1);
        assert(owned.createFromDeath(source,172,{},102,false,rng,adapter));
        assert(!adapter.spriteWrites&&!adapter.facingWrites); // Same resource and scale are not dirty writes.
        assert(owned.clear(adapter));
    }
    std::puts("Corpse owner: death-wrapper integration, pre-Create publication, capacity, phase cutoffs, reuse, culling and failure cleanup passed");
}
