#include "framework/AmigaHardware.h"
#include "framework/Bitmap.h"
#include "AmigaPlaneBank.h"
#ifdef DOWNWELL_PREPARED_FIELDS
#include "AmigaFieldBands.h"
#include "AmigaFieldComposite.h"
#include "AmigaImmutableUpload.h"
#include "framework/BitmapCopyPlan.h"
static UWORD* residentFieldBands;
static AmigaImmutableUpload fieldBandUpload;
static BitmapCopyPlan fieldPaintPlans[17],fieldRestorePlans[17];
extern "C" volatile unsigned gDownwellPreparedFieldMode;
extern "C" void amigaBlitterInvalidateFieldBands(){
    fieldBandUpload.invalidate();
    residentFieldBands=nullptr;
}
extern "C" void amigaBlitterStageFieldBands(UBYTE* bank){
    fieldBandUpload.step(reinterpret_cast<UWORD*>(bank),downwell::fieldBandPixels,
        sizeof(downwell::fieldBandPixels)/sizeof(UWORD),256);
}
extern "C" void amigaBlitterBindFieldBands(UBYTE* bank){
    AmigaHardware::blitterDrain();
    residentFieldBands=reinterpret_cast<UWORD*>(bank);
    fieldBandUpload.step(residentFieldBands,downwell::fieldBandPixels,
        sizeof(downwell::fieldBandPixels)/sizeof(UWORD),
        sizeof(downwell::fieldBandPixels)/sizeof(UWORD));
    residentFieldBands[sizeof(downwell::fieldBandPixels)/sizeof(UWORD)]=0;
    for(unsigned x=0;x<=16;++x){
        if(!fieldPaintPlans[x].prepare(11,11,24,110,x,x,160)||
           !fieldRestorePlans[x].prepare(24,11,24,110,110,x,160)){
            residentFieldBands=nullptr;return;
        }
    }
}
extern "C" int amigaBlitterFieldBands(const UBYTE* terrain,UBYTE* target,
    const downwell::TimeFieldOutlines::Circle& circle,unsigned origin,bool restore,UWORD* rows){
    if(!residentFieldBands)return 0;
    // As with terrain publication, give queued DMA priority while this phase
    // has no useful concurrent CPU work. Do not contend with our own wait loop.
    AmigaHardware::setBlitterNasty(true);
    const bool admitted=amigaFieldBandParts(circle,origin,[&](unsigned id,unsigned y,unsigned sx,unsigned sy,unsigned height){
        const auto& image=downwell::fieldBandImages[id];
        UWORD* pixels=residentFieldBands+image.wordOffset;
        if(gDownwellPreparedFieldMode>=6){
            auto* mask=pixels+11*image.height;
            auto* destination=reinterpret_cast<UWORD*>(target);
            if(restore)for(unsigned plane=0;plane<2;++plane)
                fieldRestorePlans[sx].copy((UWORD*)terrain+plane*7680,mask,destination+plane*7680,y,sy,y,height);
            else{
                fieldPaintPlans[sx].copy(pixels,mask,destination,sy,sy,y,height);
                fieldPaintPlans[sx].copy(pixels,mask,destination+7680,sy,sy,y,height,true);
            }
        }else if(restore){
            Bitmap mask(pixels+11*image.height,176,image.height,1,false);
            Bitmap background((void*)terrain,380,320,2,false,false,384);
            Bitmap destination(target,380,320,2,false,false,384);
            destination.copyWithMask(background,mask,110,y,110,y,sx,sy,160,height,false);
        }else{
            Bitmap source(pixels,176,image.height,1,false),mask(pixels+11*image.height,176,image.height,1,false);
            Bitmap low(target,380,320,1,false,false,384);
            Bitmap high(target+15360,380,320,1,false,false,384);
            low.copyWithMask(source,mask,110,y,sx,sy,sx,sy,160,height,false);
            high.copyInvertedWithMask(source,mask,110,y,sx,sy,sx,sy,160,height);
        }
        if(!restore&&rows)for(unsigned row=y;row<y+height;++row)rows[row/16]|=UWORD(1u<<(row&15));
    });
    if(admitted)AmigaHardware::blitterDrain();
    AmigaHardware::setBlitterNasty(false);
    return admitted;
}
extern "C" int amigaBlitterRetireFieldBands(const UBYTE* terrain,UBYTE* target,
    const downwell::TimeFieldOutlines::Circle& old,const downwell::TimeFieldOutlines::Circle& next,unsigned origin){
    if(!residentFieldBands)return 0;
    AmigaHardware::setBlitterNasty(true);
    const bool ok=amigaFieldRetireOutside(old,next,origin,
        [&](unsigned id,unsigned y,unsigned sx,unsigned sy,unsigned height){
            const auto& image=downwell::fieldBandImages[id];
            UWORD* mask=residentFieldBands+image.wordOffset+11*image.height;
            for(unsigned p=0;p<2;++p)fieldRestorePlans[sx].copy((UWORD*)terrain+p*7680,mask,
                (UWORD*)target+p*7680,y,sy,y,height);
            return true;
        });
    AmigaHardware::blitterDrain();AmigaHardware::setBlitterNasty(false);return ok;
}
extern "C" int amigaBlitterComposeFieldBands(const UBYTE* terrain,UBYTE* target,
    const downwell::TimeFieldOutlines::Circle& circle,unsigned origin,UWORD* rows){
    if(!residentFieldBands)return 0;
    AmigaHardware::setBlitterNasty(true);
    const bool ok=amigaFieldBandParts(circle,origin,[&](unsigned id,unsigned y,unsigned sx,unsigned sy,unsigned height){
        const auto& image=downwell::fieldBandImages[id];
        UWORD* pixels=residentFieldBands+image.wordOffset;UWORD* mask=pixels+11*image.height;
        for(unsigned p=0;p<2;++p)fieldPaintPlans[sx].compose(pixels,mask,(UWORD*)terrain+p*7680,
            (UWORD*)target+p*7680,sy,sy,y,height,p!=0);
        if(rows)for(unsigned row=y;row<y+height;++row)rows[row/16]|=UWORD(1u<<(row&15));
    });
    AmigaHardware::blitterDrain();AmigaHardware::setBlitterNasty(false);return ok;
}
#endif
#ifdef DOWNWELL_PREPARED_FIELDS_VERIFY
#include "core/TimeFieldOutlines.h"
#endif

