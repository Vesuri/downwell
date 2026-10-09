#ifndef DOWNWELL_CORE_FONT_GLYPH_MASKS_H
#define DOWNWELL_CORE_FONT_GLYPH_MASKS_H
#include "EffectMaskAssets.h"
namespace downwell {
// Offline, binary-alpha glyphs from a single original font resource. The
// backing arrays are immutable ordinary memory; no target-side conversion.
struct FontGlyphMask {
    unsigned character;
    BinaryEffectSurface<>::Mask mask;
    int offset;
};
class FontGlyphMasks {
public:
    FontGlyphMasks(unsigned font,const FontGlyphMask* glyphs,unsigned count,
                   const std::uint8_t* zero,unsigned zeroBytes):
        font_(font),glyphs_(glyphs),count_(count),zero_(zero),zeroBytes_(zeroBytes){}
    // Missing characters use the runner's U+25AF fallback, then no ink.
    // Color black remains opaque; it must not subtract surface coverage.
    bool resolve(unsigned font,unsigned character,unsigned color,EffectMaskAssets::Asset& asset,bool& ink)const{
        ink=false;
        if(font!=font_||!glyphs_||!zero_||(color!=0&&color!=0xffffff))return false;
        const auto* glyph=find(character);
        if(!glyph)glyph=find(0x25af);
        if(!glyph||!glyph->mask.width||!glyph->mask.height)return true;
        if(!glyph->mask.data||glyph->mask.bytes>zeroBytes_)return false;
        asset={glyph->mask,-glyph->offset,0,glyph->mask.data,color?zero_:glyph->mask.data};
        ink=true;return true;
    }
private:
    const FontGlyphMask* find(unsigned character)const{
        unsigned first=0,last=count_;
        while(first<last){const unsigned middle=first+(last-first)/2;
            if(glyphs_[middle].character<character)first=middle+1;else last=middle;
        }
        return first<count_&&glyphs_[first].character==character?glyphs_+first:nullptr;
    }
    unsigned font_;const FontGlyphMask* glyphs_;unsigned count_;
    const std::uint8_t* zero_;unsigned zeroBytes_;
};
}
#endif
