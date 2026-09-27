#ifndef DOWNWELL_CORE_MACHINE_GUN_SOURCE_STEP_H
#define DOWNWELL_CORE_MACHINE_GUN_SOURCE_STEP_H
#include "MachineGunSourceMotion.h"
#include "BulletSolidScript.h"
namespace downwell {
// bulletStandard Step for the two usual-gun birth profiles. Scheduling,
// collision events and engine cleanup remain the world owner's responsibility.
// Ops' selectors/approach must use this same absolute-coordinate motion owner;
// its destroySelf marks destruction, never erases the executing instance.
struct MachineGunSourceStep {
    template<class Random,class Effects,class Ops>
    static bool run(MachineGunSourceMotion& motion,Random& random,bool gemHigh,
                    Effects& effects,Ops& ops,bool& succeeded,int hitWallFx=101){
        if(!succeeded)return false;
        auto check=[&](bool ok){if(!ok)succeeded=false;return ok;};
        if(!check(motion.initializeWithEffects(random,
            [&](RunnerImageValue angle){return ops.publishAngle(angle);},gemHigh,effects)))return false;
        const auto profile=machineGunProfile(gemHigh);
        const BulletSolidScript::Settings settings{MotionBinary64::fromInteger(profile.damage),
            motion.direction,hitWallFx,profile.consistent,profile.wave,profile.explode};
        if(!BulletSolidScript::run(settings,ops,succeeded))return false;
        // instance_destroy does not return from the VM event. Both builtin
        // setters still execute, individually, after solid-contact effects.
        return check(motion.commitPosition([&](RunnerImageValue x,RunnerImageValue y){
            return ops.publishPosition(x,y);
        }));
    }
};
}
#endif
