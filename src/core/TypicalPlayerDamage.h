#ifndef DOWNWELL_CORE_TYPICAL_PLAYER_DAMAGE_H
#define DOWNWELL_CORE_TYPICAL_PLAYER_DAMAGE_H
#include "MotionBinary64.h"
namespace downwell {
// scrTypicalDamage's CALLER variables, not player or run-global storage.
// Events supplies synchronous scrPDamage/scrFjump/effect calls and source
// point_direction/lengthdir_x math. Its player iterator must use the original
// object-35 traversal and published x/y, never global plx/ply or private xx/yy.
// Callbacks return false on unsupported behavior; the world must latch failure
// and must not retry a partially executed script. They may mutate globals.
struct TypicalPlayerDamage {
    MotionBinary64 damage{},xImpact{},yImpact{},direction{};
    template<class Events,class CreateField>
    bool execute(MotionBinary64 amount,MotionBinary64 horizontal,MotionBinary64 vertical,
                 Events& events,CreateField createField){
        damage=amount;xImpact=horizontal;yImpact=vertical;
        if(events.playerDamaged())return true;
        // Read in VM evaluation order. scrPDamage may move the player or
        // mutate the caller; do not recompute pdir after that synchronous call.
        const auto py=events.playerY(),px=events.playerX();
        const auto cy=events.callerY(),cx=events.callerX();
        if(!events.pointDirection(cx,cy,px,py,direction) || !events.damagePlayer(damage))return false;
        const auto jumpY=yImpact;
        MotionBinary64 jumpX;
        if(!events.lengthdirX(xImpact,direction,jumpX) || !events.forcedJump(jumpX,jumpY))return false;
        if(!events.playerDead() && !events.shake(5,8))return false;
        if(events.playerHealthPositive()){
            const auto effectY=events.playerY(),effectX=events.playerX();
            if(!events.hitEffect(effectX,effectY,114,MotionBinary64{0x3fd99999u,0x9999999au},0,-100000))return false;
        }
        // This is outside both death and surviving-health conditions. Do not
        // snapshot pTimeStop: every with iteration reads the current global.
        if(events.timeoutOwned() && !events.forEachPlayer([&](auto& player){
            if(events.timeStopped())return true;
            const auto y=player.publishedY(),x=player.publishedX();
            return createField(x,y);
        }))return false;
        events.clearNoDamage();
        events.clearAreaNoDamage();
        return true;
    }
};
}
#endif
