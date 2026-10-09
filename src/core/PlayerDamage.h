#ifndef DOWNWELL_CORE_PLAYER_DAMAGE_H
#define DOWNWELL_CORE_PLAYER_DAMAGE_H
#include "MotionBinary64.h"
#include "AlarmBank.h"
#include "InstanceHandle.h"
namespace downwell {
// Borrow the shared flag; evaluate the object selector at the alarm write,
// after synchronous effects that may have changed the player population.
template<class World,class SelectPlayer> struct RegisteredPlayerDamageState {
    World& world;SelectPlayer selectPlayer;
    bool playerDamaged()const{return world.playerDamaged();}
    void setPlayerDamaged(){world.setPlayerDamaged();}
    bool subtractHealth(MotionBinary64 amount){return world.subtractPlayerHealth(amount);}
    bool damageNumber(MotionBinary64 amount,MotionBinary64 direction){return world.emitPlayerDamageNumber(amount,direction);}
    bool setPlayerAlarm(unsigned index,int ticks){
        InstanceHandle player;
        return index==1&&selectPlayer(player)&&world.setPlayerDamageAlarm(player,ticks);
    }
};
// Borrow one registered controller for the scrPDamage WITH body. Resolve
// every write independently: a callback may retire/compact the owner.
template<class Resolve,class Apply>
bool withPlayerDamageController(Resolve resolve,Apply apply){
    struct Controller {
        Resolve& resolve;bool valid=true;
        void setSlow(int value){
            auto* record=valid?resolve():nullptr;
            if(!record){valid=false;return;}
            record->controllerSlow=value;
        }
        bool setAlarm(unsigned index,int value){
            auto* record=valid?resolve():nullptr;
            if(!record||index>=AlarmBank::count)return valid=false;
            record->alarms.set(index,value);return true;
        }
    } controller{resolve};
    return apply(controller)&&controller.valid;
}
// scrPDamage caller state. Source-name storage/resolution is supplied by Events
// because it is a GameMaker string, not a port-owned instance ID. All effect
// calls are synchronous; callback failure leaves earlier mutations in place.
struct PlayerDamage {
    int hitInvulnerability=0;
    template<class Events> bool execute(MotionBinary64 amount,Events& events){
        return executeWithState(amount,events,events);
    }
    template<class Events,class State> bool executeWithState(MotionBinary64 amount,Events& events,State& state){
        if(!events.setDamageSourceName())return false;
        hitInvulnerability=events.candleOwned()?240:90;
        if(!events.playerDead() && !state.playerDamaged()){
            if(!state.subtractHealth(amount))return false;
            const auto py=events.playerY(),px=events.playerX();
            const auto cy=events.callerY(),cx=events.callerX();
            MotionBinary64 direction;
            if(!events.pointDirection(cx,cy,px,py,direction) ||
               !state.damageNumber(amount,direction) ||
               !events.sound(5,90,0,0) ||
               !events.duckMusic(500,MotionBinary64{0x3fe66666u,0x66666666u}))return false;
            events.setHit();
            // room_speed is assigned once per controller in the with loop,
            // before that controller's slown and Alarm 0 writes.
            if(!events.forEachController([&](auto& controller){
                events.setRoomSpeed(40);
                controller.setSlow(23);
                return controller.setAlarm(0,40);
            }))return false;
        }
        state.setPlayerDamaged();
        // Object-selector property assignment, NOT an invented with loop.
        return state.setPlayerAlarm(1,hitInvulnerability);
    }
};
}
#endif
