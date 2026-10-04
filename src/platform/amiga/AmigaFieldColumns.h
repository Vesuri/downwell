#ifndef DOWNWELL_AMIGA_FIELD_COLUMNS_H
#define DOWNWELL_AMIGA_FIELD_COLUMNS_H
#include "AmigaGoalField.h"
#include "../../../generated/downwell-field-columns.h"

namespace downwell_field_columns {
static_assert(sizeof(downwell::FieldColumnWord)==8,"prepared column word layout");
static_assert(downwellGameRingPlaneBytes==24*2*320 && downwellGameRingHeight==320 &&
              downwellPlayfieldX==110 && downwellPlayfieldWidth==160,
              "prepared column stride and clip masks must match the gameplay surface");
// Chip buffers are word aligned. Host tests retain the same big-endian byte
// layout; native drawing consumes and publishes actual planar words.
inline unsigned read(const UBYTE* bytes,unsigned word){
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
    return reinterpret_cast<const UWORD*>(bytes)[word];
#else
    return (unsigned(bytes[word*2])<<8)|bytes[word*2+1];
#endif
}
inline void write(UBYTE* bytes,unsigned word,unsigned value){
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
    reinterpret_cast<UWORD*>(bytes)[word]=value;
#else
    bytes[word*2]=value>>8;bytes[word*2+1]=value;
#endif
}
template<bool Restore> inline bool paint(const UBYTE* terrain,UBYTE* target,
    unsigned offset,unsigned low,unsigned high,unsigned clip){
    low&=clip;high&=clip;
    const unsigned mask=low|high;
    if(!mask)return false;
    for(unsigned plane=0;plane<2;++plane,offset+=downwellGameRingPlaneBytes/2){
        const unsigned colour=Restore?(read(terrain,offset)&mask):plane?high:low;
        write(target,offset,(read(target,offset)&~mask)|colour);
    }
    return true;
}
template<bool Restore> void transfer(const UBYTE* terrain,UBYTE* target,
    const downwell::TimeFieldOutlines::Circle& circle,unsigned origin,UWORD* rows){
    using namespace downwell;
    if(circle.x<=downwellPlayfieldX-132||circle.x>=downwellPlayfieldX+downwellPlayfieldWidth+132||
       circle.y<=-132||circle.y>=downwellDisplayHeight+132)return;
    int base=(circle.x-14)/16,shift=(circle.x-14)%16;
    if(shift<0){shift+=16;--base;}
    int first=6-base-(shift!=0),end=17-base;
    if(first < -8)first=-8;
    if(end>10)end=10;
    if(first>=end)return;
    static const UWORD clip[24]={0,0,0,0,0,0,0x0003,
        0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xfffc};
    const auto* frame=fieldColumnFrames[fieldColumnRadii[circle.radius-128*256]];
    const int rowBase=(int(origin)+circle.y)*24;
    for(int col=first;col<end;++col){
        const unsigned column=frame[col+8];
        const int x=base+col;
        const unsigned leftMask=clip[x],rightMask=shift?clip[x+1]:0;
        const auto* word=fieldColumnWords+fieldColumnOffsets[column];
        const auto* stop=fieldColumnWords+fieldColumnOffsets[column+1];
        for(;word!=stop;++word){
            const int y=circle.y+word->row;
            if(y<0||y>=downwellDisplayHeight)continue;
            unsigned offset=unsigned(rowBase+word->wordOffset+x);
            if(offset>=downwellGameRingPlaneBytes/2)offset-=downwellGameRingPlaneBytes/2;
            bool touched;
            if(!shift)touched=paint<Restore>(terrain,target,offset,word->low,word->high,leftMask);
            else{
                touched=paint<Restore>(terrain,target,offset,word->low>>shift,word->high>>shift,leftMask);
                touched=paint<Restore>(terrain,target,offset+1,
                    (unsigned(word->low)<<(16-shift))&0xffffu,
                    (unsigned(word->high)<<(16-shift))&0xffffu,rightMask)||touched;
            }
            if(rows&&touched){
                unsigned physical=origin+unsigned(y);
                if(physical>=downwellGameRingHeight)physical-=downwellGameRingHeight;
                rows[physical/16]|=UWORD(1u<<(physical&15));
            }
        }
    }
}
}
inline bool amigaFieldColumns(const UBYTE* terrain,UBYTE* target,
    const downwell::TimeFieldOutlines::Circle& circle,unsigned origin,bool restore,UWORD* rows=nullptr){
    if(origin>=downwellGameRingHeight||circle.radius<128*256||circle.radius>=130*256)return false;
    if(restore)downwell_field_columns::transfer<true>(terrain,target,circle,origin,nullptr);
    else downwell_field_columns::transfer<false>(terrain,target,circle,origin,rows);
    return true;
}
#endif
