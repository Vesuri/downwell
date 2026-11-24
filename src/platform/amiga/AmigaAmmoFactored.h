#ifndef DOWNWELL_AMIGA_AMMO_FACTORED_H
#define DOWNWELL_AMIGA_AMMO_FACTORED_H
#include "AmigaRenderer.h"
#include "core/AmmoGaugeRows.h"
#include "core/NarrowArithmetic.h"
#include "core/Profile.h"
#ifdef DOWNWELL_INSTALLED_TABLES
#include "../../../generated/installed/amiga-ammo-factored.h" // WI.3: bound from Downwell.tables
#else
#include "../../../generated/amiga-ammo-factored.h"
#endif
extern volatile unsigned amigaAmmoFactoredMode;

// Complete prepared column plus independent outer-rim changes. The labels are
// fused into final publication: never erase a label just to draw it again.
// Only callers using AmmoGaugeRows::step may enable this observer. The generic
// compositor remains the fallback for edited rows, artwork and wider numbers.
class AmigaAmmoFactored {
    const UBYTE* pack_ = nullptr;
    const UBYTE* linkedCopy_ = nullptr;
    const UBYTE* linkedCopyEnd_ = nullptr;
    UBYTE* destination_ = nullptr;
    bool matches_ = false, valid_ = false, uniform_ = false;
    unsigned capacity_ = 0, meter_ = 0, label_ = 0;
    std::uint32_t serial_ = 0;
    UWORD oldTiles_[16] = {};
    static UWORD read(const UBYTE* p) {
#ifdef DOWNWELL_PLATFORM_AMIGA
        return *reinterpret_cast<const UWORD*>(p);
#else
        return UWORD(unsigned(p[0])*256+p[1]);
#endif
    }
    static void write(UBYTE* p,UWORD word) {
#ifdef DOWNWELL_PLATFORM_AMIGA
        *reinterpret_cast<UWORD*>(p)=word;
#else
        p[0]=UBYTE(word>>8);p[1]=UBYTE(word);
#endif
    }
    static unsigned longWord(const UBYTE* p) { return (unsigned(read(p))<<16)|read(p+2); }
    static bool equal(const UBYTE* a,const UBYTE* b,unsigned size) {
        for(unsigned i=0;i<size;++i)if(a[i]!=b[i])return false;
        return true;
    }
    static bool matches(const UBYTE* pack,const UBYTE* end) {
        DW_PROFILE_SCOPE(profileAmmoArtAdmission);
        using namespace downwell_ammo_factored;
        if(!pack || end-pack<12 || !equal(pack,reinterpret_cast<const UBYTE*>("DWAP"),4) || read(pack+4)!=2)return false;
        const unsigned directory=read(pack+6),count=longWord(pack+8);
        if(directory>unsigned(end-pack) || count>(unsigned(end-pack)-directory)/24)return false;
#ifdef DOWNWELL_INSTALLED_TABLES
        for(unsigned r=0;r<sourceRecords_count;++r){
#else
        for(unsigned r=0;r<sizeof(sourceRecords)/sizeof(sourceRecords[0]);++r){
#endif
            const auto& expected=sourceRecords[r];
            const UBYTE* record=nullptr;
            for(unsigned i=0;i<count;++i){
                const UBYTE* candidate=pack+directory+i*24;
                if(read(candidate)==expected[0] && read(candidate+2)==expected[1]){record=candidate;break;}
            }
            if(!record)return false;
            for(unsigned i=2;i<8;++i)
                if((i<4?int(static_cast<short>(read(record+i*2))):int(read(record+i*2)))!=expected[i])return false;
            const unsigned offset=longWord(record+16),size=longWord(record+20);
            if(size!=unsigned(expected[9]) || offset>unsigned(end-pack) || size>unsigned(end-pack)-offset ||
               !equal(pack+offset,sourceBytes+expected[8],size))return false;
        }
        return true;
    }
public:
    // Called only by the embedded bank's copy/residency owner. The complete
    // immutable source is link-sealed; arbitrary packs still use matches().
    void bindLinkedCopy(const UBYTE* source,const UBYTE* sourceEnd,
                        const UBYTE* copy,const UBYTE* copyEnd) {
#if defined(DOWNWELL_PLATFORM_AMIGA) && !defined(DOWNWELL_REFERENCE_AMMO_ADMISSION)
        const bool accepted=downwell_ammo_factored::linkedSource(source,sourceEnd)&&
            copy&&copyEnd&&copyEnd>=copy&&copyEnd-copy==sourceEnd-source;
        if(pack_==linkedCopy_||pack_==copy)pack_=nullptr;
        linkedCopy_=accepted?copy:nullptr;linkedCopyEnd_=accepted?copyEnd:nullptr;
#else
        (void)source;(void)sourceEnd;(void)copy;(void)copyEnd;
#endif
    }
    void invalidate() { valid_=uniform_=false;capacity_=0; }
    bool draw(AmigaRenderer* renderer,UBYTE* retained,const UBYTE* pack,const UBYTE* end,
              unsigned ammo,unsigned capacity,unsigned offset,const downwell::AmmoGaugeRows* rows,
              unsigned meter,unsigned& writes) {
        using namespace downwell_ammo_factored;
        if(pack_!=pack){
            pack_=pack;
            matches_=(pack&&pack==linkedCopy_&&end==linkedCopyEnd_)||matches(pack,end);
            invalidate();
        }
        if(destination_!=renderer->hud){destination_=renderer->hud;invalidate();}
        if(!matches_ || !rows || !capacity || capacity>16 || ammo>capacity || ammo>=10 || meter>3 || offset>2){invalidate();return false;}
        const auto* boundaries=bounds[capacity-1];
        if(rows->filled!=boundaries[ammo]){invalidate();return false;}
        if(capacity_!=capacity){invalidate();capacity_=capacity;}
        if(rows->visualSerial()==0xffffffffu ||
           (rows->visualSerial()!=serial_ && rows->visualSerial()!=serial_+1u))uniform_=false;
        if(!uniform_){
            for(unsigned segment=0;segment<capacity;++segment)
                for(unsigned row=boundaries[segment]+1;row<boundaries[segment+1];++row)
                    if(rows->state[row]!=rows->state[boundaries[segment]])return false;
            uniform_=true;
        }
        serial_=rows->visualSerial();
        writes=0;
        if(!valid_){
            for(unsigned y=40;y<256;++y){
                const auto* value=outerRows[outerFrames[meter][y-40]];
                for(unsigned word=0;word<4;++word){
                    UBYTE* p=destination_+y*48+(word%2?38:34)+(word>=2?downwellPlaneBytes:0);
                    const UWORD mask=word%2?0xc000:0x003f;
                    const UWORD before=read(p),after=UWORD((before&~mask)|value[word]);
                    if(before!=after){write(p,after);++writes;}
                }
                write(retained+y*48+36,read(destination_+y*48+36));
                write(retained+y*48+36+downwellPlaneBytes,read(destination_+y*48+36+downwellPlaneBytes));
            }
        }else if(meter_!=meter){
            const auto* directory=patchDirectory[meter_*4+meter];
            for(unsigned i=0;i<directory[1];++i){
                const auto* patch=patches[directory[0]+i];
                UBYTE* p=destination_+patch[0];
                write(p,UWORD((read(p)&~patch[1])|patch[2]));++writes;
            }
        }
        const unsigned label=ammo*3+offset;
        const auto* glyph=labels[label];
        const unsigned config=starts[capacity-1]+downwell::mulu16(ammo*4+meter,capacity);
        for(unsigned segment=0;segment<capacity;++segment){
            unsigned y=segment==capacity-1?40:229+4*meter-boundaries[segment+1]+8;
            const unsigned bottom=segment?229+4*meter-boundaries[segment]+8:256;
            const unsigned own=rows->state[boundaries[segment]]<4?rows->state[boundaries[segment]]:3;
            const unsigned previous=segment?(rows->state[boundaries[segment-1]]<4?rows->state[boundaries[segment-1]]:3):0;
            const auto tile=pages[configurations[config+segment]][own*4+previous];
            const bool same=valid_ && meter==meter_ && tile==oldTiles_[segment];
            oldTiles_[segment]=tile;
            if(same && (label==label_ || y>=250 || bottom<=238))continue;
            const auto* directory=tiles[tile];
            for(unsigned i=0;i<directory[1];++i){
                const auto* run=runs[directory[0]+i];
                const auto* pair=pairs[run[1]];
                const unsigned last=y+run[0];
                if(same && y<238)y=last<238?last:238;
                for(;y<last;++y){
                    if(same && y>=250)break;
                    UWORD low=pair[0],high=pair[1];
                    if(y>=238 && y<250){
                        const UWORD a=glyph[(y-238)*2],b=glyph[(y-238)*2+1],mask=a|b;
                        low=UWORD((low&~mask)|a);high=UWORD((high&~mask)|b);
                    }
                    const unsigned address=y*48+36;
                    if(read(retained+address)!=low){write(retained+address,low);write(destination_+address,low);++writes;}
                    if(read(retained+address+downwellPlaneBytes)!=high){
                        write(retained+address+downwellPlaneBytes,high);write(destination_+address+downwellPlaneBytes,high);++writes;
                    }
                }
                y=last;
            }
        }
        valid_=true;meter_=meter;label_=label;
        amigaRendererClearClipRect(renderer);
        return true;
    }
};
#endif
