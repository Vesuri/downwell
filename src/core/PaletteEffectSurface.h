#ifndef DOWNWELL_CORE_PALETTE_EFFECT_SURFACE_H
#define DOWNWELL_CORE_PALETTE_EFFECT_SURFACE_H
#include <cstdint>
namespace downwell {
// Binary alpha with the native two-bit palette. Painter-order masked replace,
// not OR of colors: opaque black (pen 0 or 3) also occludes previous effects.
// CPU-owned scratch by default; a platform can bind guarded planar storage
// and queued operations without changing producer order or alpha semantics.
template<unsigned Width=160,unsigned Height=284> class PaletteEffectSurface {
    static_assert(Width && Width%16==0 && Width<=4096 && Height && Height<=4096,"bounded viewport required");
    static constexpr unsigned words=Width/16;
public:
    struct Mask {const std::uint8_t* data;unsigned bytes,width,height,stride;};
    struct Asset {Mask alpha;const std::uint8_t* low;const std::uint8_t* high;};
    // Optional platform-owned planar storage and operations. A declined stamp
    // must not modify storage. All CPU reads/writes synchronize queued work.
    struct Backend {
        void* context=nullptr;
        bool (*stamp)(void*,const Mask&,const std::uint8_t*,const std::uint8_t*,int,int)=nullptr;
        void (*clear)(void*,unsigned,unsigned)=nullptr;
        void (*wait)(void*)=nullptr;
    };
    PaletteEffectSurface(){for(unsigned y=0;y<Height;++y)first_[y]=words;}
    PaletteEffectSurface(const PaletteEffectSurface&)=delete;
    PaletteEffectSurface& operator=(const PaletteEffectSurface&)=delete;
    bool bindStorage(std::uint16_t* alpha,std::uint16_t* low,std::uint16_t* high,unsigned stride,Backend backend){
        if(!alpha||!low||!high||stride<words||!backend.stamp||!backend.clear||!backend.wait||lastRow_)return false;
        synchronize();
        alpha_=alpha;low_=low;high_=high;stride_=stride;backend_=backend;
        for(unsigned y=0;y<Height;++y)for(unsigned x=0;x<words;++x)
            alpha_[y*stride_+x]=low_[y*stride_+x]=high_[y*stride_+x]=0;
        return true;
    }
    void clear(){
        if(backend_.clear)backend_.clear(backend_.context,firstRow_,lastRow_);
        for(unsigned y=firstRow_;y<lastRow_;++y){
            if(!backend_.clear)for(unsigned x=first_[y];x<last_[y];++x){const unsigned at=y*stride_+x;alpha_[at]=low_[at]=high_[at]=0;}
            first_[y]=words;last_[y]=0;
        }
        firstRow_=Height;lastRow_=0;
    }
    bool add(const Asset& asset,int x,int y){
        if(!asset.low || !asset.high)return false;
        return stamp(asset.alpha,asset.low,asset.high,x,y);
    }
    bool subtract(const Mask& mask,int x,int y){return stamp(mask,nullptr,nullptr,x,y);}
    // Half-open viewport rectangle. Test final alpha, including shade holes
    // and opaque black; stale color bits are never evidence of overlap.
    bool intersects(int left,int top,int right,int bottom)const{
        synchronize();
        if(left<0)left=0;
        if(top<0)top=0;
        if(right>int(Width))right=Width;
        if(bottom>int(Height))bottom=Height;
        if(top<int(firstRow_))top=firstRow_;
        if(bottom>int(lastRow_))bottom=lastRow_;
        if(left>=right || top>=bottom)return false;
        const unsigned first=unsigned(left)/16,last=unsigned(right-1)/16;
        for(unsigned y=unsigned(top);y<unsigned(bottom);++y){
            unsigned begin=first>first_[y]?first:first_[y];
            unsigned end=last+1<last_[y]?last+1:last_[y];
            for(unsigned x=begin;x<end;++x){
                unsigned mask=0xffffu;
                if(x==first)mask&=0xffffu>>(unsigned(left)%16);
                if(x==last && unsigned(right)%16)mask&=0xffffu<<(16-unsigned(right)%16);
                if(alpha_[y*stride_+x]&mask)return true;
            }
        }
        return false;
    }
    template<class Emit> void visit(Emit emit)const{
        synchronize();
        for(unsigned y=firstRow_;y<lastRow_;++y)for(unsigned x=first_[y];x<last_[y];++x){
            const unsigned at=y*stride_+x;const auto a=alpha_[at];
            if(a)emit(x*16,y,a,std::uint16_t(low_[at]&a),std::uint16_t(high_[at]&a));
        }
    }
    bool samePixels(const PaletteEffectSurface& other)const{
        synchronize();other.synchronize();
        for(unsigned y=0;y<Height;++y)for(unsigned x=0;x<words;++x){
            const unsigned a=y*stride_+x,b=y*other.stride_+x;
            const auto mask=alpha_[a];
            if(mask!=other.alpha_[b]||((low_[a]^other.low_[b])&mask)||
                ((high_[a]^other.high_[b])&mask))return false;
        }
        return true;
    }
private:
    void synchronize()const{if(backend_.wait)backend_.wait(backend_.context);}
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
        if(backend_.stamp&&backend_.stamp(backend_.context,mask,lo,hi,x,y)){
            if(!subtract){
                const unsigned left=x<0?0:unsigned(x)/16;
                const unsigned right=x+int(mask.width)>int(Width)?words:unsigned(x+int(mask.width)+15)/16;
                const unsigned top=y<0?0:unsigned(y);
                const unsigned bottom=y+int(mask.height)>int(Height)?Height:unsigned(y+int(mask.height));
                for(unsigned row=top;row<bottom;++row){
                    if(left<first_[row])first_[row]=left;
                    if(right>last_[row])last_[row]=right;
                }
                if(top<firstRow_)firstRow_=top;if(bottom>lastRow_)lastRow_=bottom;
            }
            return true;
        }
        synchronize();
        int column=x/16,shift=x%16;if(shift<0){shift+=16;--column;}
        const unsigned begin=y<0?unsigned(-y):0;
        unsigned end=mask.height;if(end>unsigned(int(Height)-y))end=unsigned(int(Height)-y);
        for(unsigned sy=begin;sy<end;++sy){
            const unsigned dy=unsigned(y+int(sy)),offset=sy*mask.stride;
            if(subtract && first_[dy]==words)continue;
            const unsigned row=dy*stride_;
            unsigned first=words,last=0;
            auto put=[&](int dx,std::uint16_t a,std::uint16_t l,std::uint16_t h){
                if(!a || dx<0 || unsigned(dx)>=words)return;
                const unsigned at=row+unsigned(dx);
                if(subtract){alpha_[at]&=std::uint16_t(~a);return;}
                alpha_[at]|=a;
                low_[at]=std::uint16_t((low_[at]&~a)|(l&a));
                high_[at]=std::uint16_t((high_[at]&~a)|(h&a));
                if(unsigned(dx)<first)first=unsigned(dx);
                if(unsigned(dx)+1>last)last=unsigned(dx)+1;
            };
            for(unsigned sx=0;sx<mask.width;sx+=16){
                const unsigned left=mask.width-sx;
                const auto a=read(mask.data+offset,sx,left);
                if(!a)continue;
                const auto l=read(lo?lo+offset:nullptr,sx,left),h=read(hi?hi+offset:nullptr,sx,left);
                const int dx=column+int(sx/16);
                put(dx,a>>shift,l>>shift,h>>shift);
                if(shift)put(dx+1,std::uint16_t(unsigned(a)<<(16-shift)),
                    std::uint16_t(unsigned(l)<<(16-shift)),std::uint16_t(unsigned(h)<<(16-shift)));
            }
            if(last){
                if(first<first_[dy])first_[dy]=first;
                if(last>last_[dy])last_[dy]=last;
                if(dy<firstRow_)firstRow_=dy;
                if(dy+1>lastRow_)lastRow_=dy+1;
            }
        }
        return true;
    }
    std::uint16_t storage_[words*Height*3]{};
    std::uint16_t* alpha_=storage_;
    std::uint16_t* low_=storage_+words*Height;
    std::uint16_t* high_=storage_+words*Height*2;
    unsigned stride_=words;
    Backend backend_{};
    std::uint16_t first_[Height]{},last_[Height]{};
    // Clear/publication of an untouched surface is constant-time. Subtraction
    // never expands this conservative range, including when it removes all alpha.
    std::uint16_t firstRow_=Height,lastRow_=0;
};
}
#endif
