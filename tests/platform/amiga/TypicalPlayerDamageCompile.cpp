#include "core/RunSession.h"
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
bool compileTypicalPlayerDamage(downwell::RunSession& run,downwell::TypicalPlayerDamage& caller,
    downwell::GameMakerRandom& random,DamageEvents& events){
    return run.typicalPlayerDamage(caller,MotionBinary64::fromInteger(1),
        MotionBinary64::fromInteger(3),MotionBinary64::fromInteger(2),random,events);
}
