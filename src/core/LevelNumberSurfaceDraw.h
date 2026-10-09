#ifndef DOWNWELL_CORE_LEVEL_NUMBER_SURFACE_DRAW_H
#define DOWNWELL_CORE_LEVEL_NUMBER_SURFACE_DRAW_H
#include "FontGlyphMasks.h"
#include "MotionBinary64.h"
#include "RunnerImageValue.h"
namespace downwell {
// Borrow current renderer state, rather than resetting it on entry to title
// Draw: the original event sets only color and the surface target. The admitted
// binary-mask path requires full opacity. Unsupported
// state fails explicitly until a corresponding original-renderer path exists.
struct LevelNumberTextDrawState {
    unsigned font,halign,valign,color;
    MotionBinary64 alpha;
};
template<class Target,class Emit> class LevelNumberSurfaceDraw {
public:
    LevelNumberSurfaceDraw(const FontGlyphMasks& glyphs,LevelNumberTextDrawState& state,
                           Target& target,Emit& emit):glyphs_(glyphs),state_(state),target_(target),emit_(emit){}
    bool exists(bool& value){return target_.exists(value);}
    bool begin(){return target_.begin();}
    bool end(){return target_.end();}
    bool view(unsigned axis,MotionBinary64& value){return target_.view(axis,value);}
    bool color(unsigned value){state_.color=value;return true;}
    bool character(unsigned token,MotionBinary64 x,MotionBinary64 y){
        if(state_.halign>2||state_.valign>2||state_.alpha.high!=0x3ff00000u||state_.alpha.low)return false;
        // Each source draw_text call receives just one string_char_at token.
        // Hash-to-newline, CR/LF and NUL therefore contain no glyph ink.
        if(!token||token=='#'||token=='\r'||token=='\n')return true;
        RunnerImageValue px,py;
        if(!px.assign(x)||!py.assign(y))return false;
        x=px.promoted();y=py.promoted();
        if(state_.valign){
            int height;if(!glyphs_.lineHeight(state_.font,height))return false;
            const int shift=state_.valign==1?height/2:height;
            if(!y.add(MotionBinary64::fromInteger(-shift))||!py.assign(y))return false;
            y=py.promoted();
        }
        if(state_.halign){
            int width;if(!glyphs_.width(state_.font,token,width))return false;
            const int shift=state_.halign==1?width/2:width;
            if(!x.add(MotionBinary64::fromInteger(-shift))||!px.assign(x))return false;
            x=px.promoted();
        }
        EffectMaskAssets::Asset asset;bool ink;
        if(!glyphs_.resolve(state_.font,token,state_.color,asset,ink))return false;
        if(!ink)return true;
        // The original atlas renderer adds the signed glyph bearing before
        // sampling, with binary32 rounding. Sprite-style integer origins in
        // the compositor would apply it after sampling and change half ties.
        if(!x.add(MotionBinary64::fromInteger(-asset.originX))||!px.assign(x))return false;
        asset.originX=0;
        return emit_.atSurface(asset,px.promoted(),y);
    }
private:
    const FontGlyphMasks& glyphs_;LevelNumberTextDrawState& state_;Target& target_;Emit& emit_;
};
}
#endif
