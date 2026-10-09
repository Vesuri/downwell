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
// Advance past sound/duck, then fail at controller traversal. The hit marker
// must already be published, but immunity/alarm writes must not have happened.
struct DamageControllerFailureEvents:DamageSoundFailureEvents {
    bool sound(int id,int priority,bool loop,bool cut){
        DamageSoundFailureEvents::sound(id,priority,loop,cut);return true;
    }
    bool duckMusic(int,Number){return true;}
};
// Explicit test effects for exercising real world state. These are not a
// backend implementation. Deliberately stale death/health answers must be
// ignored by the registered binding; effects still validate source arguments.
struct DamageLiveStateEvents:TimeoutDamageEvents {
    unsigned sounds=0,ducks=0,shakes=0,hits=0;
    bool deadAnswer=true;
    bool playerDead(){return deadAnswer;}
    bool sound(int id,int priority,bool loop,bool cut){
        ++sounds;return id==5&&priority==90&&!loop&&!cut;
    }
    bool duckMusic(int ms,Number amount){
        ++ducks;return ms==500&&amount.high==0x3fe66666u&&amount.low==0x66666666u;
    }
    bool shake(int force,int duration){++shakes;return force==5&&duration==8;}
    bool hitEffect(Number,Number,int sprite,Number speed,int angle,int depth){
        ++hits;return sprite==114&&speed.high==0x3fd99999u&&speed.low==0x9999999au&&angle==0&&depth==-100000;
    }
};
#endif
