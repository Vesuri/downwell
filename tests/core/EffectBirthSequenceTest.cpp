#include "core/BloodEffects.h"
#include "core/SmokeEffects.h"
#include "core/FlashBallEffects.h"
#include "core/DebrisEffects.h"
#include "core/GameMakerRandom.h"
#include <cassert>
#include <cstdio>
using namespace downwell;

template<class Owner,class Create> void check(Create create){
    Owner original;
    assert(create(original)==1);
    const auto sequence=original.birthSequence();
    assert(!original.restoreEmptyBirthSequence(sequence)); // Occupied owner.
    original.clear();
    assert(!original.restoreEmptyBirthSequence({1})); // No rewind after clear.
    assert(original.restoreEmptyBirthSequence(sequence));
    assert(create(original)==2);
    Owner reconstructed;
    assert(reconstructed.restoreEmptyBirthSequence(original.birthSequence()));
    assert(create(reconstructed)==3);
    reconstructed.clear();
    assert(reconstructed.restoreEmptyBirthSequence({0xffffffffu}));
    assert(create(reconstructed)==0xffffffffu);
    reconstructed.clear();
    assert(!create(reconstructed)); // Exhaustion survives clear.
    assert(!reconstructed.restoreEmptyBirthSequence({1}));
    Owner exhausted;
    assert(exhausted.restoreEmptyBirthSequence(reconstructed.birthSequence()));
    assert(!create(exhausted)); // Exhaustion survives reconstruction too.
}
int main(){
    GameMakerRandom random(84);
    check<BloodEffects<2>>([](auto& owner){return owner.create({}, {},{});});
    const auto randomizedCreate=[&](auto& owner){
        const auto before=random.drawCount();
        const auto handle=owner.create({}, {},random);
        if(!handle)assert(random.drawCount()==before);
        return handle;
    };
    check<SmokeEffects<2>>(randomizedCreate);
    check<FlashBallEffects<2>>(randomizedCreate);
    check<DebrisEffects<2>>([&](auto& owner){
        const auto before=random.drawCount();
        const auto handle=owner.create({}, {},1,random);
        if(!handle)assert(random.drawCount()==before);
        return handle;
    });
    {
        DebrisEffects<2> original;
        const auto old=original.create({}, {},1,random);
        const auto phase=original.beginPhase();const auto sequence=original.birthSequence();
        assert(original.clear());
        DebrisEffects<2> rebuilt;
        assert(rebuilt.restoreEmptyBirthSequence(sequence));
        const auto born=rebuilt.create({}, {},1,random);
        assert(born>old&&!rebuilt.get(old));
        const auto before=*rebuilt.get(born);const auto draws=random.drawCount();
        assert(rebuilt.advanceAnimation(phase)&&rebuilt.advanceAlarm(phase,0));
        assert(rebuilt.step(phase,random,
            [](auto,const auto&,bool&){assert(false);return false;},
            [](auto,const auto&,auto,auto,bool&){assert(false);return false;}));
        assert(rebuilt.get(born)->timeline.bits==before.timeline.bits);
        assert(rebuilt.get(born)->alarms.get(0)==before.alarms.get(0));
        assert(random.drawCount()==draws);
    }
    {
        BloodEffects<2> owner;
        const auto retired=owner.create({}, {},{}),phase=owner.beginPhase().newest;
        owner.clear();const auto born=owner.create({}, {},{});
        assert(!owner.get(retired)&&owner.get(born));
        assert(owner.advanceAnimation({phase}));
        assert(owner.get(born)->timeline.bits==0);
    }
    {
        SmokeEffects<2> owner;
        const auto retired=owner.create({}, {},random),phase=owner.beginPhase().newest;
        owner.clear();const auto born=owner.create({}, {},random);
        assert(!owner.get(retired)&&owner.get(born));
        const auto draws=random.drawCount();
        assert(owner.advanceAnimation({phase})&&owner.step({phase},random));
        assert(!owner.get(born)->allSet&&owner.get(born)->timeline.bits==0);
        assert(random.drawCount()==draws);
    }
    {
        FlashBallEffects<2> owner;
        const auto retired=owner.create({}, {},random),phase=owner.beginAnimation().newest;
        owner.clear();const auto born=owner.create({}, {},random);
        assert(born>retired&&owner.advanceAnimation({phase}));
        assert(owner.slot(0).effect.timeline.bits==0x3f800000u);
    }
    std::puts("Effect birth sequences: clear, reconstruction, stale phases/handles and exhaustion passed");
}
