#pragma once
#include "core/EffectMaskAssets.h"
#include "core/PaletteEffectSurface.h"

// Immutable offline streams, indexed once by their original planar identity.
// Queries and producer coverage remain owned by PaletteEffectSurface.
class AmigaEffectStampBank {
public:
    static constexpr unsigned payloadOffset=388;
    struct Entry {
        downwell::EffectMaskAssets::Asset original{};
        const unsigned char* pixels=nullptr;
        const unsigned char* mask=nullptr;
    };
private:
    Entry entries_[17]{};
    unsigned char slots_[64]{};
    bool ready_=false;
    static unsigned word(const unsigned char* p){return unsigned(p[0])*256+p[1];}
    static unsigned long dword(const unsigned char* p){return (static_cast<unsigned long>(word(p))<<16)|word(p+2);}
    static unsigned hash(const void* p){return (reinterpret_cast<unsigned long>(p)>>4)&63;}
public:
    bool bind(const unsigned char* bank,unsigned bytes,const downwell::EffectMaskAssets& assets){
        return bindPayload(bank,bytes,bank&&bytes>=payloadOffset?bank+payloadOffset:nullptr,
            bytes>=payloadOffset?bytes-payloadOffset:0,assets);
    }
    // Metadata is CPU-only; the independently resident payload is the sole
    // DMA source. Its offsets retain the on-disk coordinate system.
    bool bindPayload(const unsigned char* bank,unsigned bytes,const unsigned char* payload,
                     unsigned payloadBytes,const downwell::EffectMaskAssets& assets){
        ready_=false;for(auto& slot:slots_)slot=0;
        if(!bank||!payload||bytes<payloadOffset||payloadBytes!=bytes-payloadOffset||
           bank[0]!='D'||bank[1]!='W'||bank[2]!='E'||bank[3]!='F'||
           word(bank+4)!=1||word(bank+6)!=17||dword(bank+8)!=20||dword(bank+12)!=388)return false;
        unsigned end=388;
        for(unsigned i=0;i<17;++i){
            const auto* r=bank+48+i*20;
            const unsigned sprite=word(r),frame=word(r+2),width=word(r+4),height=word(r+6);
            if(sprite!=(i<9?84:133+(i-9)/4)||frame!=(i<9?i:(i-9)%4))return false;
            const unsigned stride=dword(r+8),pixels=dword(r+12),mask=dword(r+16);
            const auto* source=sprite==84?assets.smoke(frame):
                sprite==133||sprite==134?assets.currency(sprite==134,frame):nullptr;
            if(!source||!width||!height||height>32||stride>8||
               source->mask.width!=width||source->mask.height!=height||source->mask.stride!=stride)return false;
            const unsigned span=stride*height*3;
            if(pixels!=end||mask!=pixels+span||mask>bytes||span>bytes-mask)return false;
            entries_[i]={*source,payload+(pixels-payloadOffset),payload+(mask-payloadOffset)};end=mask+span;
            unsigned slot=hash(source->mask.data);
            while(slots_[slot])slot=(slot+1)&63;
            slots_[slot]=i+1;
        }
        return ready_=end==bytes;
    }
    const Entry* find(const downwell::PaletteEffectSurface<>::Mask& mask,
                      const unsigned char* low,const unsigned char* high)const{
        if(!ready_)return nullptr;
        for(unsigned slot=hash(mask.data);slots_[slot];slot=(slot+1)&63){
            const auto& entry=entries_[slots_[slot]-1];const auto& original=entry.original;
            if(original.mask.data==mask.data&&original.low==low&&original.high==high&&
               original.mask.width==mask.width&&original.mask.height==mask.height&&
               original.mask.stride==mask.stride&&original.mask.bytes==mask.bytes)return &entry;
        }
        return nullptr;
    }
};
