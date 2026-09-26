#ifndef DOWNWELL_CORE_PLAYER_DAMAGE_H
#define DOWNWELL_CORE_PLAYER_DAMAGE_H
#include "MotionBinary64.h"
namespace downwell {
// scrPDamage caller state. Source-name storage/resolution is supplied by Events
// because it is a GameMaker string, not a port-owned instance ID. All effect
// calls are synchronous; callback failure leaves earlier mutations in place.
struct PlayerDamage {
    int hitInvulnerability=0;
    template<class Events> bool execute(MotionBinary64 amount,Events& events){
        if(!events.setDamageSourceName())return false;
        hitInvulnerability=events.candleOwned()?240:90;
        if(!events.playerDead() && !events.playerDamaged()){
            if(!events.subtractHealth(amount))return false;
            const auto py=events.playerY(),px=events.playerX();
            const auto cy=events.callerY(),cx=events.callerX();
            MotionBinary64 direction;
            if(!events.pointDirection(cx,cy,px,py,direction) ||
               !events.damageNumber(amount,direction) ||
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
        events.setPlayerDamaged();
        // Object-selector property assignment, NOT an invented with loop.
        return events.setPlayerAlarm(1,hitInvulnerability);
    }
};
}
#endif
