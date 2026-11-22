#ifndef DOWNWELL_AMIGA_ACTOR_FRAMES_H
#define DOWNWELL_AMIGA_ACTOR_FRAMES_H
#include "AmigaRenderer.h"

// Immutable render ownership, prepared before the first gameplay frame. This
// bank remains in ordinary memory: only the existing channel buffers use DMA.
// Frame numbers have sparse 256-frame variant pages (0, 256 and 768, and
// W1.7's afterimage pages 1024..2047).
class AmigaActorFrames {
public:
    struct Frame {
        WORD originX, originY, width, height;
        const UBYTE* pack;   // nullptr: no frame at this index
        const UBYTE* end;
        // Side 0's records, then side 1's, in the shared part pool.
        const UBYTE* const* parts;
        UBYTE partCount[2];
        void bounds(int x,int y,int flip,int& left,int& top,int& w,int& h)const {
            left=flip?x+originX-width:x-originX;top=y-originY;w=width;h=height;
        }
        bool part(unsigned index,int flip,AmigaResolvedSpritePart& out)const {
            const unsigned side=flip?1:0;
            if(index>=partCount[side])return false;
            out={pack,end,parts[side?partCount[0]+index:index],partCount[side]};return true;
        }
    };
private:
    // W1.4 gun art: 592 cavern and 622 reference frames (was under 512).
    // W1.7 combo art, afterimages and upgrade icons: 936 cavern and 960
    // reference frames. W1.10 per-style player art (styles 1-4, with
    // afterimages): 1,274 cavern and 1,298 reference frames, resolving 747
    // and 805 hardware sprite parts (most frames are CPU-drawn, with none).
    static constexpr unsigned sprites=741,pages=8,capacity=1408,partCapacity=1024;
    // W1.9's explosions (463, 470), drone (131), balloon (526, 700, 701) and jetpack flame (21-23) are drawn only on the CPU
    // effect surface, which resolves them from the pack itself
    // (tools/amiga_scene_contract.py SURFACE_ONLY_IDS): no actor frames.
    static bool surfaceOnly(unsigned sprite){
        return sprite==463||sprite==470||sprite==131||sprite==526||sprite==700||sprite==701||
            (sprite>=21&&sprite<=23);
    }
    // Frames of sprite s, page p occupy [base_[k],base_[k+1]), k=s*pages+p.
    UWORD base_[sprites*pages+1]{};
    Frame frames_[capacity]{};
    const UBYTE* parts_[partCapacity]{};
    bool ready_=false;
    static unsigned word(const UBYTE* p){return unsigned(p[0])*256+p[1];}
    static unsigned longword(const UBYTE* p){return word(p)*65536u+word(p+2);}
public:
    void invalidate(){ready_=false;}
    bool prepare(const UBYTE* pack,const UBYTE* end){
        ready_=false;
        // Count each sprite page's span into base_[k+1], then accumulate.
        for(auto& n:base_)n=0;
        for(auto& f:frames_)f.pack=nullptr;
        if(!pack||end<pack||end-pack<20||pack[0]!='D'||pack[1]!='W'||
            pack[2]!='A'||pack[3]!='P'||word(pack+4)!=2||word(pack+6)!=20)return false;
        const unsigned count=longword(pack+8);
        if(count>unsigned(end-pack-20)/24)return false;
        for(unsigned i=0;i<count;++i){
            const auto* r=pack+20+i*24;const unsigned sprite=word(r),frame=word(r+2);
            if(sprite>=sprites||surfaceOnly(sprite))continue;
            if(frame>=pages*256)return false;
            auto& span=base_[sprite*pages+frame/256+1];
            if(span<=frame%256)span=frame%256+1;
        }
        unsigned used=0;
        for(unsigned k=1;k<=sprites*pages;++k){
            if(base_[k]>capacity-used)return false;
            used+=base_[k];base_[k]=used;
        }
        unsigned partsUsed=0;
        for(unsigned i=0;i<count;++i){
            const auto* r=pack+20+i*24;const unsigned sprite=word(r),frame=word(r+2);
            if(sprite>=sprites||surfaceOnly(sprite))continue;
            const unsigned index=base_[sprite*pages+frame/256]+frame%256;
            auto& f=frames_[index];
            if(f.pack)return false;
            int originX,originY,width,height;
            if(!amigaRendererAssetBounds(pack,end,sprite,frame,0,0,0,
                &originX,&originY,&width,&height))return false;
            if(originX<-32767||originX>32767||originY<-32767||originY>32767||
                width<0||width>32767||height<0||height>32767)return false;
            f.originX=WORD(-originX);f.originY=WORD(-originY);f.width=WORD(width);f.height=WORD(height);
            f.pack=pack;f.end=end;f.parts=parts_+partsUsed;f.partCount[0]=f.partCount[1]=0;
            for(unsigned side=0;side<2;++side){
                AmigaResolvedSpritePart part;
                if(!amigaRendererResolveSpritePart(&part,pack,end,sprite,frame,0,side))continue;
                if(part.partCount>4||part.partCount>partCapacity-partsUsed)return false;
                f.partCount[side]=UBYTE(part.partCount);parts_[partsUsed++]=part.record;
                for(unsigned p=1;p<f.partCount[side];++p){
                    if(!amigaRendererResolveSpritePart(&part,pack,end,sprite,frame,p,side)||
                        part.partCount!=f.partCount[side])return false;
                    parts_[partsUsed++]=part.record;
                }
            }
        }
        ready_=true;return true;
    }
    const Frame* find(unsigned sprite,unsigned frame)const {
        if(!ready_||sprite>=sprites||frame>=pages*256)return nullptr;
        const unsigned k=sprite*pages+frame/256,index=base_[k]+frame%256;
        if(index>=base_[k+1])return nullptr;
        return frames_[index].pack?&frames_[index]:nullptr;
    }
};
#endif
