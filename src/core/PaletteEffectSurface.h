#ifndef DOWNWELL_CORE_PALETTE_EFFECT_SURFACE_H
#define DOWNWELL_CORE_PALETTE_EFFECT_SURFACE_H
#include <cstdint>
namespace downwell {
// Binary alpha with the native two-bit palette. Painter-order masked replace,
// not OR of colors: opaque black (pen 0 or 3) also occludes previous effects.
// Ordinary-RAM scene scratch, never displayed or rebuilt as a DMA bitmap.
template<unsigned Width=160,unsigned Height=284> class PaletteEffectSurface {
    static_assert(Width && Width%16==0 && Width<=4096 && Height && Height<=4096,"bounded viewport required");
    static constexpr unsigned words=Width/16;
public:
    struct Mask {const std::uint8_t* data;unsigned bytes,width,height,stride;};
    struct Asset {Mask alpha;const std::uint8_t* low;const std::uint8_t* high;};
    PaletteEffectSurface(){for(unsigned y=0;y<Height;++y)first_[y]=words;}
    void clear(){
        for(unsigned y=0;y<Height;++y){
            for(unsigned x=first_[y];x<last_[y];++x){const unsigned at=y*words+x;alpha_[at]=low_[at]=high_[at]=0;}
            first_[y]=words;last_[y]=0;
        }
    }
    bool add(const Asset& asset,int x,int y){
        if(!asset.low || !asset.high)return false;
        return stamp(asset.alpha,asset.low,asset.high,x,y);
    }
    bool subtract(const Mask& mask,int x,int y){return stamp(mask,nullptr,nullptr,x,y);}
    template<class Emit> void visit(Emit emit)const{
        for(unsigned y=0;y<Height;++y)for(unsigned x=first_[y];x<last_[y];++x){
            const unsigned at=y*words+x;const auto a=alpha_[at];
            if(a)emit(x*16,y,a,std::uint16_t(low_[at]&a),std::uint16_t(high_[at]&a));
        }
    }
private:
    void put(int x,unsigned y,std::uint16_t a,std::uint16_t lo,std::uint16_t hi,bool subtract){
        if(!a || x<0 || unsigned(x)>=words)return;
        const unsigned at=y*words+unsigned(x);
        if(subtract){alpha_[at]&=std::uint16_t(~a);return;}
        alpha_[at]|=a;
        low_[at]=std::uint16_t((low_[at]&~a)|(lo&a));
        high_[at]=std::uint16_t((high_[at]&~a)|(hi&a));
        if(unsigned(x)<first_[y])first_[y]=unsigned(x);
        if(unsigned(x)+1>last_[y])last_[y]=unsigned(x)+1;
    }
    static std::uint16_t read(const std::uint8_t* row,unsigned sx,unsigned left){
        if(!row)return 0;
        unsigned value=unsigned(row[sx/8])<<8;
        if(left>8)value|=row[sx/8+1];
        if(left<16)value&=0xffffu<<(16-left);
        return std::uint16_t(value);
    }
    bool stamp(const Mask& mask,const std::uint8_t* lo,const std::uint8_t* hi,int x,int y){
        if(!mask.data || !mask.width || mask.width>65535 || !mask.height || mask.height>65535 ||
           mask.stride>8192 || mask.stride<(mask.width+7)/8 || mask.bytes<mask.stride*mask.height)return false;
        if(x>=int(Width) || y>=int(Height) || x<=-int(mask.width) || y<=-int(mask.height))return true;
        const bool subtract=!lo;
        int column=x/16,shift=x%16;if(shift<0){shift+=16;--column;}
        const unsigned begin=y<0?unsigned(-y):0;
        unsigned end=mask.height;if(end>unsigned(int(Height)-y))end=unsigned(int(Height)-y);
        for(unsigned sy=begin;sy<end;++sy){
            const unsigned dy=unsigned(y+int(sy)),offset=sy*mask.stride;
            if(subtract && first_[dy]==words)continue;
            for(unsigned sx=0;sx<mask.width;sx+=16){
                const unsigned left=mask.width-sx;
                const auto a=read(mask.data+offset,sx,left);
                const auto l=read(lo?lo+offset:nullptr,sx,left),h=read(hi?hi+offset:nullptr,sx,left);
                const int dx=column+int(sx/16);
                put(dx,dy,a>>shift,l>>shift,h>>shift,subtract);
                if(shift)put(dx+1,dy,std::uint16_t(unsigned(a)<<(16-shift)),
                    std::uint16_t(unsigned(l)<<(16-shift)),std::uint16_t(unsigned(h)<<(16-shift)),subtract);
            }
        }
        return true;
    }
    std::uint16_t alpha_[words*Height]{},low_[words*Height]{},high_[words*Height]{};
    std::uint16_t first_[Height]{},last_[Height]{};
};
}
#endif