#include <exec/types.h>
inline void* operator new(__SIZE_TYPE__, void* address) noexcept { return address; }

namespace {
struct PreparedBitmap {
    const UBYTE* original;
    Bitmap source[2], mask;
    PreparedBitmap(const UBYTE* originalData,const UBYTE* low,const UBYTE* high,
                   const UBYTE* maskData, UWORD width, UWORD height, UWORD stride)
        : original(originalData),source{Bitmap((void*)low,width,height,1,false,false,stride),
                                      Bitmap((void*)high,width,height,1,false,false,stride)},
          mask((void*)maskData, width, height, 1, false, false, stride) {}
};
alignas(PreparedBitmap) UBYTE artStorage[64 * sizeof(PreparedBitmap)];
bool artReady[64];
AmigaPlaneBank gamePlanes;
}

extern "C" int amigaBlitterBindGamePlanes(const UBYTE* source,unsigned sourceBytes,
    const UBYTE* bank,unsigned bytes,const UBYTE* hash){
    AmigaHardware::blitterDrain();
    for(unsigned i=0;i<64;++i)if(artReady[i]){
        reinterpret_cast<PreparedBitmap*>(artStorage+i*sizeof(PreparedBitmap))->~PreparedBitmap();
        artReady[i]=false;
    }
    return gamePlanes.bind(source,sourceBytes,bank,bytes,hash);
}

extern "C" void amigaBlitterReleaseGamePlanes(){
    // Invalid binding drains outstanding blits and retires every cached view.
    (void)amigaBlitterBindGamePlanes(nullptr,0,nullptr,0,nullptr);
}

