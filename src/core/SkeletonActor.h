#ifndef DOWNWELL_CORE_SKELETON_ACTOR_H
#define DOWNWELL_CORE_SKELETON_ACTOR_H
#include "SkeletonDecisions.h"
#include "EnemyContact.h"
namespace downwell {
// Shared object-120 event body. Owner runs pre-Step animation and global alarm
// phases separately, supplies synchronous effect/child calls, and defers removal
// until this event has returned. This type does not own a world or renderer.
struct SkeletonActor : SkeletonDecisions {
    MotionBinary64 xx{}, yy{}, xsp{}, ysp{};
    std::int32_t x=0, y=0;
    int health=60, previousHealth=60, money=10, hitDamage=0;
    MotionBinary64 takenImpact{},hitDirection{},hitAngle{};
    bool damageStun=false, stomped=false;
    bool grounded=true, alive=true, destroyRequested=false;
    bool timefieldImmune=false;
    ContactBounds collisionBounds() const {
        auto bounds=contactSpriteBounds(208);
        // The explicit mask is horizontally symmetric; image_xscale cannot
        // change these bounds. Use published x/y, never the xx/yy accumulators.
        bounds.left+=x; bounds.right+=x; bounds.top+=y; bounds.bottom+=y;
        return bounds;
    }
    bool meetsPlayer(const ContactActor& player) const {
        return contactBoundsOverlap(collisionBounds(),contactWorldBounds(player));
    }
    // Precise player globals for the source Collision event, independently of
    // the published geometry used to decide whether that event is dispatched.
    // Player exposes current globalY(), grounded(), verticalSpeed(), stomp()
    // and damage(). Re-read after stomp: it may modify those player values.
    // No time-stop early return: the source event does not have that guard.
    template<class Player> bool collidePlayerAt(Player& player) {
        const auto publishedY=MotionBinary64::fromInteger(y);
        int order;
        if(!compareRunnerReal(publishedY,player.globalY(),order)) return false;
        if(order>0 && !player.grounded()) {
            if(!compareRunnerReal(player.verticalSpeed(),{},order)) return false;
            if(order>0) player.stomp();
        }
        if(player.grounded()) player.damage(1,3,2);
        else {
            if(!compareRunnerReal(player.globalY(),publishedY,order)) return false;
            if(order>0) player.damage(1,3,2);
        }
        return true;
    }
    void collideTimeField() { if(!timefieldImmune) health=0; }
    // scrEnemyStomped: callbacks are synchronous, including child Create/RNG.
    // forcedJump implements scrFjump (and its own recharge); the explicit
    // recharge call below is a second source call, not a duplicate to remove.
    // No death/reward or storage retirement is performed by this event.
    template<class Events> bool stompPlayer(Events& events) {
        if(events.playerDead()) return true;
        if(!events.blood(3,0)) return false;
        events.forcedJump(0,events.playStyle()==2?15:25); // vertical tenths
        events.shake(2,10);
        health-=100;
        events.stompSound(122);
        events.recharge(); // scrRecharge ignores its optional argument.
        hitPending=true;
        stomped=true;
        if(events.explodingKnees()) {
            if(!events.createBlast(276,x,y)) return false;
            events.explosionSound(91+events.choose(3));
            // Read current player globals separately for each synchronous call.
            for(unsigned i=0;i<4;++i) {
                const int directions[]={0,180,15,165};
                auto py=events.playerY();
                if(!py.add(MotionBinary64::fromInteger(4))) return false;
                if(!events.emitSmoke(events.playerX(),py,directions[i],i<2?4:3)) return false;
            }
        }
        return true;
    }
    // Current machine-gun branch: non-consistent, non-exploding bullet against
    // this non-immune/non-through enemy. Other weapon contracts are not implied.
    // Events owns the bullet's destruction and source point_direction call.
    template<class Events>
    bool hitByMachineGun(int damage,MotionBinary64 direction,Events& events) {
        if(damage!=9 && damage!=15) return false;
        if(health<=0) return true; // Source leaves the bullet alive in this case.
        events.destroyBullet();
        hitPending=true;
        hitDirection=direction;
        hitAngle=events.impactAngle(x,y);
        hitDamage+=damage;
        health-=damage;
        damageStun=true;
        events.momentDelay(); // scrDamageEnemy, even when already hit-stunned.
        events.bulletHitSound(29);
        takenImpact=damage==9?MotionBinary64{0x3ffcccccu,0xcccccccdu}:MotionBinary64{0x40080000u,0};
        events.impactEffect(605,4,direction,0); // 0.4 speed, no rounding to pixels.
        if(!hitStun) events.momentDelay();
        return true;
    }
    static bool snapCreateCoordinate(std::int32_t value,std::int32_t& snapped) {
        RunnerImageValue coordinate;
        if(!coordinate.assign(MotionBinary64::fromInteger(value))) return false;
        // Native instance x/y are binary32. Division by 16 is an exact exponent
        // adjustment for these integer inputs; original fistp rounds to even.
        if(coordinate.bits&0x7fffffffu) coordinate.bits-=4u<<23;
        std::int32_t grid;
        if(!coordinate.promoted().roundPixel(grid) || grid>134217727 || grid< -134217728) return false;
        snapped=grid*16;
        return true;
    }
    bool create(std::int32_t px, std::int32_t py) {
        std::int32_t snappedX,snappedY;
        if(!snapCreateCoordinate(px,snappedX) || !snapCreateCoordinate(py,snappedY)) return false;
        *this=SkeletonActor{};
        x=snappedX; y=snappedY;
        xx=MotionBinary64::fromInteger(x); yy=MotionBinary64::fromInteger(y);
        return true;
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
            // Creation callbacks complete synchronously and report failure.
            // Earlier side effects cannot be rolled back; false is terminal
            // for the owning simulation, not permission to retry this event.
            events.enemyDeath();
            if(!events.blood(10,3) || !events.smoke(xx,yy,5,1) ||
               !events.flash(xx,yy,6,2,10) || !events.currency(money)) return false;
            events.deathSound(132);
            if(!events.deadBody(206,takenImpact,102) ||
               !events.debris(x,y-5) || !events.debris(x,y-5)) return false;
            destroyRequested=true;
            events.destroy();
        }
        takenImpact={};
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
