#ifndef DOWNWELL_CORE_SCATTERED_EVENTS_H
#define DOWNWELL_CORE_SCATTERED_EVENTS_H
#include "MotionBinary64.h"
namespace downwell {
// Object 289's VM events. The caller owns synchronous child creation, queries,
// setter publication and audio. In particular currency() must retain myGem;
// attractGem() changes that returned instance, not every gem in the batch.
struct ScatteredEvents {
    template<class State,class Random,class Effects>
    static bool step(State& s,Random& random,Effects& effects){
        // instance_destroy in scrOutofview does not return from this event.
        if(!effects.outOfView())return false;
        if(s.objHp>0||!s.scriptActive)return true;
        if(!effects.currency(2))return false;
        if(s.stomped&&!effects.attractGem())return false;
        for(unsigned i=0;i<2;++i){
            if(!effects.createDebris())return false;
            if(!effects.debrisYSpeed(MotionBinary64{0xbfe00000u,0}))return false;
        }
        s.breakSound=171+int(random.chooseIndexSmall(4));
        if(!effects.sound(s.breakSound,50,0,1))return false;
        if(!effects.sprite(551))return false;
        s.spriteId=551;
        // This is the script's active variable, not runner deactivation. The
        // broken sprite retains its mask, frame and zero image_speed.
        s.scriptActive=0;
        return true;
    }
    template<class State,class Random,class Effects>
    static bool playerContact(State& s,Random& random,Effects& effects){
        if(!s.scriptActive)return true;
        MotionBinary64 speed;
        if(!effects.playerYSpeed(speed)||!speed.valid())return false;
        if(speed.zero()||(speed.high&0x80000000u))return true;
        s.objHp=0;s.stomped=1;
        int style;
        if(!effects.playStyle(style))return false;
        if(!effects.jump(0,MotionBinary64{style==2?0x3ff80000u:0x40040000u,0})||
           !effects.shake(2,4))return false;
        bool knees;
        if(!effects.explodingKnees(knees))return false;
        if(knees){
            if(!effects.createBlast())return false;
            // Child Create may consume RNG before this choose.
            if(!effects.sound(91+int(random.chooseIndexSmall(3)),60,0,1))return false;
        }
        return true;
    }
    template<class State> static void alarm0(State& s){s.hitStun=0;}
};
}
#endif
