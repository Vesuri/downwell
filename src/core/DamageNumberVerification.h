#ifndef DOWNWELL_CORE_DAMAGE_NUMBER_VERIFICATION_H
#define DOWNWELL_CORE_DAMAGE_NUMBER_VERIFICATION_H
#include "RunSession.h"
#ifndef DOWNWELL_PLATFORM_AMIGA
#include <new>
#endif
namespace downwell {
struct DamageNumberVerification {
    unsigned checks=0,failures=0;std::uint32_t digest=2166136261u;
    void check(bool okay){++checks;if(!okay)++failures;}
    void word(std::uint32_t value){digest=(digest^value)*16777619u;}
    void real(MotionBinary64 value){word(value.high);word(value.low);}
};
// Use the real session dispatch/math path; keep its storage off the A500 task
// stack. This checks cross-CPU state, not font rasterization or natural damage.
[[gnu::noinline]] inline DamageNumberVerification verifyDamageNumbers(){
    alignas(RunSession) static std::uint8_t storage[sizeof(RunSession)];
    auto& run=*new(storage) RunSession;
    DamageNumberVerification result;
    for(int sample=0;sample<8;++sample){
        run.reset();
        result.check(run.createCorpseExpiry({}, {})!=0);
        auto h=run.createDamageNumber(MotionBinary64::fromInteger(240+sample),
            MotionBinary64::fromInteger(1250),MotionBinary64::fromInteger(1+sample%4),
            MotionBinary64::fromInteger(sample*45));
        result.check(h!=0);
        for(unsigned tick=0;tick<60;++tick){
            run.advanceCorpseExpiryAnimation();
            const auto phase=run.damageNumbers.beginPhase();
            result.check(run.advanceDamageNumberAlarm(phase,0)&&run.stepDamageNumbers(phase));
            result.word(run.damageNumbers.size());
            unsigned order=0;
            result.check(run.visitForegroundEffects([&](const auto& expiry){
                result.check(order==1);++order;result.word(expiry.drawBirth);return true;
            },[&](const auto& number){
                result.check(order==0);++order;result.word(number.drawBirth);return true;
            }));
            result.check(order==(tick<39?2u:tick<59?1u:0u));
            const auto* effect=run.damageNumbers.get(h);
            result.check((effect!=nullptr)==(tick<59));
            if(!effect)continue;
            result.word(effect->drawBirth);result.word(effect->x.bits);result.word(effect->y.bits);
            result.real(effect->xSpeed);result.real(effect->ySpeed);
            result.real(effect->amount);result.real(effect->direction);result.word(effect->alarm0);
        }
        result.check(run.effectsSucceeded);
    }
    run.~RunSession();return result;
}
}
#endif