#ifdef DOWNWELL_PREPARED_FIELDS_VERIFY
extern "C" {
volatile unsigned gDownwellPreparedFieldChecks,gDownwellPreparedFieldFailures;
volatile unsigned gDownwellPreparedFieldFusedChecks;
volatile unsigned gDownwellPreparedFieldFixtureChecks;
volatile unsigned gDownwellPreparedFieldCompositeChecks;
volatile unsigned gDownwellPreparedFieldMismatchOffset,gDownwellPreparedFieldExpectedByte,
    gDownwellPreparedFieldActualByte,gDownwellPreparedFieldOperation;
volatile unsigned gDownwellPreparedFieldRect[6];
__attribute__((noinline,used)) void downwellPreparedFieldMismatch(){__asm__ volatile("" ::: "memory");}
}
namespace {
UBYTE fieldExpected[30720];
UBYTE savedFieldTerrain[30720],savedFieldTarget[30720];
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
    if(gDownwellPreparedFieldOperation==10)++gDownwellPreparedFieldFusedChecks;
    for(unsigned i=0;i<fieldExpectedBytes;++i)if(actual[i]!=fieldExpected[i]){
        gDownwellPreparedFieldMismatchOffset=i;gDownwellPreparedFieldExpectedByte=fieldExpected[i];
        gDownwellPreparedFieldActualByte=actual[i];
        ++gDownwellPreparedFieldFailures;downwellPreparedFieldMismatch();return;
    }
}
extern "C" void amigaFieldVerifyTerrain(const UBYTE* source,const UBYTE* mask,const UBYTE* target,
    unsigned x,unsigned y,unsigned sx,unsigned sy,unsigned width,unsigned height){
    fieldExpectedBytes=30720;gDownwellPreparedFieldOperation=3;
    const unsigned rect[6]={x,y,sx,sy,width,height};
    for(unsigned i=0;i<6;++i)gDownwellPreparedFieldRect[i]=rect[i];
    for(unsigned i=0;i<fieldExpectedBytes;++i)fieldExpected[i]=target[i];
    for(unsigned row=0;row<height;++row)for(unsigned col=0;col<width;++col)
        if(fieldPixel(mask,8,sx+col,sy+row))for(unsigned p=0;p<2;++p)
            fieldPut(fieldExpected+p*15360,48,x+col,y+row,fieldPixel(source+p*15360,48,x+col,y+row));
}
#endif

