#include "core/CorpseExpiryEffects.h"
#include "core/CorpseLifecycle.h"
#include "core/RunSession.h"
#include <cassert>
#include <cstdio>
using namespace downwell;
struct Hooks {
    unsigned born=0,retired=0,sprites=0,failBirth=0,failRetire=0;
    bool birth(unsigned,const CorpseExpiryEffect& effect){
        assert(effect.imageSpeed.bits==0x3f800000u&&effect.spriteId==102);
        return ++born!=failBirth;
    }
    bool retire(unsigned,const CorpseExpiryEffect&){return ++retired!=failRetire;}
    bool sprite(unsigned,const CorpseExpiryEffect&){++sprites;return true;}
};
int main(){
    CorpseExpiryEffects<2> owner;Hooks hooks;
    auto old=owner.beginPhase();auto first=owner.create(RunnerImageValue{},RunnerImageValue{},hooks);
    assert(first&&owner.get(first)->imageSpeed.bits==0x3e99999au);
    assert(owner.advanceAnimation(old,hooks)&&owner.get(first)->timeline.bits==0);
    auto phase=owner.beginPhase();auto second=owner.create(RunnerImageValue{},RunnerImageValue{},hooks);
    assert(second>first&&owner.advanceAnimation(phase,hooks));
    assert(owner.get(first)->timeline.bits!=0&&owner.get(second)->timeline.bits==0);
    unsigned last=second;
    assert(owner.visitNewest([&](unsigned h,const auto&){assert(h==last--);}));
    for(unsigned i=1;i<40;++i)assert(owner.advanceAnimation(owner.beginPhase(),hooks));
    assert(!owner.get(first)&&owner.get(second)&&hooks.retired==1);
    assert(owner.advanceAnimation(owner.beginPhase(),hooks)&&!owner.size()&&hooks.retired==2);
    for(unsigned i=0;i<256;++i){
        auto h=owner.create(RunnerImageValue{},RunnerImageValue{},hooks);assert(h>second);second=h;
        assert(owner.clear(hooks)&&!owner.get(h));
        auto replacement=owner.create(RunnerImageValue{},RunnerImageValue{},hooks);assert(replacement>h);
        assert(owner.advanceAnimation(old,hooks)&&owner.get(replacement)->timeline.bits==0);
        assert(owner.clear(hooks));
    }
    auto a=owner.create(RunnerImageValue{},RunnerImageValue{},hooks),b=owner.create(RunnerImageValue{},RunnerImageValue{},hooks);assert(a&&b);
    assert(!owner.create(RunnerImageValue{},RunnerImageValue{},hooks)&&!owner.valid());
    assert(owner.clear(hooks)&&owner.valid());
    hooks.failBirth=hooks.born+1;
    assert(!owner.create(RunnerImageValue{},RunnerImageValue{},hooks)&&!owner.valid());
    auto retired=hooks.retired;
    assert(owner.clear(hooks)&&hooks.retired==retired+1);
    a=owner.create(RunnerImageValue{},RunnerImageValue{},hooks);b=owner.create(RunnerImageValue{},RunnerImageValue{},hooks);assert(a&&b);
    retired=hooks.retired;hooks.failRetire=retired+2;
    assert(!owner.clear(hooks)&&hooks.retired==retired+2);
    hooks.failRetire=0;
    assert(owner.clear(hooks)&&hooks.retired==retired+3); // Successful first removal is not repeated.
    a=owner.create(RunnerImageValue{},RunnerImageValue{},hooks);b=owner.create(RunnerImageValue{},RunnerImageValue{},hooks);assert(a&&b);
    for(unsigned i=0;i<39;++i)assert(owner.advanceAnimation(owner.beginPhase(),hooks));
    retired=hooks.retired;hooks.failRetire=retired+2;
    assert(!owner.advanceAnimation(owner.beginPhase(),hooks)&&!owner.valid());
    assert(hooks.retired==retired+2);
    hooks.failRetire=0;assert(owner.clear(hooks)&&hooks.retired==retired+3);
    a=owner.create(RunnerImageValue{},RunnerImageValue{},hooks);assert(a);
    assert(!owner.setSprite(a,159,0,hooks)&&!owner.valid());
    assert(owner.clear(hooks));
    struct Events {
        CorpseExpiryEffects<2>& owner;Hooks& hooks;unsigned deaths=0;
        bool create(int id,RunnerImageValue x,RunnerImageValue y,unsigned& child){
            assert(id==242);child=owner.create(x,y,hooks);return child!=0;
        }
        bool sprite(unsigned h,int id){return owner.setSprite(h,id,12,hooks);}
        void destroy(){++deaths;}
    } events{owner,hooks};
    CorpseLifecycle corpse;corpse.explosionFx=102;corpse.alarms.set(0,1);
    phase=owner.beginPhase();assert(corpse.advanceAlarm(0,events)&&events.deaths==1);
    assert(owner.get(corpse.myExplosion)&&owner.advanceAnimation(phase,hooks));
    assert(owner.get(corpse.myExplosion)->timeline.bits==0);
    for(unsigned i=0;i<40;++i)assert(owner.advanceAnimation(owner.beginPhase(),hooks));
    assert(!owner.size()&&corpse.destroyRequested);
    RunSession session;
    assert(session.emitCorpseExpiry({},{}));
    for(unsigned i=0;i<39;++i)session.advanceCorpseExpiryAnimation();
    assert(session.corpseExpiryEffects.size()==1&&session.effectsSucceeded);
    session.advanceCorpseExpiryAnimation();assert(!session.corpseExpiryEffects.size());
    for(unsigned i=0;i<32;++i)assert(session.emitCorpseExpiry({},{}));
    assert(!session.emitCorpseExpiry({}, {})&&!session.effectsSucceeded);
    session.reset();assert(session.effectsSucceeded&&!session.corpseExpiryEffects.size());
    assert(session.emitCorpseExpiry({},{}));session.clearCorpseExpiry();
    assert(!session.corpseExpiryEffects.size());
    CorpseLifecycle linked;linked.x={0x42c00000u};linked.y={0x43400000u};linked.explosionFx=102;
    linked.alarms.set(0,2);unsigned destroyed=0,metadataReads=0;
    auto frames=[&](int id){
        assert(id==102&&!linked.destroyRequested&&linked.myExplosion);
        assert(session.corpseExpiryEffects.get(linked.myExplosion));++metadataReads;return 12u;
    };
    auto destroy=[&](){assert(linked.destroyRequested&&metadataReads==1);++destroyed;};
    assert(session.advanceCorpseAlarm(linked,0,frames,destroy));
    assert(!session.corpseExpiryEffects.size()&&!destroyed&&!metadataReads);
    assert(session.advanceCorpseAlarm(linked,0,frames,destroy));
    assert(destroyed==1&&session.corpseExpiryEffects.size()==1);
    const auto* effect=session.corpseExpiryEffects.get(linked.myExplosion);
    assert(effect&&effect->x.bits==linked.x.bits&&effect->y.bits==linked.y.bits);
    assert(session.advanceCorpseAlarm(linked,0,frames,destroy)&&destroyed==1);
    for(unsigned i=0;i<40;++i)session.advanceCorpseExpiryAnimation();
    assert(!session.corpseExpiryEffects.size());
    // Child capacity failure must not retire the parent or consume the event twice.
    session.reset();for(unsigned i=0;i<32;++i)assert(session.emitCorpseExpiry({},{}));
    linked={};linked.alarms.set(0,1);
    assert(!session.advanceCorpseAlarm(linked,0,frames,destroy));
    assert(!linked.valid&&!linked.destroyRequested&&!linked.myExplosion&&destroyed==1);
    assert(!session.advanceCorpseAlarm(linked,0,frames,destroy)&&destroyed==1);
    // Metadata/publication failure retains the already-created child for cleanup.
    session.reset();linked={};linked.alarms.set(0,1);
    assert(!session.advanceCorpseAlarm(linked,0,[](int){return 0u;},destroy));
    assert(!linked.valid&&!linked.destroyRequested&&linked.myExplosion&&session.corpseExpiryEffects.size()==1);
    session.reset();assert(session.effectsSucceeded&&!session.corpseExpiryEffects.size());
    std::puts("Corpse expiry ownership: connected alarm, phase cutoffs, stable reuse, capacity and partial cleanup passed");
}
