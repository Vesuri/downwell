#include "core/CompactRoomEffects.h"
#include "core/GemLifetime.h"
#include <cassert>
#include <cstdio>
// W1.5 compact time-stop pieces below the world: objCurrency's stopped
// branch and parentMovingFx's TimeStopBound. The slice route
// (AmigaSceneRuntimeTest) covers fields, shards and frozen enemies in place.
using downwell::MotionBinary64;
int main(){
    {
        // Stopped outside a field: alarms held, obtainable forced at once.
        downwell::GemLifetime gem;
        assert(!gem.obtainable());
        gem.freeze();
        assert(gem.obtainable()&&gem.age==0&&gem.visible());
    }
    {
        // A disappearing gem keeps toggling dflash while its age is held.
        downwell::GemLifetime gem;
        while(!gem.disappearing())assert(gem.advance(false));
        const unsigned age=gem.age;const bool shown=gem.visible();
        gem.freeze();assert(gem.age==age&&gem.visible()!=shown);
        gem.freeze();assert(gem.age==age&&gem.visible()==shown);
        assert(gem.advance(false)&&gem.visible()!=shown); // and resumes from there
    }
    {
        // parentMovingFx: x/y advance only while TimeStopBound holds;
        // the animation is the runner's and continues either way.
        downwell::CompactRoomEffects<4,4> fx;
        assert(fx.spawnMoving(MotionBinary64::fromInteger(100),MotionBinary64::fromInteger(50),84,
            {0x3fe00000u,0},MotionBinary64::fromInteger(0),MotionBinary64::fromInteger(1)));
        const auto x0=fx.moving(0).effect.x16;
        bool asked=false;
        assert(fx.stepMoving(fx.beginPhase(),[&](const downwell::CompactRoomEffects<4,4>::Effect& e,bool& moving){
            asked=e.spriteId==84;moving=false;return true;}));
        assert(asked&&fx.moving(0).effect.x16==x0);
        assert(fx.stepMoving(fx.beginPhase()));
        assert(fx.moving(0).effect.x16!=x0);
        // A failed bound query is terminal for the store.
        assert(!fx.stepMoving(fx.beginPhase(),[](const downwell::CompactRoomEffects<4,4>::Effect&,bool&){return false;})&&!fx.valid());
    }
    std::puts("Compact time-stop tests passed");
}
