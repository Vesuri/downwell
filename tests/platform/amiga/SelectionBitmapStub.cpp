// Scalar framework stand-in for host tests of selection command retention.
// Native builds link the actual Bitmap implementation and its blitter paths.
#include "platform/amiga/framework/Bitmap.h"
Bitmap::Bitmap(void* p, uint16_t w, uint16_t h, uint16_t planes, bool inter,
               bool own, uint16_t dma)
    : data(p), width(w), height(h), bitplanes(planes), dataWidth(dma ? dma : ((w+15)&~15)),
      widthInBytes(dataWidth/8), rowSizeInBytes(widthInBytes),
      bitplaneSizeInBytes(widthInBytes*h), interleaved(inter), owner(own), blittable(false) {}
Bitmap::~Bitmap() {}
namespace {
bool bit(const Bitmap& b, unsigned plane, unsigned x, unsigned y) {
    return (static_cast<const uint8_t*>(b.data)[plane*b.bitplaneSizeInBytes+y*b.widthInBytes+x/8] & (128u>>(x&7))) != 0;
}
void put(Bitmap& b, unsigned plane, unsigned x, unsigned y, bool v) {
    auto& byte = static_cast<uint8_t*>(b.data)[plane*b.bitplaneSizeInBytes+y*b.widthInBytes+x/8];
    const auto mask = 128u>>(x&7);
    byte = v ? byte | mask : byte & ~mask;
}
}
void Bitmap::clear(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    if (!w) w = width;
    if (!h) h = height;
    const unsigned right = (x+w+15)&~15u;
    x &= ~15u; w = right-x;
    for (unsigned p=0;p<bitplanes;++p) for(unsigned yy=0;yy<h;++yy)
        for(unsigned xx=0;xx<w;++xx) put(*this,p,x+xx,y+yy,false);
}
void Bitmap::solidRect(uint16_t x,uint16_t y,uint16_t w,uint16_t h,uint16_t color) {
    for(unsigned p=0;p<bitplanes;++p) for(unsigned yy=0;yy<h;++yy)
        for(unsigned xx=0;xx<w;++xx) put(*this,p,x+xx,y+yy,(color&(1u<<p))!=0);
}
void Bitmap::copyWithMask(const Bitmap& s, const Bitmap& m, uint16_t x, uint16_t y,
    uint16_t sx, uint16_t sy, uint16_t mx, uint16_t my, uint16_t w, uint16_t h, bool) {
    for(unsigned yy=0;yy<h;++yy) for(unsigned xx=0;xx<w;++xx) if(bit(m,0,mx+xx,my+yy))
        for(unsigned p=0;p<bitplanes;++p) put(*this,p,x+xx,y+yy,bit(s,p,sx+xx,sy+yy));
}
void Bitmap::patternWithMask(const Bitmap& m, uint16_t x, uint16_t y, uint16_t mx,
    uint16_t my, uint16_t w, uint16_t h, uint16_t pattern) {
    for(unsigned yy=0;yy<h;++yy) for(unsigned xx=0;xx<w;++xx) if(bit(m,0,mx+xx,my+yy))
        put(*this,0,x+xx,y+yy,(pattern & (0x8000u>>(xx&15))) != 0);
}
