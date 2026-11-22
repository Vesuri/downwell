#ifndef DOWNWELL_CORE_EFFECT_MASK_ASSETS_H
#define DOWNWELL_CORE_EFFECT_MASK_ASSETS_H
#include "BinaryEffectSurface.h"
#include "EntryDitherMask.h"
#include "CorpseSprites.h"
#include <cstddef>
#include <cstdint>

namespace downwell {
// Borrowed immutable views of existing offline DWAP color planes and masks. No allocation,
// texture decoding, pixel conversion or target-specific types. The pack must
// remain alive and unchanged until these descriptors are discarded.
class EffectMaskAssets {
public:
    struct Asset {
        BinaryEffectSurface<>::Mask mask{};
        int originX=0,originY=0;
        const std::uint8_t* low=nullptr;
        const std::uint8_t* high=nullptr;
    };
    bool ready()const{return ready_;}
    void release(){ready_=false;pack_=nullptr;size_=count_=0;entryMask_=nullptr;}
    bool bindEntryMask(const std::uint8_t* data,std::size_t bytes){
        entryMask_=nullptr;
        if(!data||bytes!=EntryDitherMask::bytes)return false;
        entryMask_=data;return true;
    }
    // This compact bank represents the centered, unit-scale source sprite.
    // Unsupported placement fails rather than silently clipping different art.
    bool entryDither(int screenX,int screenY,Asset& out)const{
        unsigned offset;
        if(!entryMask_||screenX!=80||!EntryDitherMask::offset(screenY,offset))return false;
        const auto* mask=entryMask_+offset;
        // Pen 3 is opaque black. Alias both color planes to alpha: no zero
        // bitmap allocation, conversion, or copy is needed on the host.
        out={{mask,284*48,384,284,48},110,0,mask,mask};return true;
    }
    const Asset* smoke(unsigned frame)const{return ready_ && frame<9 ? &smoke_[frame] : nullptr;}
    const Asset* shade()const{return ready_ ? &shade_ : nullptr;}
    const Asset* currency(bool large,unsigned frame)const{
        return ready_ && frame<4 ? &currency_[large?1:0][frame] : nullptr;
    }
    // Optional offline debris frames. Old packs remain usable for smoke/gems;
    // a missing frame fails without exposing a partial descriptor.
    bool debris(unsigned sprite,unsigned frame,Asset& out)const{
        const unsigned frames=sprite==91?6:sprite==115||sprite==116?4:0;
        return ready_ && frame<frames && resolve(pack_,size_,count_,sprite,frame,out);
    }
    bool corpse(unsigned sprite,bool mirrored,Asset& out)const{
        return ready_&&corpseSpriteAdmitted(int(sprite))&&
            resolve(pack_,size_,count_,sprite+(mirrored?62000:0),0,out);
    }
    // W1.9 objDrone's sprDrone (two frames) and the explosions' quarter-turn
    // pages (turn * 256 + frame; mirrored pages at 62000 + sprite).
    bool drone(unsigned frame,Asset& out)const{
        return ready_&&frame<2&&resolve(pack_,size_,count_,131,frame,out);
    }
    // W1.9 decoyBalloon: sprDecoyBalloon 526 until its first held Step, then
    // sprHeartBalloon 701 or sprHeartBalloonPull 700 (four frames each).
    bool balloon(unsigned sprite,unsigned frame,Asset& out)const{
        return ready_&&(sprite==526||sprite==700||sprite==701)&&frame<4&&resolve(pack_,size_,count_,sprite,frame,out);
    }
    bool explosion(unsigned sprite,bool mirrored,unsigned turn,unsigned frame,Asset& out)const{
        const unsigned frames=sprite==463?12:sprite==470?8:0;
        return ready_&&frame<frames&&turn<4&&resolve(pack_,size_,count_,sprite+(mirrored?62000:0),turn*256+frame,out);
    }
    bool corpseExpiry(unsigned frame,Asset& out)const{
        return ready_&&frame<12&&resolve(pack_,size_,count_,102,frame,out);
    }
    bool bind(const std::uint8_t* pack,std::size_t size){
        // Failed rebind must not leave a formerly valid, now stale view live.
        ready_=false;
        if(!pack || size<20 || u32(pack)!=0x44574150u || u16(pack+4)!=2 || u16(pack+6)!=20)return false;
        static constexpr std::uint8_t source[8]={0xc0,0x7d,0xaf,0x5f,0xbe,0x94,0x0a,0xe0};
        for(unsigned i=0;i<8;++i)if(pack[12+i]!=source[i])return false;
        const std::uint32_t count=u32(pack+8);
        if(count>(size-20)/24)return false;
        // The producer guarantees strict sorted keys. Validate once at bind,
        // not once per effect/frame. Payloads may alias immutable whole/subblocks.
        for(std::uint32_t i=1;i<count;++i)
            if(u32(pack+20+std::size_t(i-1)*24)>=u32(pack+20+std::size_t(i)*24))return false;
        for(unsigned frame=0;frame<9;++frame)if(!resolve(pack,size,count,84,frame,smoke_[frame]))return false;
        if(!resolve(pack,size,count,117,0,shade_))return false;
        for(unsigned large=0;large<2;++large)
            for(unsigned frame=0;frame<4;++frame)
                if(!resolve(pack,size,count,133+large,frame,currency_[large][frame]))return false;
        pack_=pack;size_=size;count_=count;
        ready_=true;return true;
    }
private:
    static bool resolve(const std::uint8_t* pack,std::size_t size,std::uint32_t count,
                        unsigned id,unsigned frame,Asset& out){
            const std::size_t payload=20+std::size_t(count)*24;
            const std::uint32_t key=(id<<16)|frame;
            std::uint32_t first=0,last=count;
            while(first<last){
                const auto middle=first+(last-first)/2;
                const auto* record=pack+20+std::size_t(middle)*24;
                const auto found=u32(record);
                if(found<key){first=middle+1;continue;}
                if(found>key){last=middle;continue;}
                const unsigned width=u16(record+8),height=u16(record+10),dma=u16(record+12);
                if(!width || !height || dma<width || dma%16 || u16(record+14)!=3)return false;
                const std::size_t plane=std::size_t(dma/8)*height;
                const std::size_t offset=u32(record+16),bytes=u32(record+20);
                if(offset<payload || offset%2 || offset>size || bytes>size-offset ||
                   plane>bytes/3 || plane*3!=bytes)return false;
                out={{pack+offset+2*plane,unsigned(plane),width,height,dma/8},
                     signedWord(record+4),signedWord(record+6),pack+offset,pack+offset+plane};
                return true;
            }
            return false;
    }
    static unsigned u16(const std::uint8_t* p){return (unsigned(p[0])<<8)|p[1];}
    static std::uint32_t u32(const std::uint8_t* p){return (std::uint32_t(u16(p))<<16)|u16(p+2);}
    static int signedWord(const std::uint8_t* p){const unsigned n=u16(p);return n<32768?int(n):int(n)-65536;}
    Asset smoke_[9]{},shade_{},currency_[2][4]{};
    const std::uint8_t* pack_=nullptr;
    const std::uint8_t* entryMask_=nullptr;
    std::size_t size_=0;
    std::uint32_t count_=0;
    bool ready_=false;
};
}
#endif
