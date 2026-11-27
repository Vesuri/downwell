#ifndef DOWNWELL_CORE_EFFECT_SURFACE_PLACEMENT_H
#define DOWNWELL_CORE_EFFECT_SURFACE_PLACEMENT_H
#include "FixedPoint.h"
namespace downwell {
// drawFx/drawShade placement only. Surface lifetime, draw ordering and GPU
// subtract/alpha composition remain the renderer's responsibility.
// W3.20: coordinates are Q16. The runner compares reals with a 1e-5
// equality band; a Q16 step (1/65536) is wider than the band, so strict
// Q16 comparisons are that comparison exactly for Q16 inputs.
struct EffectSurfacePlacement {
    using Real=Fixed16;
    enum Target { hidden, direct, effects, shade };
    struct View { Real x,y,width,height; };
    struct Draw { Target target=hidden; Real x{},y{}; };
    static bool inView(Real x,Real y,const View& view,bool& visible) {
        visible=x>view.x && x<view.x+view.width && y>view.y && y<view.y+view.height;
        return true;
    }
    static bool relative(Real world,Real camera,Real& out) {out=world-camera;return true;}
    // round(world / 2) * 2, halves to even: odd whole pixels go to the
    // multiple of four (an even half), not always up or down.
    static Real snapShade(Real world) {
        std::int32_t half=(world.v+0x10000)>>17;
        if((world.v&0x1ffff)==0x10000 && (half&1))--half;
        return Real::fromInteger(half*2);
    }
    static bool snapShade(Real world,Real& out) {out=snapShade(world);return true;}
    static bool fx(Real x,Real y,const View& view,
                   bool death,bool lowSpec,bool surfaceExists,Draw& out) {
        Draw next;
        if(death || lowSpec)next={direct,x,y};
        else {
            bool visible;inView(x,y,view,visible);
            if(visible && surfaceExists)next={effects,x-view.x,y-view.y};
        }
        out=next;return true;
    }
    // drawOnSurface: unlike drawFx, no origin visibility or death/lowSpec gate.
    // A missing surface produces no draw, not a direct-render fallback.
    static bool onSurface(Real x,Real y,const View& view,
                          bool surfaceExists,Draw& out){
        Draw next;
        if(surfaceExists)next={effects,x-view.x,y-view.y};
        out=next;return true;
    }
    static bool enemyShade(Real x,Real y,const View& view,
                           bool lowSpec,bool noControl,bool surfaceExists,Draw& out) {
        Draw next;
        if(!lowSpec){
            bool visible;inView(x,y,view,visible);
            if(visible && !noControl && surfaceExists)
                next={shade,snapShade(x)-view.x,snapShade(y)-view.y};
        }
        out=next;return true;
    }
    // Whole-pixel producers (gem and enemy positions): the same placement
    // without Q16 values, for whole pixels inside smallPixel.
    struct PixelView { std::int32_t x,y,width,height; };
    static bool pixelView(const View& view,PixelView& out){
        if((view.x.v|view.y.v|view.width.v|view.height.v)&0xffff)return false;
        out={view.x.v>>16,view.y.v>>16,view.width.v>>16,view.height.v>>16};
        return true;
    }
    static bool smallPixel(std::int32_t n){return n>-(1<<14)&&n<(1<<14);}
    static bool pixelInView(std::int32_t x,std::int32_t y,const PixelView& v){
        return x>v.x && x<v.x+v.width && y>v.y && y<v.y+v.height;
    }
    static std::int32_t snapShadePixel(std::int32_t integer){
        unsigned magnitude=static_cast<unsigned>(integer<0?-integer:integer);
        if(magnitude&1u)magnitude=(magnitude+1u)&~3u;
        return integer<0?-static_cast<std::int32_t>(magnitude):static_cast<std::int32_t>(magnitude);
    }
};
}
#endif
