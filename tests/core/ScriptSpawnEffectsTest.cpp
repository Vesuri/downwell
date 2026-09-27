#include "core/ScriptSpawnEffect.h"
#include "core/EffectSpawnScript.h"
#include <cassert>
#include <cstdio>
using namespace downwell;
struct Hooks {
    unsigned births=0,sprites=0,retires=0;bool reject=false;
    unsigned images=0,rejectImage=0;
    unsigned lastImage=0,lastImageHandle=0;
    unsigned published[16]{};bool hasPublished[16]{};
    unsigned marks=0,rejectMark=0;
    bool destroyed(unsigned h,const ScriptSpawnEffect& e){
        assert(e.destroyRequested&&lastImageHandle==h&&lastImage==e.imageIndex.bits);return ++marks!=rejectMark;
    }
    bool image(unsigned h,const ScriptSpawnEffect& e){
        assert(!e.destroyRequested&&h<16);lastImage=e.imageIndex.bits;lastImageHandle=h;
        published[h]=e.imageIndex.bits;hasPublished[h]=true;
        return ++images!=rejectImage;
    }
    bool birth(unsigned h,const ScriptSpawnEffect& e){
        assert(h&&e.spriteId==-1&&!e.frames&&e.imageSpeed.bits==0x3f800000u&&
            !e.angle.bits&&!e.depth.bits&&!e.destroyRequested);++births;return true;
    }
    bool sprite(unsigned,const ScriptSpawnEffect& e){assert(e.frames==1);++sprites;return true;}
    bool retire(unsigned h,const ScriptSpawnEffect& e){
        if(e.destroyRequested)assert(h<16&&hasPublished[h]&&published[h]==e.imageIndex.bits);
        ++retires;return !reject;
    }
};
struct Ops {
    using Handle=unsigned;
    ScriptSpawnEffects<2>& owner;Hooks& hooks;unsigned callerMyFx=0;
    bool create(int object,MotionBinary64 x,MotionBinary64 y,Handle& h){
        assert(object==219);h=owner.create(x,y,hooks);return h!=0;
    }
    bool myFx(Handle h){callerMyFx=h;return true;}
    bool sprite(Handle h,int id){return owner.setSprite(h,id,1,hooks);}
    bool imageSpeed(Handle h,MotionBinary64 n){return owner.mutate(h,[&](auto& e){return e.setValue(e.imageSpeed,n,[](const auto&){return true;});});}
    bool imageAngle(Handle h,MotionBinary64 n){return owner.mutate(h,[&](auto& e){return e.setValue(e.angle,n,[](const auto&){return true;});});}
    bool depth(Handle h,MotionBinary64 n){return owner.mutate(h,[&](auto& e){return e.setValue(e.depth,n,[](const auto&){return true;});});}
    bool spawn(int id){return EffectSpawnScript::run(*this,{0x406e1000u,0},{0x40bfff40u,0},
        id,{0x3fe00000u,0},MotionBinary64::fromInteger(270),{});}
};
int main(){
    {
        ScriptSpawnEffects<2> effects;Hooks hooks;Ops ops{effects,hooks};
        assert(ops.spawn(603));const auto first=ops.callerMyFx;
        assert(ops.spawn(602));const auto second=ops.callerMyFx;
        const auto phase=effects.beginPhase();
        assert(effects.advanceRegistered(phase,hooks)&&effects.advanceRegistered(phase,hooks));
        assert(hooks.marks==2&&hooks.retires==0&&hooks.images==4&&effects.size()==2&&!effects.get(first)&&!effects.get(second));
        assert(effects.advanceRegistered(phase,hooks)&&hooks.images==4&&hooks.marks==2);
        // Cleanup may traverse in a different order from the newest-first store.
        assert(effects.retireMarked(first,hooks)&&effects.size()==1);
        assert(effects.retireMarked(second,hooks)&&effects.size()==0&&hooks.retires==2);
    }
    for(unsigned failure=0;failure<2;++failure){
        const bool rejectMark=failure!=0;
        ScriptSpawnEffects<2> effects;Hooks hooks;Ops ops{effects,hooks};
        assert(ops.spawn(603));const auto h=ops.callerMyFx;
        assert(effects.advanceRegistered(effects.beginPhase(),hooks));
        hooks.rejectMark=rejectMark?1:0;
        assert(effects.advanceRegistered(effects.beginPhase(),hooks)!=rejectMark);
        if(!rejectMark){hooks.reject=true;assert(!effects.retireMarked(h,hooks));}
        assert(!effects.valid()&&effects.size()==1&&hooks.marks==1&&hooks.images==2);
        const auto retires=hooks.retires;
        assert(!effects.advanceRegistered(effects.beginPhase(),hooks)&&!effects.retireMarked(h,hooks));
        assert(hooks.marks==1&&hooks.images==2&&hooks.retires==retires);
        hooks.reject=false;assert(effects.clear(hooks));
    }
    for(unsigned failure=1;failure<=2;++failure){
        ScriptSpawnEffects<2> effects;Hooks hooks;Ops ops{effects,hooks};
        assert(ops.spawn(603));hooks.rejectImage=failure;
        const auto* retained=effects.get(ops.callerMyFx);
        if(failure==2)assert(effects.advanceAnimation(effects.beginPhase(),hooks));
        assert(!effects.advanceAnimation(effects.beginPhase(),hooks)&&!effects.valid()&&
            hooks.images==failure&&hooks.retires==0);
        assert(retained&&!retained->destroyRequested&&!retained->valid&&
            retained->imageIndex.bits==(failure==1?0x3f000000u:0u));
        assert(!effects.advanceAnimation(effects.beginPhase(),hooks)&&hooks.images==failure);
        assert(effects.clear(hooks)&&hooks.retires==1);
    }
    {
        ScriptSpawnEffects<2> bound;Hooks first,other;
        const auto h=bound.create({},{},first);assert(h);
        assert(!bound.setSprite(h,603,1,other)&&!bound.valid()&&other.sprites==0);
        assert(bound.clear(first)&&first.retires==1&&bound.valid());
        assert(!bound.create({},{},other)&&other.births==0&&!bound.valid());
    }
    ScriptSpawnEffects<2> owner;Hooks hooks;Ops ops{owner,hooks};
    const auto beforeBirth=owner.beginPhase();assert(ops.spawn(603));const auto first=ops.callerMyFx;
    const auto* stable=owner.get(first);assert(stable&&stable->angle.bits==0x43870000u);
    assert(owner.advanceAnimation(beforeBirth,hooks)&&owner.get(first)->imageIndex.bits==0);
    const auto firstPhase=owner.beginPhase();assert(ops.spawn(602));const auto second=ops.callerMyFx;
    assert(first!=second&&owner.get(first)==stable);
    assert(owner.advanceAnimation(firstPhase,hooks)&&owner.get(first)->imageIndex.bits==0x3f000000u&&
        owner.get(second)->imageIndex.bits==0);
    assert(owner.advanceAnimation(owner.beginPhase(),hooks)&&!owner.get(first)&&owner.get(second)&&hooks.retires==1);
    assert(ops.spawn(603));const auto third=ops.callerMyFx;assert(third>second&&!owner.get(first));
    unsigned order[2]{},count=0;
    assert(owner.visitNewest([&](unsigned h,const auto&){order[count++]=h;}));
    assert(count==2&&order[0]==third&&order[1]==second);
    assert(owner.clear(hooks)&&hooks.births==3&&hooks.sprites==3&&hooks.retires==3);
    assert(ops.callerMyFx==third&&!owner.get(third)); // Caller retains the source ID after death.
    assert(ops.spawn(603)&&ops.callerMyFx>third);
    const auto newest=ops.callerMyFx;
    assert(!owner.mutate(newest,[](auto& e){return e.setValue(e.angle,MotionBinary64::fromInteger(12),
        [](const auto& e){assert(e.angle.bits==0x41400000u);return false;});})&&!owner.valid());
    assert(!owner.advanceAnimation(owner.beginPhase(),hooks));
    assert(owner.clear(hooks)&&owner.valid());
    assert(ops.spawn(602)&&ops.spawn(603));const auto idBefore=ops.callerMyFx;
    assert(!ops.spawn(603)&&!owner.valid()&&ops.callerMyFx==idBefore);
    hooks.reject=true;assert(!owner.clear(hooks)&&!owner.valid());
    std::puts("Script effects: synchronous publication, phase cutoffs, stable keys, reuse, ordering, caller ID retention and terminal failures passed");
}
