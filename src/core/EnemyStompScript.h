#ifndef DOWNWELL_CORE_ENEMY_STOMP_SCRIPT_H
#define DOWNWELL_CORE_ENEMY_STOMP_SCRIPT_H
#include "MotionBinary64.h"
namespace downwell {
// Shared scrEnemyStomped event sequence. Borrow the caller's variables and
// synchronous services; do not move child creation or either recharge call.
struct EnemyStompScript {
    template<class Jump,class Shake,class Events,class Blast>
    static bool run(int& health,bool& hit,bool& stomped,Jump& jump,Shake& shake,
                    Events& events,Blast blast,bool& good){
        if(!good)return false;
        if(events.playerDead())return true;
        if(!events.blood(3,0))return good=false;
        const MotionBinary64 vertical=events.playStyle()==2?MotionBinary64{0x3ff80000u,0}:MotionBinary64{0x40040000u,0};
        if(!jump.execute({},vertical,events)||
           !shake.execute(2,10,[&](int amount,int duration){return events.shake(amount,duration);}))return good=false;
        if(health<(-2147483647-1)+100)return good=false;
        health-=100;
        if(!events.stompSound(122,50,false,true,"footsteps")||!events.recharge())return good=false;
        hit=true;stomped=true;
        if(events.explodingKnees()){
            if(!blast())return good=false;
            const unsigned choice=events.choose(3);
            if(choice>2||!events.explosionSound(91+choice,60,false,true))return good=false;
            for(unsigned i=0;i<4;++i){
                const int directions[]={0,180,15,165};
                auto py=events.playerY();
                if(!py.add(MotionBinary64::fromInteger(4))||
                   !events.emitSmoke(events.playerX(),py,directions[i],i<2?4:3))return good=false;
            }
        }
        return true;
    }
};
}
#endif
