#include "core/BoneProjectileStore.h"
#include "core/SkeletonActor.h"
#include "core/GameMakerRandom.h"
#include <cstdlib>
#include <cstdio>
static void check(bool ok) { if (!ok) std::abort(); }
using Store=downwell::BoneProjectileStore<2>;
struct Sound {
    Store& store;
    downwell::GameMakerRandom& random;
    bool& shot;
    unsigned calls=0;
    void operator()(int n) {
        check(n==134 && !shot && random.drawCount()==3);
        // Callback observes a real, completely created child BEFORE overrides.
        check(store.slot(0).handle && store.slot(0).projectile.active);
        ++calls;
    }
};
struct Events {
    unsigned destroys=0,effects=0;
    void destroy() { ++destroys; }
    void effect(int id,int rate) { check(id==85 && rate==3); ++effects; }
};
int main() {
    for (unsigned seed=0;seed<1024;++seed) for (bool separated:{false,true}) {
        Store store; downwell::GameMakerRandom random(seed),oracle(seed);
        downwell::SkeletonActor actor; actor.create(100,200);
        Sound sound{store,random,actor.shot};
        const auto stepPhase=store.beginPhase();
        downwell::SkeletonBoneCreation<Store,downwell::GameMakerRandom,Sound>
            creation{store,random,sound,actor.shot};
        check(creation.run(actor.x,actor.y,separated,-1));
        check(actor.shot && sound.calls==1 && creation.child);
        downwell::BoneProjectile expected;
        check(expected.create(actor.x,actor.y-10,oracle));
        const int vy=-30-int(oracle.chooseIndexSmall(3));
        const int choices[]={10,8,12};
        const int vx=separated?-choices[oracle.chooseIndexSmall(3)]:10;
        check(expected.motion.launch(actor.x,actor.y-10,vx,vy));
        if(vx>0) expected.sampledAnimationRate.high^=0x80000000u;
        auto* bone=store.get(creation.child);
        unsigned steps=0;
        store.visitPhase(stepPhase,[&](Store::Handle,downwell::BoneProjectile&){++steps;});
        check(steps==0); // Synchronous Create does not imply same-phase Step.
        store.visitPhase(store.beginPhase(),[&](Store::Handle h,downwell::BoneProjectile& p){
            check(h==creation.child && &p==bone); ++steps;
        });
        check(steps==1); // A later phase is independently eligible.
        check(bone && bone->motion.xsp.high==expected.motion.xsp.high &&
              bone->motion.xsp.low==expected.motion.xsp.low &&
              bone->motion.ysp.high==expected.motion.ysp.high &&
              bone->motion.ysp.low==expected.motion.ysp.low &&
              bone->sampledAnimationRate.high==expected.sampledAnimationRate.high &&
              bone->sampledAnimationRate.low==expected.sampledAnimationRate.low);
        for(unsigned i=0;i<16;++i) check(random.stateWord(i)==oracle.stateWord(i));
        check(random.drawCount()==oracle.drawCount());
        const auto second=store.create(1,2,random); check(second);
        const auto before=random.drawCount();
        check(!store.create(3,4,random) && random.drawCount()==before);
        bool failedShot=false;
        auto unexpectedSound=[](int) { check(false); };
        downwell::SkeletonBoneCreation<Store,downwell::GameMakerRandom,decltype(unexpectedSound)>
            failed{store,random,unexpectedSound,failedShot};
        check(!failed.run(100,200,true,1) && !failedShot && !failed.child && random.drawCount()==before);
        check(!store.retire(creation.child));
        Events events; bone->collideWall(events);
        check(events.destroys==1 && events.effects==1 && store.get(creation.child)==bone);
        // A destroy request does not make capacity available during the event.
        check(!store.create(3,4,random) && random.drawCount()==before);
        check(store.retire(creation.child) && !store.get(creation.child));
        const auto replacement=store.create(3,4,random);
        check(replacement && replacement!=creation.child && store.get(second));
        check(!store.retire(creation.child));
    }
    // Reusing a later slot during a dispatch must not accidentally Step the
    // newly created projectile just because traversal has not reached it yet.
    Store store; downwell::GameMakerRandom random(5);
    auto first=store.create(1,2,random),second=store.create(3,4,random);
    unsigned visited=0; Store::Handle replacement=0;
    store.visitPhase(store.beginPhase(),[&](Store::Handle h,downwell::BoneProjectile&){
        ++visited; check(h==first);
        store.get(second)->destroyRequested=true; check(store.retire(second));
        replacement=store.create(5,6,random); check(replacement);
    });
    check(visited==1);
    visited=0;
    store.visitPhase(store.beginPhase(),[&](Store::Handle,downwell::BoneProjectile&){++visited;});
    check(visited==2);
    std::puts("Owned skeleton bone creation: 2048 RNG/velocity/deferred-retirement cases passed");
}
