#ifndef DOWNWELL_CORE_BINARY_EFFECT_SURFACE_H
#define DOWNWELL_CORE_BINARY_EFFECT_SURFACE_H
#include <cstdint>
#include "EffectMask.h"
namespace downwell {
// Opaque/binary-alpha drawFx subset: union smoke coverage, then subtract shade.
// Mask data is offline MSB-first planar bytes, not converted texture pixels.
// Ordinary RAM scratch, not a display plane; owners should not put this on the
// AmigaDOS task stack. Neither scrolling display ownership nor RNG lives here.
template<unsigned Width=160,unsigned Height=284> class BinaryEffectSurface {
    static_assert(Width>0 && Width%16==0 && Width<=4096 && Height>0 && Height<=4096,
                  "bounded word-aligned effect viewport required");
    static constexpr unsigned words=Width/16;
public:
    using Mask=EffectMask;
    BinaryEffectSurface(){for(unsigned y=0;y<Height;++y)first_[y]=words;}
    void clear(){
        if(!dirty_)return;
        for(unsigned y=0;y<Height;++y){
            for(unsigned x=first_[y];x<last_[y];++x)coverage_[y*words+x]=0;
            first_[y]=words;last_[y]=0;
        }
        dirty_=false;
    }
    // All effect additions must precede shade subtraction in a logical Draw.
    bool add(const Mask& mask,int x,int y){return stamp(mask,x,y,false);}
    bool subtract(const Mask& mask,int x,int y){return stamp(mask,x,y,true);}
    template<class Emit> void visit(Emit emit)const{
        if(!dirty_)return;
        for(unsigned y=0;y<Height;++y)
            for(unsigned x=first_[y];x<last_[y];++x){
                const auto value=coverage_[y*words+x];
                if(value)emit(x*16,y,value);
            }
    }
private:
    void put(int x,unsigned y,std::uint16_t bits,bool subtract){
        if(!bits || x<0 || unsigned(x)>=words)return;
        auto& value=coverage_[y*words+unsigned(x)];
        if(subtract){value&=std::uint16_t(~bits);return;}
        value|=bits;dirty_=true;
        if(unsigned(x)<first_[y])first_[y]=unsigned(x);
        if(unsigned(x)+1>last_[y])last_[y]=unsigned(x)+1;
    }
    bool stamp(const Mask& mask,int x,int y,bool subtract){
        if(!mask.data || !mask.width || mask.width>65535 || !mask.height || mask.height>65535 ||
           mask.stride>8192 || mask.stride<(mask.width+7)/8 ||
           mask.bytes<mask.stride*mask.height)return false;
        if((subtract&&!dirty_) || x>=int(Width) || y>=int(Height) ||
           x<=-int(mask.width) || y<=-int(mask.height))return true;
        int column=x/16,shift=x%16;
        if(shift<0){shift+=16;--column;}
        const unsigned begin=y<0?unsigned(-y):0;
        unsigned end=mask.height;
        if(int(end)>int(Height)-y)end=unsigned(int(Height)-y);
        for(unsigned sy=begin;sy<end;++sy){
            const unsigned dy=unsigned(y+int(sy));
            if(subtract && first_[dy]==words)continue;
            const auto* row=mask.data+sy*mask.stride;
            for(unsigned sx=0;sx<mask.width;sx+=16){
                const unsigned left=mask.width-sx;
                std::uint16_t bits=std::uint16_t(row[sx/8])<<8;
                if(left>8)bits|=row[sx/8+1];
                if(left<16)bits&=std::uint16_t(0xffffu<<(16-left));
                const int dx=column+int(sx/16);
                put(dx,dy,std::uint16_t(bits>>shift),subtract);
                if(shift)put(dx+1,dy,std::uint16_t(unsigned(bits)<<(16-shift)),subtract);
            }
        }
        return true;
    }
    std::uint16_t coverage_[words*Height]{};
    std::uint16_t first_[Height]{},last_[Height]{};
    bool dirty_=false;
};
}
#endif
