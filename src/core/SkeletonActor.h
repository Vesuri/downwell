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
