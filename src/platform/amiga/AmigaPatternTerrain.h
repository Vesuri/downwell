#ifndef DOWNWELL_AMIGA_PATTERN_TERRAIN_H
#define DOWNWELL_AMIGA_PATTERN_TERRAIN_H
#include <cstdint>
#include <cstring>

// Offline wall artwork selected by live occupancy, never by predicted births.
// Mutations and unlisted seams miss the dictionary and use the scalar painter.
class AmigaPatternTerrain {
    const std::uint8_t* entries_=nullptr;
    const std::uint8_t* pixels_=nullptr;
    const std::uint8_t* overlays_=nullptr;
    const std::uint8_t* overlayPixels_=nullptr;
    unsigned overlayCount_=0;
    unsigned count_=0;
    const std::uint8_t* groups_=nullptr;
    const std::uint8_t* groupPixels_=nullptr;
    static unsigned word(const std::uint8_t* p){
#ifdef DOWNWELL_PLATFORM_AMIGA
        return *reinterpret_cast<const std::uint16_t*>(p);
#else
        return unsigned(p[0])*256+p[1];
#endif
    }
public:
    static constexpr unsigned headerBytes=14,entryBytes=8,rowBytes=44;
    bool ready()const{return entries_!=nullptr;}
    void release(){entries_=pixels_=overlays_=overlayPixels_=groups_=groupPixels_=nullptr;count_=overlayCount_=0;}
    bool bind(const std::uint8_t* data,unsigned bytes){
        release();
        if(!data||bytes<headerBytes||data[0]!='D'||data[1]!='W'||data[2]!='P'||data[3]!='T'||
            word(data+4)!=4)return false;
        const unsigned count=word(data+6),rows=word(data+8),overlays=word(data+10);
        const unsigned groupCount=word(data+12);
        const unsigned fixed=headerBytes+count*entryBytes+rows*rowBytes+overlays*20;
        if(!count||!rows||bytes<fixed)return false;
        const auto* p=data+headerBytes;
        for(unsigned i=0;i<count;++i,p+=entryBytes){
            const unsigned m=word(p),a=word(p+2),b=word(p+4);
            if(m>8191||(a&~(m&0xffe))||(b&~(m&0xffe)))return false;
            if(i){
                const auto* prev=p-entryBytes;
                const unsigned pm=word(prev),pa=word(prev+2),pb=word(prev+4);
                if(pm>m||(pm==m&&(pa>a||(pa==a&&pb>=b))))return false;
            }
            if(word(p+6)+16>rows)return false;
        }
        const auto* overlay=data+fixed-overlays*20;
        unsigned expected=0;
        for(unsigned i=0;i<overlays;++i){
            const auto* r=overlay+i*20;
            const unsigned offset=(word(r+16)<<16)|word(r+18),w=word(r+10),h=word(r+12);
            if(offset!=expected||!w||w>3||!h||h>32||word(r+4)>1||word(r+14)>15)return false;
            expected+=w*h*6;
        }
        if(!groupCount||groupCount>625||bytes!=fixed+expected+1250+groupCount*320)return false;
        const auto* groups=data+fixed+expected;
        for(unsigned i=0;i<625;++i)if(word(groups+i*2)!=65535&&word(groups+i*2)>=groupCount)return false;
        groups_=groups;groupPixels_=groups+1250;
        count_=count;entries_=data+headerBytes;pixels_=entries_+count*entryBytes;
        overlays_=overlay;overlayPixels_=data+fixed;overlayCount_=overlays;
        return true;
    }
    const std::uint8_t* tileGroup(unsigned code)const{
        if(!groups_||code>=625)return nullptr;
        const unsigned index=word(groups_+code*2);
        return index==65535?nullptr:groupPixels_+index*320;
    }
    struct Overlay {
        const std::uint8_t* pixels;
        int originX,originY;
        unsigned words,height,phase;
    };
    bool overlay(unsigned sprite,unsigned frame,bool flip,Overlay& out)const{
        // Tiny scene-local catalogue, used only for the rare decoration rows.
        for(unsigned i=0;i<overlayCount_;++i){
            const auto* p=overlays_+i*20;
            if(word(p)==sprite&&word(p+2)==frame&&word(p+4)==unsigned(flip)){
                out={overlayPixels_+((word(p+16)<<16)|word(p+18)),
                     int(std::int16_t(word(p+6))),int(std::int16_t(word(p+8))),
                     word(p+10),word(p+12),word(p+14)};
                return true;
            }
        }
        return false;
    }
    const std::uint8_t* find(unsigned middle,unsigned above,unsigned below)const{
        above&=middle&0xffe;below&=middle&0xffe;
        unsigned first=0,end=count_;
        while(first<end){
            const unsigned mid=first+(end-first)/2;
            const auto* p=entries_+mid*entryBytes;
            const unsigned m=word(p),a=word(p+2),b=word(p+4);
            if(m==middle&&a==above&&b==below)return p+6;
            if(m<middle||(m==middle&&(a<above||(a==above&&b<below))))first=mid+1;
            else end=mid;
        }
        return nullptr;
    }
    // The caller has admitted this reference with find(). Destination rows
    // retain the raster's eleven words per plane, two planes per pixel row.
    void copy(const std::uint8_t* reference,unsigned first,unsigned count,std::uint16_t* out)const{
        const auto* source=pixels_+(word(reference)+first)*rowBytes;
#ifdef DOWNWELL_PLATFORM_AMIGA
        std::memcpy(out,source,count*rowBytes);
#else
        for(unsigned w=0;w<count*22;++w)out[w]=std::uint16_t(word(source+w*2));
#endif
    }
    const std::uint8_t* row(const std::uint8_t* reference,unsigned y)const{
        return pixels_+(word(reference)+y)*rowBytes;
    }
};
#endif
