#ifndef DOWNWELL_TEST_TIMEOUT_DAMAGE_EVENTS_H
#define DOWNWELL_TEST_TIMEOUT_DAMAGE_EVENTS_H
#include "core/MotionBinary64.h"
// Test environment for the real script/registered field path. Other effects
// are excluded by a lethal hit; changing stop state exercises per-player reads.
struct TimeoutDamageEvents {
    using Number=downwell::MotionBinary64;
    bool immune=false,timeout=true,stopped=false,stopSecond=true;
    unsigned players=0,clears=0;
    int alarm=-1;bool spin=false,grounded=true;Number vx{},vy{};
    bool playerDamaged(){return immune;}
    Number playerX(){return Number::fromInteger(240);}
    Number playerY(){return Number::fromInteger(40000);}
    Number callerX(){return playerX();}
    Number callerY(){return playerY();}
    bool pointDirection(Number,Number,Number,Number,Number& out){out={};return true;}
    bool damagePlayer(Number){return true;}
    bool lengthdirX(Number speed,Number,Number& out){out=speed;return true;}
    bool forcedJump(Number,Number){return true;}
    bool setDamageSourceName(){return true;}
    bool candleOwned(){return false;}
    bool subtractHealth(Number){return false;}
    bool damageNumber(Number,Number){return false;}
    bool sound(int,int,bool,bool){return false;}
    bool duckMusic(int,Number){return false;}
    void setHit(){}
    template<class Visit> bool forEachController(Visit){return false;}
    void setRoomSpeed(int){}
    void setPlayerDamaged(){immune=true;}
    bool setPlayerAlarm(unsigned index,int ticks){alarm=ticks;return index==1;}
    void setSpinJumping(){spin=true;}
    bool recharge(){return true;}
    bool setGrounded(bool value){grounded=value;return true;}
    bool setHorizontal(Number value){vx=value;return true;}
    bool setVertical(Number value){vy=value;return true;}
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
// Stop immediately after the real health/number prefix, before any fake audio
// can conceal missing production services.
struct DamageSoundFailureEvents:TimeoutDamageEvents {
    unsigned soundCalls=0;
    bool playerDead(){return false;}
    bool sound(int id,int priority,bool loop,bool cut){
        soundCalls+=(id==5&&priority==90&&!loop&&!cut)?1:1000;
        return false;
    }
};
#endif
