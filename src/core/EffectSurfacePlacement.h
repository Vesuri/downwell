#ifndef DOWNWELL_CORE_EFFECT_SURFACE_PLACEMENT_H
#define DOWNWELL_CORE_EFFECT_SURFACE_PLACEMENT_H
#include "RunnerRealComparison.h"
#include "MultiplyBinary64.h"
namespace downwell {
// drawFx/drawShade placement only. Surface lifetime, draw ordering and GPU
// subtract/alpha composition remain the renderer's responsibility.
struct EffectSurfacePlacement {
    enum Target { hidden, direct, effects, shade };
    struct View { MotionBinary64 x,y,width,height; };
    struct Draw { Target target=hidden; MotionBinary64 x{},y{}; };
    static bool inView(MotionBinary64 x,MotionBinary64 y,const View& view,bool& visible) {
        auto right=view.x,bottom=view.y;int l,r,t,b;
        if(!right.add(view.width) || !bottom.add(view.height) ||
           !compareRunnerReal(x,view.x,l) || !compareRunnerReal(x,right,r) ||
           !compareRunnerReal(y,view.y,t) || !compareRunnerReal(y,bottom,b))return false;
        visible=l>0 && r<0 && t>0 && b<0;return true;
    }
    static bool relative(MotionBinary64 world,MotionBinary64 camera,MotionBinary64& out) {
        camera.high^=0x80000000u;
        if(!world.add(camera))return false;
        out=world;return true;
    }
    static bool snapShade(MotionBinary64 world,MotionBinary64& out) {
        MotionBinary64 half;std::int32_t rounded;
        if(!multiplyBinary64(world,{0x3fe00000u,0},half) || !half.roundPixel(rounded) ||
           rounded < -1073741824 || rounded > 1073741823)return false;
        out=MotionBinary64::fromInteger(rounded*2);return true;
    }
    static bool fx(MotionBinary64 x,MotionBinary64 y,const View& view,
                   bool death,bool lowSpec,bool surfaceExists,Draw& out) {
        Draw next;
        if(death || lowSpec)next={direct,x,y};
        else {
            bool visible;
            if(!inView(x,y,view,visible))return false;
            if(visible && surfaceExists){
                next.target=effects;
                if(!relative(x,view.x,next.x) || !relative(y,view.y,next.y))return false;
            }
        }
        out=next;return true;
    }
    static bool enemyShade(MotionBinary64 x,MotionBinary64 y,const View& view,
                           bool lowSpec,bool noControl,bool surfaceExists,Draw& out) {
        Draw next;
        if(!lowSpec){
            bool visible;
            if(!inView(x,y,view,visible))return false;
            if(visible && !noControl && surfaceExists){
                MotionBinary64 sx,sy;
                next.target=shade;
                if(!snapShade(x,sx) || !snapShade(y,sy) ||
                   !relative(sx,view.x,next.x) || !relative(sy,view.y,next.y))return false;
            }
        }
        out=next;return true;
    }
};
}
#endif
