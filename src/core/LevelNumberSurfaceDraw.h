#ifndef DOWNWELL_CORE_LEVEL_NUMBER_SURFACE_DRAW_H
#define DOWNWELL_CORE_LEVEL_NUMBER_SURFACE_DRAW_H
#include "FontGlyphMasks.h"
#include "FixedPoint.h"
namespace downwell {
// Borrow current renderer state, rather than resetting it on entry to title
// Draw: the original event sets only color and the surface target. The admitted
// binary-mask path requires full opacity. Unsupported
// state fails explicitly until a corresponding original-renderer path exists.
struct LevelNumberTextDrawState {
    unsigned font,halign,valign,color;
    Fixed16 alpha;
};
template<class Target,class Emit> class LevelNumberSurfaceDraw {
public:
    LevelNumberSurfaceDraw(const FontGlyphMasks& glyphs,LevelNumberTextDrawState& state,
                           Target& target,Emit& emit):glyphs_(glyphs),state_(state),target_(target),emit_(emit){}
    bool exists(bool& value){return target_.exists(value);}
    bool begin(){return target_.begin();}
    bool end(){return target_.end();}
    bool view(unsigned axis,Fixed16& value){return target_.view(axis,value);}
    bool color(unsigned value){state_.color=value;return true;}
    // W3.20: every input is a whole pixel (views, offsets, widths), so the
    // binary32 rounding the atlas renderer applies is exact and drops out.
    bool character(unsigned token,Fixed16 x,Fixed16 y){
        if(state_.halign>2||state_.valign>2||state_.alpha!=Fixed16::fromInteger(1))return false;
        // Each source draw_text call receives just one string_char_at token.
        // Hash-to-newline, CR/LF and NUL therefore contain no glyph ink.
        if(!token||token=='#'||token=='\r'||token=='\n')return true;
        if(state_.valign){
            int height;if(!glyphs_.lineHeight(state_.font,height))return false;
            y=y-Fixed16::fromInteger(state_.valign==1?height/2:height);
        }
        if(state_.halign){
            int width;if(!glyphs_.width(state_.font,token,width))return false;
            x=x-Fixed16::fromInteger(state_.halign==1?width/2:width);
        }
        EffectMaskAssets::Asset asset;bool ink;
        if(!glyphs_.resolve(state_.font,token,state_.color,asset,ink))return false;
        if(!ink)return true;
        // The original atlas renderer adds the signed glyph bearing before
        // sampling. Sprite-style integer origins in the compositor would
        // apply it after sampling and change half ties.
        x=x-Fixed16::fromInteger(asset.originX);
        asset.originX=0;
        return emit_.atSurface(asset,x,y);
    }
private:
    const FontGlyphMasks& glyphs_;LevelNumberTextDrawState& state_;Target& target_;Emit& emit_;
};
}
#endif
