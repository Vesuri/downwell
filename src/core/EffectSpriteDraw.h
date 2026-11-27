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
    // The nearest pixel (halves to even, as roundPixel) of a binary32
    // coordinate inside smallPixel; false outside it or for NaN/infinity.
    static bool roundedPixel(RunnerImageValue value,std::int32_t& out){
        const auto magnitude=value.bits&0x7fffffffu;
        const unsigned exponent=magnitude>>23;
        if(exponent>147)return false;
        if(exponent<126){out=0;return true;} // below one half, or zero
        const unsigned shift=150-exponent,significand=(magnitude&0x7fffffu)|0x800000u;
        std::uint32_t whole=significand>>shift;
        if(shift<=23){
            const std::uint32_t rest=significand&((1u<<shift)-1u),half=1u<<(shift-1);
            if(rest>half||(rest==half&&(whole&1u)))++whole;
        }else if(significand>0x800000u)++whole; // exponent 126: (0.5, 1) rounds up, 0.5 to 0
        out=std::int32_t(whole);
        if(value.bits>>31)out=-out;
        return true;
    }
    // fx without Q16 values, for an integer view: false when the view
    // or a coordinate is outside smallPixel (use prepare). Otherwise
    // `shown` is the effects target. Whole pixels match fx exactly
    // (its smallInteger branch); fractional coordinates are rounded first
    // (W3 fixed point accepts the pixel this moves: visibility at a view
    // edge, and halves when the view coordinate is odd).
    static bool pixelFx(RunnerImageValue x,RunnerImageValue y,const EffectSurfacePlacement::View& view,
                        std::int32_t& px,std::int32_t& py,bool& shown){
        EffectSurfacePlacement::PixelView pixels;
        if(!roundedPixel(x,px)||!roundedPixel(y,py)||!EffectSurfacePlacement::pixelView(view,pixels))return false;
        shown=EffectSurfacePlacement::pixelInView(px,py,pixels);
        return true;
    }
    // Emitted coordinates as Q16, for consumers that need them.
    static Fixed16 real(Fixed16 n){return n;}
    static Fixed16 real(std::int32_t n){return Fixed16::fromInteger(n);}
    // A binary32 coordinate as Q16 (false for NaN, infinity or out of range).
    static bool real(RunnerImageValue n,Fixed16& out){return Fixed16::fromBinary32(n.bits,out);}
    static bool prepare(bool visible,unsigned sprite,RunnerImageValue index,
                        Fixed16 x,Fixed16 y,RunnerImageValue scaleX,
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
    static bool prepare(bool visible,unsigned sprite,RunnerImageValue index,
                        RunnerImageValue x,RunnerImageValue y,RunnerImageValue scaleX,
                        const EffectSurfacePlacement::View& view,
                        bool death,bool lowSpec,bool surfaceExists,Command& out){
        Fixed16 fx,fy;
        return real(x,fx)&&real(y,fy)&&prepare(visible,sprite,index,fx,fy,scaleX,view,death,lowSpec,surfaceExists,out);
    }
    // emit(asset,x,y) at a binary32 position as Q16.
    template<class Emit,class Asset> static bool emitAt(Emit& emit,const Asset& asset,RunnerImageValue x,RunnerImageValue y){
        Fixed16 fx,fy;return real(x,fx)&&real(y,fy)&&emit(asset,fx,fy);
    }
};
}
#endif
