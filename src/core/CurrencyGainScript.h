#ifndef DOWNWELL_CORE_CURRENCY_GAIN_SCRIPT_H
#define DOWNWELL_CORE_CURRENCY_GAIN_SCRIPT_H
#include "CurrencyCreate.h"
#include "RandomBinary64.h"
namespace downwell {
// scrGainGem, called by objCurrency's objPlayer_n collision event. Operations
// own global writes, original controller iteration, synchronous effects and
// lifetime-safe child selection. No missing operation is a successful no-op.
struct CurrencyGainScript {
    enum class AccountField {currency,gameGem,streak};
    template<class Random,class Ops> static bool run(CurrencyCreate& s,int amount,Random& random,Ops& ops,bool& good){
        if(!good)return false;
        if(!s.obtainable)return true;
        auto check=[&](bool ok){if(!ok)good=false;return ok;};
        s.gemAmount=amount;
        if(!check(ops.add(AccountField::currency,s.gemAmount)))return false;
        bool reached=false;if(!check(ops.currencyAtLeast(3000,reached)))return false;
        if(reached){
            if(!check(ops.achievement(15)&&ops.currencyAtLeast(5000,reached)))return false;
            if(reached&&!check(ops.achievement(22)))return false;
        }
        if(!check(ops.add(AccountField::gameGem,s.gemAmount)&&ops.add(AccountField::streak,s.gemAmount)&&ops.resetTimer()))return false;
        if(ops.lessResist()&&!check(ops.extendTimer(180)))return false;
        if(!check(ops.resetControllers()))return false;
        if(ops.gemPop()){
            MotionBinary64 x,y;
            if(!check(ops.playerPosition(x,y)&&y.add(MotionBinary64::fromInteger(-8))&&
                ops.effect(x,y,120,{0x3fd99999u,0x9999999au},0,0)))return false;
            typename Ops::Handle bullet;
            if(!check(ops.playerPosition(x,y)&&ops.create(266,x,y,bullet)&&ops.myBullet(bullet)))return false;
            MotionBinary64 direction;
            if(!check(randomBinary64(random.nextU32(),MotionBinary64::fromInteger(4),direction)&&
                direction.add(MotionBinary64::fromInteger(-2))&&direction.add(MotionBinary64::fromInteger(90))&&
                ops.bulletDirection(direction)))return false;
            const bool large=s.gemAmount>=10;
            if(!check(ops.bulletSprite(large?473:472)&&ops.bulletDamage(large?30:20)&&ops.bulletSpeed(6)&&
                ops.sound(341,70,false,true)))return false;
        }else if(!check(ops.soundOverlap(167,60,false,true,"UI")))return false;
        if(!check(ops.fxNoLoop(105,{0x3fe99999u,0x9999999au})))return false;
        if(ops.gemPowered()&&!check(ops.recharge()))return false;
        return check(ops.destroy());
    }
};
}
#endif
