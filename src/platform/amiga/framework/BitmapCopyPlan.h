#ifndef DOWNWELL_BITMAP_COPY_PLAN_H
#define DOWNWELL_BITMAP_COPY_PLAN_H
#include "AmigaHardware.h"
#include "core/NarrowArithmetic.h"

// Prepared form of Bitmap::copyWithMaskOperation for one non-interleaved
// plane. The original Bitmap path remains the native comparison. Only row
// placement and height vary at submission; word masks/modulos/shift direction
// are prepared once. Buffers need one readable guard word after their planes,
// like the original shifted blitter path's zero-masked extra fetch.
class BitmapCopyPlan {
    uint16_t sourceStride_=0,maskStride_=0,targetStride_=0,words_=0;
    int16_t sourceWord_=0,maskWord_=0,targetWord_=0;
    int16_t sourceModulo_=0,maskModulo_=0,targetModulo_=0;
    int16_t sourceShift_=0,maskShift_=0;
    uint16_t first_=0,last_=0;
    bool reverse_=false;
public:
    bool prepare(unsigned sourceWords,unsigned maskWords,unsigned targetWords,
                 unsigned targetX,unsigned sourceX,unsigned maskX,unsigned width){
        words_=0;
        // Source-aligned mask (painting) or target-aligned source (restoring).
        // Other channel-phase combinations retain the general Bitmap path.
        if(!width||!sourceWords||!maskWords||!targetWords||
           sourceX+width>sourceWords*16||maskX+width>maskWords*16||
           targetX+width>targetWords*16||
           (sourceX!=maskX&&sourceX!=targetX))return false;
        unsigned sourceFirst=sourceX/16,targetFirst=targetX/16,maskFirst=maskX/16;
        unsigned n=(sourceX+width-1)/16-sourceFirst;
        const unsigned targetN=(targetX+width-1)/16-targetFirst;
        const unsigned maskN=(maskX+width-1)/16-maskFirst;
        if(targetN>n)n=targetN;if(maskN>n)n=maskN;
        if(n>=64)return false;
        sourceShift_=int(targetX&15)-int(sourceX&15);
        maskShift_=int(targetX&15)-int(maskX&15);
        reverse_=sourceShift_<0||(sourceShift_==0&&maskShift_<0);
        if(sourceShift_>=0&&maskN+1<n+1&&((maskX+width)&15)&&((targetX+width)&15))return false;
        sourceWord_=sourceFirst;maskWord_=maskFirst;targetWord_=targetFirst;
        const unsigned trailing=(maskFirst+n)*16+16-maskX-width;
        first_=uint16_t(0xffffu>>(maskX&15));
        last_=trailing>=16?0:uint16_t(0xffffu<<trailing);
        if(reverse_){
            const auto mask=first_;first_=last_;last_=mask;
            sourceWord_+=n;maskWord_+=n;targetWord_+=n;
            if(maskShift_>0){maskShift_-=16;--maskWord_;}
        }else if(maskShift_<0)maskShift_+=16;
        if(maskWord_<0)return false;
        sourceStride_=sourceWords;maskStride_=maskWords;targetStride_=targetWords;
        words_=n+1;
        sourceModulo_=(int(sourceWords)-int(words_))*2;
        maskModulo_=(int(maskWords)-int(words_))*2;
        targetModulo_=(int(targetWords)-int(words_))*2;
        return true;
    }
    // Rows (after the reverse adjustment) fit a word: the field bands' are <320.
    void copy(uint16_t* source,uint16_t* mask,uint16_t* target,
              unsigned sourceY,unsigned maskY,unsigned targetY,unsigned height,bool invert=false) const {
        if(!words_||!height)return;
        if(reverse_){sourceY+=height-1;maskY+=height-1;targetY+=height-1;}
        AmigaHardware::blitterCopyWithMaskOperation(
            source+downwell::mulu16(sourceY,sourceStride_)+sourceWord_,target+downwell::mulu16(targetY,targetStride_)+targetWord_,
            mask+downwell::mulu16(maskY,maskStride_)+maskWord_,words_,height,sourceModulo_,targetModulo_,maskModulo_,
            sourceShift_,maskShift_,first_,last_,false,invert);
    }
    void compose(uint16_t* source,uint16_t* mask,uint16_t* background,uint16_t* target,
                 unsigned sourceY,unsigned maskY,unsigned targetY,unsigned height,bool invert=false) const {
        if(!words_||!height)return;
        if(reverse_){sourceY+=height-1;maskY+=height-1;targetY+=height-1;}
        const unsigned at=downwell::mulu16(targetY,targetStride_)+targetWord_;
        AmigaHardware::blitterCopyWithMaskBackgroundOperation(
            source+downwell::mulu16(sourceY,sourceStride_)+sourceWord_,target+at,mask+downwell::mulu16(maskY,maskStride_)+maskWord_,background+at,
            words_,height,sourceModulo_,targetModulo_,maskModulo_,sourceShift_,maskShift_,first_,last_,false,invert);
    }
};
#endif
