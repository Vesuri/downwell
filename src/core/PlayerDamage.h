#ifndef DOWNWELL_CORE_PLAYER_DAMAGE_H
#define DOWNWELL_CORE_PLAYER_DAMAGE_H
#include "MotionBinary64.h"
#include "AudioLevel.h"
#include "AlarmBank.h"
#include "InstanceHandle.h"
#include "CurrencyHomingMath.h"
#include "ScreenShakeCaller.h"
namespace downwell {
// Borrow the shared flag; evaluate the object selector at the alarm write,
// after synchronous effects that may have changed the player population.
template<class World,class SelectPlayer> struct RegisteredPlayerDamageState {
    World& world;SelectPlayer selectPlayer;ScreenShakeCaller& shakeCaller;
    bool playerDamaged()const{return world.playerDamaged();}
    bool playerDead()const{return world.playerDeathActive();}
    bool playerHealthPositive()const{return world.playerHealthPositive();}
    bool shake(int amount,int duration){
        return shakeCaller.execute(amount,duration,[&](int a,int d){return world.requestCameraShake(a,d);});
    }
    void setPlayerDamaged(){world.setPlayerDamaged();}
    void setHit(){world.setPlayerHit();}
    bool candleOwned()const{return world.upgrades().levels[7]!=0;}
    bool timeoutOwned()const{return world.upgrades().levels[17]!=0;}
    void clearNoDamage(){world.clearNoDamageAchievement();}
    void clearAreaNoDamage(){world.clearAreaNoDamageAchievement();}
    // Same recovered lengthdir wrapper used by currency; this does not
    // supply atan2 or convert the result to the legacy player's Q8 storage.
    bool lengthdirX(MotionBinary64 length,MotionBinary64 direction,MotionBinary64& result){
        return CurrencyHomingMath::component(length,direction,false,result);
    }
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
// Explicit callbacks remain the script oracle. A registered state uses the
// world's actual membership and room-speed global instead of caller stubs.
template<class Events,class State> bool slowPlayerDamageControllers(Events& events,State&){
    return events.forEachController([&](auto& controller){
        events.setRoomSpeed(40);
        controller.setSlow(23);
        return controller.setAlarm(0,40);
    });
}
template<class Events,class World,class SelectPlayer>
bool slowPlayerDamageControllers(Events&,RegisteredPlayerDamageState<World,SelectPlayer>& state){
    return state.world.applyPlayerDamageSlowdown();
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
        return executeWithServices(amount,events,state,events);
    }
    template<class Events,class State,class Audio> bool executeWithServices(
        MotionBinary64 amount,Events& events,State& state,Audio& audio){
        if(!events.setDamageSourceName())return false;
        hitInvulnerability=state.candleOwned()?240:90;
        if(!state.playerDead() && !state.playerDamaged()){
            if(!state.subtractHealth(amount))return false;
            const auto py=events.playerY(),px=events.playerX();
            const auto cy=events.callerY(),cx=events.callerX();
            MotionBinary64 direction;
            if(!events.pointDirection(cx,cy,px,py,direction) ||
               !state.damageNumber(amount,direction) ||
               !audio.sound(5,90,0,0) ||
               !audio.duckMusic(500,AudioLevel::raw(45875)))return false;
            state.setHit();
            // room_speed is assigned once per controller in the with loop,
            // before that controller's slown and Alarm 0 writes.
            if(!slowPlayerDamageControllers(events,state))return false;
        }
        state.setPlayerDamaged();
        // Object-selector property assignment, NOT an invented with loop.
        return state.setPlayerAlarm(1,hitInvulnerability);
    }
};
}
#endif