extern "C" int amigaBlitterDrawPlanarMasked(
    const UBYTE* source, const UBYTE* mask,
    UWORD sourceWidth, UWORD sourceHeight, UWORD sourceDataWidth,
    UBYTE* destination, UWORD destinationHeight,
    UWORD destinationX, UWORD destinationY,
    UWORD sourceX, UWORD sourceY, UWORD width, UWORD height, int outputPen)
{
    if ((ULONG)destination >= 0x200000UL)
        return 0;

    const unsigned slot = ((ULONG)source >> 2) & 63;
    PreparedBitmap* art = reinterpret_cast<PreparedBitmap*>(artStorage + slot * sizeof(PreparedBitmap));
    if (!artReady[slot] || art->original != source ||
        art->source[0].width != sourceWidth || art->source[0].height != sourceHeight ||
        art->source[0].dataWidth != sourceDataWidth ||
        (!gamePlanes.contains(source)&&art->mask.data!=mask)) {
        const unsigned planeBytes=unsigned(sourceDataWidth/8)*sourceHeight;
        const UBYTE* low=source;const UBYTE* high=source+planeBytes;
        if(gamePlanes.contains(source)&&!gamePlanes.find(source,planeBytes,low,high,mask))return 0;
        if((ULONG)low>=0x200000UL||(ULONG)high>=0x200000UL||(ULONG)mask>=0x200000UL)return 0;
        if (artReady[slot]) art->~PreparedBitmap();
        new (art) PreparedBitmap(source,low,high,mask,sourceWidth,sourceHeight,sourceDataWidth);
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
    } else for(unsigned plane=0;plane<2;++plane){
        Bitmap target(destination+plane*destinationBitmap.bitplaneSizeInBytes,
            380,destinationHeight,1,false,false,384);
        target.copyWithMask(art->source[plane],art->mask,destinationX,destinationY,
            sourceX,sourceY,sourceX,sourceY,width,height,false);
    }
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

#ifdef DOWNWELL_PLANE_BANK_VERIFY
extern "C" {
volatile unsigned gDownwellPlaneBlitChecks,gDownwellPlaneBlitFailures;
__attribute__((noinline,used)) void downwellPlaneBlitsReady(){__asm__ volatile("" ::: "memory");}
int amigaBlitterVerifyGamePlanes(const UBYTE* pack,UBYTE* target){
    const auto word=[](const UBYTE* p){return unsigned(p[0])*256+p[1];};
    const auto number=[&](const UBYTE* p){return (ULONG(word(p))<<16)|word(p+2);};
    static UBYTE expected[2][8*48];
    const unsigned count=number(pack+8);
    for(unsigned i=0;i<count;++i){
        const UBYTE* r=pack+20+i*24;if(word(r+14)!=3)continue;
        const unsigned width=word(r+8),height=word(r+10),stride=word(r+12)/8;
        const unsigned plane=stride*height;
        const UBYTE* source=pack+number(r+16);
        for(unsigned pass=0;pass<2;++pass){
            const unsigned sx=pass&&width>1?1:0,sy=pass&&height>1?height-2:0;
            const unsigned w=width-sx<19?width-sx:19,h=height-sy<5?height-sy:5;
            const unsigned x=120+(i+pass*7)%16;
            for(unsigned p=0;p<2;++p)for(unsigned b=0;b<8*48;++b)
                target[p*15360+b]=expected[p][b]=0xa5;
            for(unsigned y=0;y<h;++y)for(unsigned bit=0;bit<w;++bit){
                const unsigned at=(sy+y)*stride+(sx+bit)/8,mask=0x80u>>((sx+bit)&7);
                const bool low=source[at]&mask,high=source[plane+at]&mask;
                if(!low&&!high)continue;
                const unsigned dest=y*48+(x+bit)/8,selected=0x80u>>((x+bit)&7);
                expected[0][dest]=(expected[0][dest]&~selected)|(low?selected:0);
                expected[1][dest]=(expected[1][dest]&~selected)|(high?selected:0);
            }
            if(!amigaBlitterDrawPlanarMasked(source,source+2*plane,width,height,stride*8,
                target,320,x,0,sx,sy,w,h,-1)){++gDownwellPlaneBlitFailures;return 0;}
            for(unsigned p=0;p<2;++p)for(unsigned b=0;b<8*48;++b)
                if(target[p*15360+b]!=expected[p][b]){++gDownwellPlaneBlitFailures;return 0;}
            ++gDownwellPlaneBlitChecks;
        }
    }
    downwellPlaneBlitsReady();return 1;
}
}
#endif

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

extern "C" int amigaBlitterRestoreField(const UBYTE* terrain,const UBYTE* mask,UBYTE* target,
    UWORD x,UWORD y,UWORD sx,UWORD sy,UWORD width,UWORD height)
{
    if((ULONG)terrain>=0x200000UL||(ULONG)target>=0x200000UL||(ULONG)mask>=0x200000UL||
       !width||!height||x+width>384||y+height>320||sx+width>64||sy+height>56)return 0;
    Bitmap source((void*)terrain,380,320,2,false,false,384);
    Bitmap destination(target,380,320,2,false,false,384);
    Bitmap coverage((void*)mask,64,56,1,false,false,64);
    destination.copyWithMask(source,coverage,x,y,x,y,sx,sy,width,height,false);
    AmigaHardware::blitterDrain();
    return 1;
}

#ifdef DOWNWELL_PREPARED_FIELDS_VERIFY
extern "C" int amigaFieldVerifyTerrainFixture(UBYTE* terrain,UBYTE* target,UBYTE* mask){
    UBYTE savedMask[448];
    AmigaHardware::blitterDrain();
    for(unsigned i=0;i<448;++i){savedMask[i]=mask[i];mask[i]=(i*73u+(i>>4)+29)&255u;}
    for(unsigned i=0;i<30720;++i){
        savedFieldTerrain[i]=terrain[i];savedFieldTarget[i]=target[i];
        terrain[i]=(i*37u+(i>>7))&255u;target[i]=(i*53u+(i>>9)+71)&255u;
    }
    bool ok=true;
    for(unsigned xPhase=0;xPhase<16&&ok;++xPhase)for(unsigned maskPhase=0;maskPhase<16&&ok;++maskPhase)
        for(unsigned size=0;size<2&&ok;++size){
            const unsigned width=size?48:2;
            amigaFieldVerifyTerrain(terrain,mask,target,112+xPhase,12,maskPhase,0,width,56);
            ok=amigaBlitterRestoreField(terrain,mask,target,112+xPhase,12,maskPhase,0,width,56)!=0;
            amigaFieldVerifyFinish(target);++gDownwellPreparedFieldFixtureChecks;
            ok=ok&&!gDownwellPreparedFieldFailures;
        }
    for(unsigned i=0;i<30720;++i){terrain[i]=savedFieldTerrain[i];target[i]=savedFieldTarget[i];}
    for(unsigned i=0;i<448;++i)mask[i]=savedMask[i];
    return ok?1:0;
}
extern "C" void amigaFieldVerifyCompositeBegin(UBYTE* terrain,UBYTE* target){
    AmigaHardware::blitterDrain();
    for(unsigned i=0;i<30720;++i){
        savedFieldTerrain[i]=terrain[i];savedFieldTarget[i]=target[i];
        terrain[i]=target[i]=(i*37u+(i>>7))&255u;
    }
}
extern "C" void amigaFieldVerifyCompositeEnd(UBYTE* terrain,UBYTE* target){
    AmigaHardware::blitterDrain();
    for(unsigned i=0;i<30720;++i){terrain[i]=savedFieldTerrain[i];target[i]=savedFieldTarget[i];}
}
extern "C" bool amigaFieldVerifyComposite(const UBYTE* terrain,const UBYTE* target,
    const downwell::TimeFieldOutlines& shape,unsigned origin){
    fieldExpectedBytes=30720;gDownwellPreparedFieldOperation=4;
    for(unsigned i=0;i<30720;++i)fieldExpected[i]=terrain[i];
    // Independent portable per-pixel raster, not the Bitmap transfer oracle.
    shape.drawClipped(110,0,270,284,[&](int x,int y,unsigned pen){
        const unsigned row=(origin+unsigned(y))%320;
        for(unsigned p=0;p<2;++p)fieldPut(fieldExpected+p*15360,48,x,row,(pen>>p)&1);
    });
    amigaFieldVerifyFinish(target);++gDownwellPreparedFieldCompositeChecks;
    return !gDownwellPreparedFieldFailures;
}
extern "C" void amigaFieldVerifyColumn(const UBYTE* terrain,const UBYTE* target,
    const downwell::TimeFieldOutlines::Circle& circle,unsigned origin,bool restore){
    fieldExpectedBytes=30720;gDownwellPreparedFieldOperation=restore?7:8;
    for(unsigned i=0;i<30720;++i)fieldExpected[i]=target[i];
    downwell::GoalFieldOutline shape;
    shape.setRasterCircle(circle.x,circle.y,circle.radius);
    shape.drawClipped(110,0,270,284,[&](int x,int y,unsigned pen){
        const unsigned row=(origin+unsigned(y))%320;
        for(unsigned p=0;p<2;++p)fieldPut(fieldExpected+p*15360,48,x,row,
            restore?fieldPixel(terrain+p*15360,48,x,row):(pen>>p)&1);
    });
}
extern "C" void amigaFieldVerifyCompositeTransfer(const UBYTE* terrain,const UBYTE* target,
    const downwell::TimeFieldOutlines::Circle& old,const downwell::TimeFieldOutlines::Circle& next,unsigned origin){
    amigaFieldVerifyColumn(terrain,target,old,origin,true);
    gDownwellPreparedFieldOperation=10;
    downwell::GoalFieldOutline shape;shape.setRasterCircle(next.x,next.y,next.radius);
    shape.drawClipped(110,0,270,284,[&](int x,int y,unsigned pen){
        const unsigned row=(origin+unsigned(y))%320;
        for(unsigned p=0;p<2;++p)fieldPut(fieldExpected+p*15360,48,x,row,(pen>>p)&1);
    });
}
#endif

extern "C" int amigaBlitterSolidRect(UBYTE* destination, UWORD targetHeight,
    UWORD x, UWORD y, UWORD width, UWORD height, UWORD pen)
{
    if ((ULONG)destination >= 0x200000UL) return 0;
    Bitmap bitmap(destination, 380, targetHeight, 2, false, false, 384);
    bitmap.solidRect(x, y, width, height, pen);
    AmigaHardware::blitterDrain();
    return 1;
}
