#include "framework/AmigaHardware.h"
#include "framework/Bitmap.h"

#include <exec/types.h>
inline void* operator new(__SIZE_TYPE__, void* address) noexcept { return address; }

namespace {
struct PreparedBitmap {
    Bitmap source, mask;
    PreparedBitmap(const UBYTE* data, const UBYTE* maskData, UWORD width, UWORD height, UWORD stride)
        : source((void*)data, width, height, 2, false, false, stride),
          mask((void*)maskData, width, height, 1, false, false, stride) {}
};
alignas(PreparedBitmap) UBYTE artStorage[64 * sizeof(PreparedBitmap)];
bool artReady[64];
}

#ifdef DOWNWELL_PREPARED_FIELDS_VERIFY
extern "C" {
volatile unsigned gDownwellPreparedFieldChecks,gDownwellPreparedFieldFailures;
volatile unsigned gDownwellPreparedFieldMismatchOffset,gDownwellPreparedFieldExpectedByte,
    gDownwellPreparedFieldActualByte,gDownwellPreparedFieldOperation;
volatile unsigned gDownwellPreparedFieldRect[6];
__attribute__((noinline,used)) void downwellPreparedFieldMismatch(){__asm__ volatile("" ::: "memory");}
}
namespace {
UBYTE fieldExpected[30720];
unsigned fieldExpectedBytes;
unsigned fieldPixel(const UBYTE* bytes,unsigned stride,unsigned x,unsigned y){
    return (bytes[y*stride+x/8]>>(7-(x&7)))&1;
}
void fieldPut(UBYTE* bytes,unsigned stride,unsigned x,unsigned y,unsigned value){
    const unsigned offset=y*stride+x/8,mask=0x80u>>(x&7);
    bytes[offset]=(bytes[offset]&~mask)|(value?mask:0);
}
}
extern "C" void amigaFieldVerifyCapture(const UBYTE* saved,const UBYTE* target,const UBYTE* mask,
    unsigned x,unsigned y,unsigned sx,unsigned sy,unsigned width,unsigned height){
    fieldExpectedBytes=896;
    gDownwellPreparedFieldOperation=1;
    const unsigned rect[6]={x,y,sx,sy,width,height};
    for(unsigned i=0;i<6;++i)gDownwellPreparedFieldRect[i]=rect[i];
    for(unsigned i=0;i<fieldExpectedBytes;++i)fieldExpected[i]=saved[i];
    for(unsigned p=0;p<2;++p)for(unsigned row=0;row<height;++row)for(unsigned col=0;col<width;++col)
        if(fieldPixel(mask,8,sx+col,sy+row))
            fieldPut(fieldExpected+p*448,8,sx+col,sy+row,fieldPixel(target+p*15360,48,x+col,y+row));
}
extern "C" void amigaFieldVerifyMask(const UBYTE* source,const UBYTE* mask,const UBYTE* target,
    unsigned x,unsigned y,unsigned sx,unsigned sy,unsigned width,unsigned height){
    fieldExpectedBytes=30720;
    gDownwellPreparedFieldOperation=2;
    const unsigned rect[6]={x,y,sx,sy,width,height};
    for(unsigned i=0;i<6;++i)gDownwellPreparedFieldRect[i]=rect[i];
    for(unsigned i=0;i<fieldExpectedBytes;++i)fieldExpected[i]=target[i];
    for(unsigned row=0;row<height;++row)for(unsigned col=0;col<width;++col)
        if(fieldPixel(mask,8,sx+col,sy+row))for(unsigned p=0;p<2;++p)
            fieldPut(fieldExpected+p*15360,48,x+col,y+row,fieldPixel(source+p*448,8,sx+col,sy+row));
}
extern "C" void amigaFieldVerifyFinish(const UBYTE* actual){
    ++gDownwellPreparedFieldChecks;
    for(unsigned i=0;i<fieldExpectedBytes;++i)if(actual[i]!=fieldExpected[i]){
        gDownwellPreparedFieldMismatchOffset=i;gDownwellPreparedFieldExpectedByte=fieldExpected[i];
        gDownwellPreparedFieldActualByte=actual[i];
        ++gDownwellPreparedFieldFailures;downwellPreparedFieldMismatch();return;
    }
}
#endif

