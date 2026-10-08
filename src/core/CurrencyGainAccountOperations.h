#ifndef DOWNWELL_CORE_CURRENCY_GAIN_ACCOUNT_OPERATIONS_H
#define DOWNWELL_CORE_CURRENCY_GAIN_ACCOUNT_OPERATIONS_H
#include "CurrencyAccount.h"
#include "CurrencyGainScript.h"
#include "GemStreakRise.h"
namespace downwell {
// Shared account publication for the admitted nonnegative uint32 account
// domain. Effects/controllers remain mandatory operations on Controls.
template<class Controls> struct CurrencyGainAccountOperations:Controls {
    CurrencyAccount& account;
    GemStreakRise* rise;
    CurrencyGainAccountOperations(CurrencyAccount& value,Controls controls,GemStreakRise* controller=nullptr)
        :Controls(controls),account(value),rise(controller){}
    bool resetControllers(){
        if(!Controls::resetControllers())return false;
        if(rise)rise->reset();
        return true;
    }
    bool add(CurrencyGainScript::AccountField field,int amount){
        auto& value=field==CurrencyGainScript::AccountField::currency?account.currency:
            field==CurrencyGainScript::AccountField::gameGem?account.gameGem:account.streak;
        if(amount<0||std::uint32_t(amount)>0xffffffffu-value)return false;
        value+=std::uint32_t(amount);return true;
    }
    bool currencyAtLeast(unsigned threshold,bool& result){result=account.currency>=threshold;return true;}
    bool resetTimer(){return this->timerStart(account.timer);}
    bool extendTimer(unsigned amount){if(amount>0xffffffffu-account.timer)return false;account.timer+=amount;return true;}
};
}
#endif
