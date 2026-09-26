#include "core/RunSession.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace downwell;
struct Draw {unsigned birth;bool number;};
static std::vector<Draw> draws(const RunSession& run){
    std::vector<Draw> result;
    assert(run.visitForegroundEffects([&](const auto& e){result.push_back({e.drawBirth,false});return true;},
        [&](const auto& n){result.push_back({n.drawBirth,true});return true;}));
    for(unsigned i=1;i<result.size();++i)assert(result[i-1].birth>result[i].birth);
    return result;
}
int main(){
    {
        PlayerDamageNumbers<1> numbers;CorpseExpiryEffects<1> expiry;
        assert(numbers.restoreEmptyBirthSequence({42})&&expiry.restoreEmptyBirthSequence({42}));
        assert(!numbers.restoreEmptyBirthSequence({1})&&!expiry.restoreEmptyBirthSequence({1}));
        assert(numbers.restoreEmptyBirthSequence({0})&&expiry.restoreEmptyBirthSequence({0}));
        assert(!numbers.restoreEmptyBirthSequence({42})&&!expiry.restoreEmptyBirthSequence({42}));
        RunSession::DamageNumberHooks numberHooks;RunSession::ExpiryHooks expiryHooks;
        assert(!numbers.create({}, {},numberHooks)&&!expiry.create({}, {},expiryHooks));
        assert(numbers.clear(numberHooks)&&expiry.clear(expiryHooks));
        assert(!numbers.create({}, {},numberHooks)&&!expiry.create({}, {},expiryHooks));
    }
    for(unsigned pattern=0;pattern<256;++pattern){
        RunSession run;
        for(unsigned i=0;i<8;++i){
            if(pattern&(1u<<i))assert(run.createDamageNumber({}, {},MotionBinary64::fromInteger(1),{}));
            else assert(run.createCorpseExpiry({},{}));
        }
        auto result=draws(run);assert(result.size()==8);
        for(unsigned i=0;i<8;++i){assert(result[i].birth==8-i);assert(result[i].number==bool(pattern&(1u<<(7-i))));}
        assert(draws(run).size()==8&&run.nextForegroundBirth==9); // Rendering has no simulation writes.
        unsigned visits=0;
        assert(!run.visitForegroundEffects([&](const auto&){return ++visits<3;},[&](const auto&){return ++visits<3;}));
        assert(visits==3&&run.effectsSucceeded);
        for(unsigned i=0;i<40;++i)run.advanceCorpseExpiryAnimation();
        result=draws(run);
        for(const auto& d:result)assert(d.number);
        // Reusing a dead expiry slot still inserts ahead of every survivor.
        const auto expiry=run.createCorpseExpiry({},{});assert(expiry);
        result=draws(run);assert(result.front().birth==9&&!result.front().number);
        auto phase=run.damageNumbers.beginPhase();
        for(unsigned i=0;i<60;++i)assert(run.advanceDamageNumberAlarm(phase,0));
        result=draws(run);assert(result.size()==1&&result[0].birth==9);
        run.reset();assert(draws(run).empty()&&run.nextForegroundBirth==10);
        assert(run.createDamageNumber({}, {},MotionBinary64::fromInteger(1),{}));
        result=draws(run);assert(result.size()==1&&result[0].birth==10&&result[0].number);
    }
    {
        RunSession run;run.nextForegroundBirth=0xffffffffu;
        assert(run.createCorpseExpiry({},{}));
        auto result=draws(run);assert(result.size()==1&&result[0].birth==0xffffffffu);
        assert(!run.createDamageNumber({}, {},MotionBinary64::fromInteger(1),{})&&!run.effectsSucceeded);
        assert(run.damageNumbers.size()==0&&run.corpseExpiryEffects.size()==1);
        run.reset();assert(run.effectsSucceeded&&!run.createCorpseExpiry({},{}));
        assert(!run.effectsSucceeded&&run.corpseExpiryEffects.size()==0);
    }
    {
        RunSession run;RunSession::ExpiryHooks hooks;
        assert(run.corpseExpiryEffects.create({}, {},hooks));
        // Unregistered births cannot be silently assigned an arbitrary family order.
        unsigned visits=0;
        assert(!run.visitForegroundEffects([&](const auto&){++visits;return true;},[&](const auto&){++visits;return true;}));
        assert(!visits);
    }
    std::puts("Foreground effects: 256 cross-family birth tapes, newest-first merge, retirement/reuse, read-only projection, failure and exhausted keys passed");
}
