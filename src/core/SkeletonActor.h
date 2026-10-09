#ifndef DOWNWELL_CORE_SKELETON_ACTOR_H
#define DOWNWELL_CORE_SKELETON_ACTOR_H
#include "SkeletonDecisions.h"
#include "EnemyContact.h"
#include "OutOfViewLifetime.h"
#include "SkeletonEffectBursts.h"
namespace downwell {
// Shared object-120 event body. Owner runs pre-Step animation and global alarm
// phases separately, supplies synchronous effect/child calls, and defers removal
// until this event has returned. This type does not own a world or renderer.
struct SkeletonActor : SkeletonDecisions {
    MotionBinary64 xx{}, yy{}, xsp{}, ysp{};
    std::int32_t x=0, y=0;
    int health=60, previousHealth=60, money=10, hitDamage=0;
    MotionBinary64 takenImpact{},hitDirection{},hitAngle{};
    // scrDeadBody writes these caller fields before the enemy is retired.
    MotionBinary64 impactSp{};
    int bodySprite=0;
    std::uint32_t myDeadBody=0;
    SmokeBurstCaller smokeScript;
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
            if(!compareRunnerReal(player.globalY(),MotionBinary64::fromInteger(y),order)) return false;
            if(order>0) player.damage(1,3,2);
        }
        return true;
    }
    void collideTimeField() { if(!timefieldImmune) health=0; }
    // Selected source player event. Stomp is synchronous and can mutate both
    // actors; the second IF re-reads grounded, global ply and self.y. Preserve
    // this separately from the scalar successful-event reference above.
    template<class Player> bool collidePlayerPublished(Player& player,bool& good){
        if(!good)return false;
        int order;
        if(!compareRunnerReal(MotionBinary64::fromInteger(y),player.globalY(),order))return good=false;
        if(order>0&&!player.grounded()){
            if(!compareRunnerReal(player.verticalSpeed(),{},order))return good=false;
            if(order>0&&!player.stomp())return good=false;
        }
        if(player.grounded())return player.damage(1,3,2)||(good=false);
        if(!compareRunnerReal(player.globalY(),MotionBinary64::fromInteger(y),order))return good=false;
        return order<=0||player.damage(1,3,2)||(good=false);
    }
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
    // Checked live form of bulletStandard Collision subparentEnemy (2196),
    // restricted to its two non-consistent/non-exploding usual-gun profiles
    // against object 120 (neither immune nor bullet-through). The older entry
    // above remains the supported arithmetic/reference path. Publish caller
    // hitIgnore and preserve every completed write before a callback failure.
    template<class Events>
    bool hitByMachineGunChecked(int damage,MotionBinary64 direction,Events& events,bool& good){
        if(!good)return false;
        auto check=[&](bool okay){if(!okay)good=false;return good;};
        if(!check(damage==9||damage==15))return false;
        if(health<=0)return true;
        if(!check(events.clearHitIgnore())||!check(events.destroyBullet()))return false;
        hitPending=true;hitDirection=direction;
        MotionBinary64 angle;
        if(!check(events.impactAngle(angle)&&angle.valid()))return false;
        hitAngle=angle;
        if(!check(hitDamage<=2147483647-damage))return false;
        hitDamage+=damage;health-=damage;damageStun=true;
        if(!check(events.momentDelay())||!check(events.bulletHitSound(29,90,false,true)))return false;
        takenImpact=damage==9?MotionBinary64{0x3ffcccccu,0xcccccccdu}:MotionBinary64{0x40080000u,0};
        if(!check(events.impactEffect(605,MotionBinary64{0x3fd99999u,0x9999999au},direction,0)))return false;
        return hitStun||check(events.momentDelay());
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
        bool retire;
        if(!outOfViewRetirement(MotionBinary64::fromInteger(y),viewY,mainRoom,retire))return false;
        if(retire) { destroyRequested=true; events.destroy(); }
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
            // scrEnemyDeath includes levelBeginCue, which can synchronously
            // create object 372. Failure there must stop before blood/rewards.
            if(!events.enemyDeath()) return false;
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
    // Checked live Step. Builtin writes publish synchronously; effects complete
    // their child Creates before returning. Keep step()/stepAtPlayer() above as
    // the successful-event scalar reference, not the registered event adapter.
    template<class Publish,class Effects>
    bool stepPublished(bool stopped,MotionBinary64 playerX,MotionBinary64 playerY,
                       Publish& publish,Effects& effects,bool& good){
        if(!good)return false;
        const auto speed=[&](MotionBinary64 value){
            RunnerImageValue narrowed;return narrowed.assign(value)&&publish.imageSpeed(narrowed);
        };
        int comparison;
        if(!stopped){
            if(!compareRunnerReal(imageSpeed,rate,comparison)||(comparison&&!speed(rate)))return good=false;
            auto activation=yy;
            if(!activation.add(MotionBinary64::fromInteger(-96))||!compareRunnerReal(playerY,activation,comparison))return good=false;
            if(comparison>0)active=true;
            if(hitPending){
                active=true;hitStun=true;
                if(alarms[1]>2147483642||!publish.alarm(1,alarms[1]+5)||!publish.imageIndex({})||
                   !effects.hurtSound(133))return good=false;
                rate={};
                if(!publish.alarm(3,30))return good=false;
                hitPending=false;
            }
            if(!active){
                if(!publish.sprite(208))return good=false;
                rate={0x3fc99999u,0x9999999au};
            }else{
                if(sprite==208){
                    if(!publish.imageIndex({}))return good=false;
                    rate=throwRate();
                }
                if(!publish.sprite(207)||!compareRunnerReal(imageIndex,MotionBinary64::fromInteger(3),comparison))return good=false;
                if(comparison<0){
                    if(!compareRunnerReal(MotionBinary64::fromInteger(x),playerX,comparison)||
                       !publish.facing(comparison>0?-1:1))return good=false;
                }
                if(!compareRunnerReal(imageIndex,MotionBinary64::fromInteger(4),comparison))return good=false;
                if(comparison>0&&!shot){
                    if(!effects.throwBone(facing))return good=false;
                    shot=true;
                }
                if(!compareRunnerReal(imageIndex,MotionBinary64::fromInteger(6),comparison))return good=false;
                if(comparison>0){
                    if(!compareRunnerReal(rate,{},comparison))return good=false;
                    if(comparison){rate={};if(!publish.alarm(2,30))return good=false;}
                }
            }
            if(!compareRunnerReal(xsp,{},comparison))return good=false;
            if(comparison&&!publish.facing(comparison))return good=false;
            if((!grounded&&!xx.add(xsp))||!yy.add(ysp))return good=false;
            RunnerImageValue px,py;std::int32_t rounded;
            if(!px.assignInteger(x)||!py.assignInteger(y)||!xx.roundPixel(rounded)||!px.assignInteger(rounded)||
               !publish.position(px,py)||!yy.roundPixel(rounded)||!py.assignInteger(rounded)||!publish.position(px,py))return good=false;
        }else{
            if(!speed({}))return good=false;
            if(alarms[0]>0&&(alarms[0]==2147483647||!publish.alarm(0,alarms[0]+1)))return good=false;
        }
        if(health<=0)alive=false;
        if(!alive){
            if(!effects.enemyDeath()||!effects.blood(10,3)||!effects.smoke(xx,yy,5,1)||
               !effects.flash(xx,yy,6,2,10)||!effects.currency(money)||!effects.deathSound(132)||
               !effects.deadBody(206,takenImpact,102))return good=false;
            for(unsigned i=0;i<2;++i){
                if(y<(-2147483647-1)+5||!effects.debris(x,y-5))return good=false;
            }
            if(!publish.destroy())return good=false;
        }
        takenImpact={};
        if(hitStun&&(!publish.imageIndex({})||!publish.sprite(205)))return good=false;
        return true;
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
