#ifndef DOWNWELL_CORE_SKELETON_ACTOR_H
#define DOWNWELL_CORE_SKELETON_ACTOR_H
#include "SkeletonDecisions.h"
namespace downwell {
// Shared object-120 event body. Owner runs pre-Step animation and global alarm
// phases separately, supplies synchronous effect/child calls, and defers removal
// until this event has returned. This type does not own a world or renderer.
struct SkeletonActor : SkeletonDecisions {
    MotionBinary64 xx{}, yy{}, xsp{}, ysp{};
    std::int32_t x=0, y=0;
    int health=60, previousHealth=60, money=10, takenImpact=0;
    bool grounded=true, alive=true, destroyRequested=false;
    void create(std::int32_t px, std::int32_t py) {
        *this=SkeletonActor{};
        x=px; y=py;
        xx=MotionBinary64::fromInteger(px); yy=MotionBinary64::fromInteger(py);
    }
    // Live owners pass precise original globals, not rounded player pixels.
    template<class Events>
    bool stepAtPlayer(bool stopped,MotionBinary64 playerX,MotionBinary64 playerY,Events& events) {
        if(stopped) return step(true,false,false,0,events);
        auto activation=yy;
        int vertical,horizontal,movement;
        if(!activation.add(MotionBinary64::fromInteger(-96)) ||
           !compareRunnerReal(playerY,activation,vertical) ||
           !compareRunnerReal(MotionBinary64::fromInteger(x),playerX,horizontal) ||
           !compareRunnerReal(xsp,{},movement)) return false;
        return step(false,vertical>0,horizontal>0,movement,events);
    }
    // scrOutofview runs in End Step, including time-stop. It removes only in
    // rmMain and does not invoke enemy death, currency or hit effects.
    template<class Events>
    bool endStep(bool mainRoom,MotionBinary64 viewY,Events& events) {
        if(!mainRoom) return true;
        auto top=viewY,bottom=viewY;
        int before,after;
        const auto publishedY=MotionBinary64::fromInteger(y);
        if(!top.add(MotionBinary64::fromInteger(-160)) ||
           !bottom.add(MotionBinary64::fromInteger(3000)) ||
           !compareRunnerReal(publishedY,top,before) ||
           !compareRunnerReal(publishedY,bottom,after)) return false;
        if(before<0 || after>0) { destroyRequested=true; events.destroy(); }
        return true;
    }
    template<class Events>
    bool step(bool stopped, bool playerBelowActivation, bool playerToLeft,
              int horizontalSpeedSign, Events& events) {
        if (!stepDecisions(stopped,playerBelowActivation,playerToLeft,
                           horizontalSpeedSign,events)) return false;
        if (!stopped) {
            auto nx=xx, ny=yy; std::int32_t px,py;
            if ((!grounded && !nx.add(xsp)) || !ny.add(ysp) ||
                !nx.roundPixel(px) || !ny.roundPixel(py)) return false;
            xx=nx; yy=ny; x=px; y=py;
        }
        if (health<=0) alive=false;
        if (!alive) {
            events.enemyDeath();
            events.blood(10,3);
            events.smoke(xx,yy,5,1);
            events.flash(xx,yy,6,2,10);
            events.currency(money);
            events.deathSound(132);
            events.deadBody(206,takenImpact,102);
            events.debris(x,y-5);
            events.debris(x,y-5);
            destroyRequested=true;
            events.destroy();
        }
        takenImpact=0;
        return finishPresentation();
    }
    // Predicates must be evaluated from the current source values with the
    // runner comparison tolerance. Re-query after stomp: its script may mutate
    // the player. These are two independent source IFs, not an else-if pair.
    template<class Contact>
    void collidePlayer(Contact& contact) {
        if (contact.skeletonBelowPlayer() && !contact.playerGrounded() &&
            contact.playerDescending()) contact.stomp();
        if (contact.playerGrounded()) contact.damage(1,3,2);
        else if (contact.playerBelowSkeleton()) contact.damage(1,3,2);
    }
};
}
#endif
