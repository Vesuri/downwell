#ifndef DOWNWELL_CORE_BULLET_SOLID_SCRIPT_H
#define DOWNWELL_CORE_BULLET_SOLID_SCRIPT_H
#include "MotionBinary64.h"
namespace downwell {
// scrBulCheckSolid, excluding the outer bulletStandard Step's final X/Y writes.
// Ops retains thatWall/thatThing identities, performs original query rounding,
// delegates the approach to MachineGunSourceMotion, and publishes each write.
// Destruction is a request: explosion creation can still follow it. Callbacks
// must resolve retained handles anew rather than holding pointers across Create.
struct BulletSolidScript {
    struct Settings {
        MotionBinary64 damage{},direction{};
        int hitWallFx=-1;
        bool consistent=false,wave=false,explode=false;
    };
    template<class Ops> static bool run(const Settings& settings,Ops& ops,bool& succeeded){
        if(!succeeded)return false;
        auto check=[&](bool ok){if(!ok)succeeded=false;return ok;};
        auto positive=[](MotionBinary64 n){return !n.zero()&&!(n.high&0x80000000u);};
        auto damage=[&](MotionBinary64& hp){
            auto delta=settings.damage;delta.high^=0x80000000u;
            return check(hp.add(delta));
        };
        bool wall=false,enemy=false;
        if(!check(ops.line(59,wall)))return false;
        if(wall&&!check(ops.line(83,enemy)))return false;
        if(wall&&!enemy){
            // The second wall line call is observable: do not cache the first
            // selection or replace it with a nearest-hit result.
            if(!check(ops.selectWall())||!check(ops.approachSolid()))return false;
            int parent=-1;
            if(!check(ops.wallParent(parent)))return false;
            if(parent==84){ // Direct parent, not ancestor membership.
                MotionBinary64 hp;
                if(!check(ops.wallHealth(hp)&&hp.valid()))return false;
                if(positive(hp)){
                    if(!damage(hp)||!check(ops.wallHealthWrite(hp))||!check(ops.wallHit()))return false;
                    if(!settings.consistent&&!check(ops.destroySelf()))return false;
                }
            }else if(!settings.wave){
                if(!check(ops.effect(settings.hitWallFx,{0x3fe00000u,0},{}))||
                   !check(ops.soundWall())||!check(ops.destroySelf()))return false;
            }
            if(settings.explode&&!settings.wave&&!check(ops.explosion(272)))return false;
        }else{
            bool thing=false;
            if(!check(ops.placeShootable(90,thing)))return false;
            if(thing){
                if(!check(ops.selectShootable(90)))return false;
                MotionBinary64 hp;
                if(!check(ops.thingHealth(hp)&&hp.valid()))return false;
                if(positive(hp)){
                    if(!damage(hp)||!check(ops.thingHealthWrite(hp))||!check(ops.thingHit()))return false;
                    bool effect=false;
                    if(!check(ops.thingHitEffect(effect)))return false;
                    if(effect&&!check(ops.effect(605,{0x3fd99999u,0x9999999au},settings.direction)))return false;
                    if(!settings.consistent&&!check(ops.destroySelf()))return false;
                }
            }
        }
        return true;
    }
};
}
#endif
