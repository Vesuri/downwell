#include "core/RunSession.h"
#include "core/PlayerDamageNumber.h"
using downwell::MotionBinary64;
// Compile-only backend contract: declarations deliberately prevent the
// compiler from folding away any event branches or synchronous field Create.
struct DamageEvents {
    bool playerDamaged();
    MotionBinary64 playerY(),playerX(),callerY(),callerX();
    bool pointDirection(MotionBinary64,MotionBinary64,MotionBinary64,MotionBinary64,MotionBinary64&);
    bool damagePlayer(MotionBinary64);
    bool lengthdirX(MotionBinary64,MotionBinary64,MotionBinary64&);
    bool forcedJump(MotionBinary64,MotionBinary64);
    bool playerDead(),playerHealthPositive(),timeoutOwned(),timeStopped();
    bool setDamageSourceName(),candleOwned(),subtractHealth(MotionBinary64);
    bool damageNumber(MotionBinary64,MotionBinary64),sound(int,int,int,int),duckMusic(int,MotionBinary64);
    void setHit(),setRoomSpeed(int),setPlayerDamaged();
    bool setPlayerAlarm(int,int);
    struct Controller {void setSlow(int);bool setAlarm(int,int);};
    Controller* nextController();
    template<class Visit> bool forEachController(Visit visit){
        while(auto* controller=nextController())if(!visit(*controller))return false;
        return true;
    }
    bool shake(int,int);
    bool hitEffect(MotionBinary64,MotionBinary64,int,MotionBinary64,int,int);
    struct Player {int publishedY(),publishedX();};
    Player* nextPlayer();
    template<class Visit> bool forEachPlayer(Visit visit){
        while(auto* player=nextPlayer())if(!visit(*player))return false;
        return true;
    }
    void clearNoDamage(),clearAreaNoDamage();
};
bool compilePlayerDamage(downwell::RunSession& run,downwell::PlayerDamage& caller,
                        DamageEvents& events){
    return run.damagePlayer(caller,MotionBinary64::fromInteger(1),events);
}
bool compileTypicalPlayerDamage(downwell::RunSession& run,downwell::TypicalPlayerDamage& caller,
    downwell::GameMakerRandom& random,DamageEvents& events){
    return run.typicalPlayerDamage(caller,MotionBinary64::fromInteger(1),
        MotionBinary64::fromInteger(3),MotionBinary64::fromInteger(2),random,events);
}
struct NumberDraw {
    void align(int),color(int);
    bool text(MotionBinary64,MotionBinary64,MotionBinary64);
};
bool compileDamageNumber(downwell::PlayerDamageNumber& effect,DamageEvents& events,NumberDraw& draw){
    effect.advanceAlarm(0);
    return effect.step([&](auto speed,auto angle,auto& out){return events.lengthdirX(speed,angle,out);},
        [](bool,const auto&){return true;})&&effect.draw(draw);
}
bool compileDamageNumberOwner(downwell::RunSession& run,DamageEvents& events){
    auto h=run.createDamageNumber({}, {},MotionBinary64::fromInteger(1),{});
    auto phase=run.damageNumbers.beginPhase();
    return h&&run.advanceDamageNumberAlarm(phase,0)&&run.stepDamageNumbers(phase,
        [&](auto speed,auto angle,auto& out){return events.lengthdirX(speed,angle,out);});
}
