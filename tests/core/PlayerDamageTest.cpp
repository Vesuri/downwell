#include "core/RunSession.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace downwell;
static auto n(int v){return MotionBinary64::fromInteger(v);}
static bool eq(MotionBinary64 a,MotionBinary64 b){return a.high==b.high && a.low==b.low;}
struct Events {
    std::vector<int> log;
    bool candle=false,dead=false,damaged=false,hit=false,mutate=false;
    int health=4,initialHealth=4,roomSpeed=60,alarm=-1,controllers=2,fail=0;
    int slow[2]{},controllerAlarm[2]{},x=250,y=500;
    bool record(int v){log.push_back(v);return v!=fail;}
    bool setDamageSourceName(){return record(1);}
    bool candleOwned(){record(2);return candle;}
    bool playerDead(){record(3);return dead;}
    bool playerDamaged(){record(4);return damaged;}
    bool subtractHealth(MotionBinary64 amount){assert(eq(amount,n(1)));--health;return record(5);}
    auto playerY(){record(6);return n(y);}
    auto playerX(){record(7);return n(x);}
    auto callerY(){record(8);return n(490);}
    auto callerX(){record(9);return n(240);}
    bool pointDirection(MotionBinary64 cx,MotionBinary64 cy,MotionBinary64 px,MotionBinary64 py,MotionBinary64& out){
        assert(eq(cx,n(240))&&eq(cy,n(490))&&eq(px,n(250))&&eq(py,n(500)));
        out=n(315);return record(10);
    }
    bool damageNumber(MotionBinary64 amount,MotionBinary64 direction){
        assert(health==initialHealth-1 && eq(amount,n(1)) && eq(direction,n(315)));
        if(mutate){candle=!candle;dead=true;damaged=true;x=900;y=901;}
        return record(11);
    }
    bool sound(int id,int priority,int loop,int variation){
        assert(id==5 && priority==90 && loop==0 && variation==0);return record(12);
    }
    bool duckMusic(int ms,MotionBinary64 gain){assert(ms==500 && eq(gain,{0x3fe66666u,0x66666666u}));return record(13);}
    void setHit(){hit=true;record(14);}
    struct Controller {
        Events& events;int index;
        void setSlow(int value){assert(events.roomSpeed==40 && value==23);events.slow[index]=value;events.record(17);}
        bool setAlarm(int index,int value){assert(index==0 && value==40 && events.slow[this->index]==23);
            events.controllerAlarm[this->index]=value;return events.record(18);}
    };
    template<class Visit> bool forEachController(Visit visit){
        if(!record(15))return false;
        for(int i=0;i<controllers;++i){Controller controller{*this,i};if(!visit(controller))return false;}
        return true;
    }
    void setRoomSpeed(int value){assert(value==40);roomSpeed=value;record(16);}
    void setPlayerDamaged(){damaged=true;record(19);}
    bool setPlayerAlarm(int index,int value){assert(index==1 && damaged);alarm=value;return record(20);}
};
int main(){
    const std::vector<int> full{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,16,17,18,19,20};
    for(unsigned flags=0;flags<8;++flags){
        RunSession run;PlayerDamage caller;Events events;
        const bool candle=flags&1,dead=flags&2,damaged=flags&4;
        events.candle=candle;events.dead=dead;events.damaged=damaged;
        assert(run.damagePlayer(caller,n(1),events));
        assert(caller.hitInvulnerability==(candle?240:90) && events.alarm==caller.hitInvulnerability && events.damaged);
        if(dead || damaged){
            assert(events.health==4 && !events.hit && events.roomSpeed==60);
            assert(events.log==(dead?std::vector<int>{1,2,3,19,20}:std::vector<int>{1,2,3,4,19,20}));
        }else{
            assert(events.health==3 && events.hit && events.log==full);
            assert(events.slow[0]==23 && events.slow[1]==23 && events.controllerAlarm[0]==40 && events.controllerAlarm[1]==40);
        }
        events.log.clear();
        assert(run.damagePlayer(caller,n(1),events) && events.health==(dead||damaged?4:3));
        assert(events.log==(dead?std::vector<int>{1,2,3,19,20}:std::vector<int>{1,2,3,4,19,20}));
    }
    for(bool candle:{false,true}){
        RunSession run;PlayerDamage caller;Events events;events.mutate=true;events.candle=candle;
        assert(run.damagePlayer(caller,n(1),events) && events.log==full);
        assert(events.dead && events.damaged && events.hit && events.candle!=candle);
        assert(events.alarm==(candle?240:90)); // Chosen before synchronous effects.
    }
    for(int health:{1,0,-2}){
        RunSession run;PlayerDamage caller;Events events;events.health=events.initialHealth=health;
        assert(run.damagePlayer(caller,n(1),events));
        assert(events.health==health-1 && events.log==full && !events.dead);
        // Health is not an entry guard and this script does not publish death.
    }
    for(int fail:{1,5,10,11,12,13,15,18,20}){
        RunSession run;PlayerDamage caller;Events events;events.fail=fail;
        assert(!run.damagePlayer(caller,n(1),events) && !run.effectsSucceeded && events.log.back()==fail);
        auto log=events.log;
        assert(!run.damagePlayer(caller,n(1),events) && events.log==log);
        assert(events.health==(fail==1?4:3)); // Failure cannot roll back health.
        if(fail==1)assert(caller.hitInvulnerability==0);
    }
    {
        RunSession run;PlayerDamage caller;Events events;events.controllers=0;
        assert(run.damagePlayer(caller,n(1),events));
        assert(events.health==3 && events.hit && events.damaged && events.roomSpeed==60 && events.alarm==90);
    }
    std::puts("Player damage: health/immunity/death gates, caller state, controller ordering, synchronous mutation and failure passed");
}
