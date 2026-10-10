#ifndef DOWNWELL_CORE_RISING_TEXT_RENDERER_H
#define DOWNWELL_CORE_RISING_TEXT_RENDERER_H
#include "Renderer.h"
#include "LevelNumberSurfaceDraw.h"
#include <cstring>
namespace downwell {
// Original shop child text currently contains ASCII and '#'. Borrow the live
// font/alignment/color state; do not invent a font when the caller has none.
// Non-ASCII text and unsupported alpha/vertical alignment fail explicitly.
class RisingTextRenderer {
    const FontGlyphMasks& glyphs_;LevelNumberTextDrawState& state_;Renderer& renderer_;
    int viewX_,viewY_,width_,screenX_;
    char converted_[256]{};
    bool convert(const char* text){
        if(!text)return false;
        unsigned i=0;
        for(;text[i];++i){
            if(i==sizeof(converted_)-1||static_cast<unsigned char>(text[i])>127)return false;
            converted_[i]=text[i]=='#'?'\n':text[i];
        }
        converted_[i]=0;return true;
    }
    bool textAt(MotionBinary64 x,MotionBinary64 y){
        auto vx=MotionBinary64::fromInteger(viewX_),vy=MotionBinary64::fromInteger(viewY_);
        vx.high^=0x80000000u;vy.high^=0x80000000u;
        if(!x.add(MotionBinary64::fromInteger(screenX_))||!x.add(vx)||!y.add(vy))return false;
        RunnerImageValue px,py;if(!px.assign(x)||!py.assign(y))return false;
        float fx,fy;std::memcpy(&fx,&px.bits,4);std::memcpy(&fy,&py.bits,4);
        const auto pen=std::uint8_t(state_.color?255:0);
        renderer_.drawText(int(state_.font),converted_,fx,fy,{pen,pen,pen,255},
            state_.halign==1?TextAlign::center:state_.halign==2?TextAlign::right:TextAlign::left);
        return true;
    }
public:
    RisingTextRenderer(const FontGlyphMasks& glyphs,LevelNumberTextDrawState& state,Renderer& renderer,
        int viewX,int viewY,int width,int screenX):glyphs_(glyphs),state_(state),renderer_(renderer),
        viewX_(viewX),viewY_(viewY),width_(width),screenX_(screenX){}
    bool widthHashText(const char* text,MotionBinary64& result){
        if(!convert(text)||state_.valign||state_.halign>2||state_.alpha.high!=0x3ff00000u||state_.alpha.low)return false;
        int width=0,maximum=0;
        for(unsigned i=0;converted_[i];++i){
            const unsigned c=static_cast<unsigned char>(converted_[i]);
            if(c=='\n'||c=='\r'){if(width>maximum)maximum=width;width=0;continue;}
            int advance;if(!glyphs_.width(state_.font,c,advance)||advance < -32768||advance>32767)return false;
            width+=advance;
        }
        result=MotionBinary64::fromInteger(width>maximum?width:maximum);return true;
    }
    bool view(unsigned axis,MotionBinary64& value){
        if(axis!=0&&axis!=2)return false;
        value=MotionBinary64::fromInteger(axis==0?viewX_:width_);return true;
    }
    bool align(unsigned value){if(value>2)return false;state_.halign=value;return true;}
    bool borderTextBlack(const char* text,MotionBinary64 x,MotionBinary64 y){
        if(!convert(text))return false;
        state_.color=0;
        const int dx[]={1,-1,0,0},dy[]={0,0,1,-1};
        for(unsigned pass=0;pass<4;++pass){auto px=x,py=y;
            if(!px.add(MotionBinary64::fromInteger(dx[pass]))||!py.add(MotionBinary64::fromInteger(dy[pass]))||!textAt(px,py))return false;
        }
        state_.color=0xffffff;return textAt(x,y);
    }
};
}
#endif
