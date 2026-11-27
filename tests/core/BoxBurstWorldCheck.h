#ifndef DOWNWELL_TEST_BOX_BURST_WORLD_CHECK_H
#define DOWNWELL_TEST_BOX_BURST_WORLD_CHECK_H
#include <cassert>
#include <type_traits>
#include "core/SmokeEffects.h"
#include "core/FlashBallEffects.h"

// Integration check, not an independent script oracle: the burst primitives
// have separate VM/scalar tests. Here verify world RNG ownership, coordinates,
// retained children and terminal failure across the two effect banks.
template<class World> void checkBoxBurstWorld(World& world,bool failSmoke){
    using namespace downwell;
    // The world's smoke pool is larger than its flash pool (W1.9 smoke).
    typename std::decay<decltype(world.smokeEffects())>::type smoke;FlashBallEffects<32> flash;
    constexpr unsigned smokeSlots=decltype(smoke)::capacity;
    static_assert(smokeSlots>=32,"the burst loop fills the flash pool first");
    auto random=world.randomState();
    auto sameRandom=[&](){
        assert(world.randomState().drawCount()==random.drawCount());
        assert(world.randomState().stateIndex()==random.stateIndex());
        for(unsigned i=0;i<16;++i)assert(world.randomState().stateWord(i)==random.stateWord(i));
    };
    auto sameChildren=[&](){
        assert(world.smokeEffects().size()==smoke.size());
        for(unsigned i=0;i<smokeSlots;++i){
            const auto& a=world.smokeEffects().slot(i);const auto& b=smoke.slot(i);
            assert(a.handle==b.handle);
            if(a.handle){
                assert(a.effect.xx==b.effect.xx&&a.effect.yy==b.effect.yy);
                assert(a.effect.speed==b.effect.speed&&a.effect.direction==b.effect.direction);
                assert(a.effect.rate16==b.effect.rate16);
                assert(a.effect.frame16==b.effect.frame16);
            }
            if(i>=32)continue;
            const auto& c=world.flashEffects().slot(i);const auto& d=flash.slot(i);
            assert(c.handle==d.handle);
            if(c.handle){
                assert(c.effect.x.bits==d.effect.x.bits&&c.effect.y.bits==d.effect.y.bits);
                assert(c.effect.imageIndex.bits==d.effect.imageIndex.bits);
            }
        }
    };
    for(unsigned i=0;i<smokeSlots;++i){
        const auto x=Fixed16::fromInteger(240+i),y=Fixed16::fromInteger(304+i);
        assert(smoke.boxBurst(x,y,random));assert(world.emitBoxSmoke(x,y));
        sameRandom();sameChildren();
        if(i>=32)continue;
        const auto baseY=Fixed16::fromInteger(300+i);
        assert(flash.boxBurst(x,baseY,random));assert(world.emitBoxFlash(x,baseY));
        sameRandom();sameChildren();
    }
    const auto x=Fixed16::fromInteger(240),y=Fixed16::fromInteger(304);
    if(failSmoke){assert(!smoke.boxBurst(x,y,random));assert(!world.emitBoxSmoke(x,y));}
    else {assert(!flash.boxBurst(x,y,random));assert(!world.emitBoxFlash(x,y));}
    assert(!world.effectsSucceeded());sameRandom();sameChildren();
    assert(!world.emitBoxSmoke(x,y));assert(!world.emitBoxFlash(x,y));
    sameRandom();sameChildren();
}
#endif
