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
    // Exercise the live-contact coordinate and point-direction adapter too,
    // including negative/fractional Q8 coordinates and non-cardinal angles.
    for(int sample=0;sample<8;++sample){
        run.reset();
        result.check(run.contactDamageNumber(sample*173-600,777-sample*239,
            sample-4,3-sample,240,176));
        const auto* effect=run.damageNumbers.atNewest(0);
        result.check(effect!=nullptr);
        if(effect){
            result.word(effect->x.bits);result.word(effect->y.bits);
            result.real(effect->direction);result.real(effect->amount);
            result.check(effect->alarm0==60);
            result.check(run.stepDamageNumbers(run.damageNumbers.beginPhase()));
            result.real(effect->xSpeed);result.real(effect->ySpeed);
        }
    }
    for(int sample=0;sample<6;++sample){
        run.reset();ForcedPlayerJump jump;
        struct Events {
            DamageNumberVerification& result;
            void setSpinJumping(){result.word(1);}
            bool recharge(){result.word(2);return true;}
            bool setGrounded(bool value){result.word(3);result.check(!value);return true;}
            bool setHorizontal(MotionBinary64 value){result.word(4);result.real(value);return true;}
            bool setVertical(MotionBinary64 value){result.word(5);result.real(value);return true;}
        } events{result};
        result.check(run.forcedPlayerJump(jump,MotionBinary64::fromInteger(sample%3-1),
            MotionBinary64::fromInteger(sample/3),events));
        result.real(jump.horizontal);result.real(jump.vertical);result.real(jump.defaultVertical);
    }
    run.~RunSession();return result;
}
}
#endif