extern "C" int amigaBlitterDrawPlanarMasked(
    const UBYTE* source, const UBYTE* mask,
    UWORD sourceWidth, UWORD sourceHeight, UWORD sourceDataWidth,
    UBYTE* destination, UWORD destinationHeight,
    UWORD destinationX, UWORD destinationY,
    UWORD sourceX, UWORD sourceY, UWORD width, UWORD height, int outputPen)
{
    if ((ULONG)source >= 0x200000UL ||
        (ULONG)mask >= 0x200000UL ||
        (ULONG)destination >= 0x200000UL)
        return 0;

    const unsigned slot = ((ULONG)source >> 2) & 63;
    PreparedBitmap* art = reinterpret_cast<PreparedBitmap*>(artStorage + slot * sizeof(PreparedBitmap));
    if (!artReady[slot] || art->source.data != source || art->mask.data != mask ||
        art->source.width != sourceWidth || art->source.height != sourceHeight ||
        art->source.dataWidth != sourceDataWidth) {
        if (artReady[slot]) art->~PreparedBitmap();
        new (art) PreparedBitmap(source, mask, sourceWidth, sourceHeight, sourceDataWidth);
        artReady[slot] = true;
    }
    Bitmap destinationBitmap(destination, 380, destinationHeight, 2,
                             false, false, 384);
    if (outputPen >= 0) {
        for (unsigned plane = 0; plane < 2; ++plane) {
            Bitmap target(destination + plane * destinationBitmap.bitplaneSizeInBytes,
                380, destinationHeight, 1, false, false, 384);
            target.patternWithMask(art->mask, destinationX, destinationY,
                sourceX, sourceY, width, height, outputPen & (1u << plane) ? 0xffff : 0);
        }
    } else destinationBitmap.copyWithMask(
        art->source, art->mask,
        destinationX, destinationY,
        sourceX, sourceY, sourceX, sourceY,
        width, height, false);
    AmigaHardware::blitterDrain();
    return 1;
}

extern "C" int amigaBlitterClearPlanar(UBYTE* destination,
                                         UWORD height, UWORD planes)
{
    if ((ULONG)destination >= 0x200000UL || height == 0 || planes == 0)
        return 0;
    AmigaHardware::blitterClear(
        (UWORD*)destination, 384 / 16, (UWORD)(height * planes), 0);
    // Clear may be queued behind a terrain/display copy. Waiting only for
    // the active DMA does not finish queued work; subsequent CPU tile writes
    // would then be erased when the pending clear finally executes.
    AmigaHardware::blitterDrain();
    return 1;
}

// Saved field underlay uses the same shared Bitmap path as its masked draw.
extern "C" int amigaBlitterCaptureField(UBYTE* saved,const UBYTE* target,const UBYTE* mask,
    UWORD x,UWORD y,UWORD sx,UWORD sy,UWORD width,UWORD height)
{
    if((ULONG)saved>=0x200000UL||(ULONG)target>=0x200000UL||(ULONG)mask>=0x200000UL||
       !width||!height||x+width>384||y+height>320||sx+width>64||sy+height>56)return 0;
    Bitmap source((void*)target,380,320,2,false,false,384);
    Bitmap destination(saved,64,56,2,false,false,64);
    Bitmap coverage((void*)mask,64,56,1,false,false,64);
    // Ordinary copy overwrites whole edge words. The backing is tightly
    // packed, so even capture must preserve neighbouring rows and planes.
    destination.copyWithMask(source,coverage,sx,sy,x,y,sx,sy,width,height,false);
    AmigaHardware::blitterDrain();
    return 1;
}

extern "C" int amigaBlitterSolidRect(UBYTE* destination, UWORD targetHeight,
    UWORD x, UWORD y, UWORD width, UWORD height, UWORD pen)
{
    if ((ULONG)destination >= 0x200000UL) return 0;
    Bitmap bitmap(destination, 380, targetHeight, 2, false, false, 384);
    bitmap.solidRect(x, y, width, height, pen);
    AmigaHardware::blitterDrain();
    return 1;
}
