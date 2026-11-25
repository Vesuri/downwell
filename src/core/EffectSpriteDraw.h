#ifndef DOWNWELL_CORE_EFFECT_SPRITE_DRAW_H
#define DOWNWELL_CORE_EFFECT_SPRITE_DRAW_H
#include "EffectSurfacePlacement.h"
#include "RunnerImageValue.h"
namespace downwell {
// Shared drawFx routing for effects retaining default Y scale, angle, alpha
// and white blend. Preserve fractional inputs; backend sampling is separate.
struct EffectSpriteDraw {
    struct Command {
        EffectSurfacePlacement::Draw placement{};
        unsigned sprite=0;
        RunnerImageValue imageIndex{},scaleX{0x3f800000u};
    };
    // A binary32 coordinate that is a whole pixel inside smallPixel (smoke,
    // debris and corpse Steps publish rounded positions).
    static bool pixel(RunnerImageValue value,std::int32_t& out){
        const auto magnitude=value.bits&0x7fffffffu;
        if(!magnitude){out=0;return true;}
        const unsigned exponent=magnitude>>23;
        if(exponent<127||exponent>147)return false;
        const unsigned shift=150-exponent,significand=(magnitude&0x7fffffu)|0x800000u;
        if(significand&((1u<<shift)-1u))return false;
        out=std::int32_t(significand>>shift);
        if(value.bits>>31)out=-out;
        return true;
    }
    // fx for whole-pixel coordinates and an integer view, without binary64
    // values: false when either is not whole (use prepare). Otherwise
    // `shown` is fx's effects target, and the composition takes the pixels
    // through its whole-pixel path (the same rounding as fx's smallInteger
    // branch).
    static bool pixelFx(RunnerImageValue x,RunnerImageValue y,const EffectSurfacePlacement::View& view,
                        std::int32_t& px,std::int32_t& py,bool& shown){
        EffectSurfacePlacement::PixelView pixels;
        if(!pixel(x,px)||!pixel(y,py)||!EffectSurfacePlacement::pixelView(view,pixels))return false;
        shown=EffectSurfacePlacement::pixelInView(px,py,pixels);
        return true;
    }
    // Emitted coordinates as reals, for consumers that need them.
    static MotionBinary64 real(MotionBinary64 n){return n;}
    static MotionBinary64 real(std::int32_t n){return MotionBinary64::fromInteger(n);}
    static bool prepare(bool visible,unsigned sprite,RunnerImageValue index,
                        MotionBinary64 x,MotionBinary64 y,RunnerImageValue scaleX,
                        const EffectSurfacePlacement::View& view,
                        bool death,bool lowSpec,bool surfaceExists,Command& out){
        Command next;
        if(visible){
            if(!EffectSurfacePlacement::fx(x,y,view,death,lowSpec,surfaceExists,next.placement))return false;
            if(next.placement.target!=EffectSurfacePlacement::hidden){
                next.sprite=sprite;next.imageIndex=index;next.scaleX=scaleX;
            }
        }
        out=next;return true;
    }
};
}
#endif
