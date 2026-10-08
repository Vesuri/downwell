#ifndef DOWNWELL_PACKED_MASK_INTERSECTION_H
#define DOWNWELL_PACKED_MASK_INTERSECTION_H
#include "BinaryEffectSurface.h"
namespace downwell {
// Exact intersection of two admitted immutable packed masks inside an already
// clipped rectangle. No temporary raster and no per-pixel source extraction.
struct PackedMaskIntersection {
    using Mask=BinaryEffectSurface<>::Mask;
    static unsigned word(const std::uint8_t* row,unsigned stride,unsigned x){
        const unsigned byte=x>>3;
        std::uint32_t bits=std::uint32_t(row[byte])<<16;
        if(byte+1<stride)bits|=unsigned(row[byte+1])<<8;
        if(byte+2<stride)bits|=row[byte+2];
        return unsigned(bits>>(8-(x&7)))&65535u;
    }
    static bool intersects(const Mask& a,int ax,int ay,const Mask& b,int bx,int by,
        int left,int top,int right,int bottom){
        if(left>=right||top>=bottom)return false;
        for(int x=left;x<right;x+=16){
            const unsigned width=unsigned(right-x)<16?unsigned(right-x):16;
            const unsigned mask=(65535u<<(16-width))&65535u;
            const auto* ar=a.data+(top-ay)*a.stride;
            const auto* br=b.data+(top-by)*b.stride;
            for(int y=top;y<bottom;++y,ar+=a.stride,br+=b.stride)
                if(word(ar,a.stride,unsigned(x-ax))&word(br,b.stride,unsigned(x-bx))&mask)return true;
        }
        return false;
    }
};
}
#endif
