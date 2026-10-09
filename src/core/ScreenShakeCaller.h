#ifndef DOWNWELL_CORE_SCREEN_SHAKE_CALLER_H
#define DOWNWELL_CORE_SCREEN_SHAKE_CALLER_H
namespace downwell {
// scrSShake self fields for the admitted integral calls. All shake scripts
// on one attacking instance must borrow this same storage.
struct ScreenShakeCaller {
    int shakeAmt=0,shakeDur=0;
    template<class Apply> bool execute(int amount,int duration,Apply apply){
        shakeAmt=amount;shakeDur=duration;
        return apply(shakeAmt,shakeDur);
    }
};
}
#endif
