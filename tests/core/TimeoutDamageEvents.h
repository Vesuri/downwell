#ifndef DOWNWELL_TEST_TIMEOUT_DAMAGE_EVENTS_H
#define DOWNWELL_TEST_TIMEOUT_DAMAGE_EVENTS_H
#include "core/MotionBinary64.h"
// Test environment for the real script/registered field path. Other effects
// are excluded by a lethal hit; changing stop state exercises per-player reads.
struct TimeoutDamageEvents {
    using Number=downwell::MotionBinary64;
    bool immune=false,timeout=true,stopped=false,stopSecond=true;
    unsigned players=0,clears=0;
    bool playerDamaged(){return immune;}
    Number playerX(){return Number::fromInteger(240);}
    Number playerY(){return Number::fromInteger(40000);}
    Number callerX(){return playerX();}
    Number callerY(){return playerY();}
    bool pointDirection(Number,Number,Number,Number,Number& out){out={};return true;}
    bool damagePlayer(Number){return true;}
    bool lengthdirX(Number speed,Number,Number& out){out=speed;return true;}
    bool forcedJump(Number,Number){return true;}
    bool playerDead(){return true;}
    bool shake(int,int){return false;}
    bool playerHealthPositive(){return false;}
    bool hitEffect(Number,Number,int,Number,int,int){return false;}
    bool timeoutOwned(){return timeout;}
    bool timeStopped(){return stopped;}
    struct Player {int publishedY(){return 40100;}int publishedX(){return 240;}};
    template<class Visit> bool forEachPlayer(Visit visit){
        Player player;++players;if(!visit(player))return false;
        if(stopSecond)stopped=true;
        ++players;return visit(player);
    }
    void clearNoDamage(){++clears;}
    void clearAreaNoDamage(){++clears;}
};
#endif
