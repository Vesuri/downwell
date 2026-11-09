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
        int originX, originY, width, height;
        const UBYTE* pack;
        const UBYTE* end;
        const UBYTE* parts[2][4]{};
        unsigned partCount[2]{};
        void bounds(int x,int y,int flip,int& left,int& top,int& w,int& h)const {
            left=flip?x+originX-width:x-originX;top=y-originY;w=width;h=height;
        }
        bool part(unsigned index,int flip,AmigaResolvedSpritePart& out)const {
            const unsigned side=flip?1:0;
            if(index>=partCount[side])return false;
            out={pack,end,parts[side][index],partCount[side]};return true;
        }
    };
private:
    // W1.4 gun art: 592 cavern and 622 reference frames (was under 512).
    // W1.7 combo art, afterimages and upgrade icons: 936 cavern and 960
    // reference frames.
    static constexpr unsigned sprites=741,pages=8,capacity=1024;
    UWORD base_[sprites][pages]{};
    UWORD span_[sprites][pages]{};
    Frame frames_[capacity]{};
    bool valid_[capacity]{};
    bool ready_=false;
    static unsigned word(const UBYTE* p){return unsigned(p[0])*256+p[1];}
    static unsigned longword(const UBYTE* p){return word(p)*65536u+word(p+2);}
public:
    void invalidate(){ready_=false;}
    bool prepare(const UBYTE* pack,const UBYTE* end){
        ready_=false;
        for(auto& row:base_)for(auto& n:row)n=0;
        for(auto& row:span_)for(auto& n:row)n=0;
        for(auto& v:valid_)v=false;
        if(!pack||end<pack||end-pack<20||pack[0]!='D'||pack[1]!='W'||
            pack[2]!='A'||pack[3]!='P'||word(pack+4)!=2||word(pack+6)!=20)return false;
        const unsigned count=longword(pack+8);
        if(count>unsigned(end-pack-20)/24)return false;
        for(unsigned i=0;i<count;++i){
            const auto* r=pack+20+i*24;const unsigned sprite=word(r),frame=word(r+2);
            if(sprite>=sprites)continue;
            if(frame>=pages*256)return false;
            auto& span=span_[sprite][frame/256];
            if(span<=frame%256)span=frame%256+1;
        }
        unsigned used=0;
        for(unsigned s=0;s<sprites;++s)for(unsigned p=0;p<pages;++p){
            if(span_[s][p]>capacity-used)return false;
            base_[s][p]=used;used+=span_[s][p];
        }
        for(unsigned i=0;i<count;++i){
            const auto* r=pack+20+i*24;const unsigned sprite=word(r),frame=word(r+2);
            if(sprite>=sprites)continue;
            const unsigned index=base_[sprite][frame/256]+frame%256;
            if(valid_[index])return false;
            auto& f=frames_[index];f={};
            if(!amigaRendererAssetBounds(pack,end,sprite,frame,0,0,0,
                &f.originX,&f.originY,&f.width,&f.height))return false;
            f.originX=-f.originX;f.originY=-f.originY;f.pack=pack;f.end=end;
            for(unsigned side=0;side<2;++side){
                AmigaResolvedSpritePart part;
                if(!amigaRendererResolveSpritePart(&part,pack,end,sprite,frame,0,side))continue;
                if(part.partCount>4)return false;
                f.partCount[side]=part.partCount;f.parts[side][0]=part.record;
                for(unsigned p=1;p<f.partCount[side];++p){
                    if(!amigaRendererResolveSpritePart(&part,pack,end,sprite,frame,p,side)||
                        part.partCount!=f.partCount[side])return false;
                    f.parts[side][p]=part.record;
                }
            }
            valid_[index]=true;
        }
        ready_=true;return true;
    }
    const Frame* find(unsigned sprite,unsigned frame)const {
        if(!ready_||sprite>=sprites||frame>=pages*256||frame%256>=span_[sprite][frame/256])return nullptr;
        const unsigned index=base_[sprite][frame/256]+frame%256;
        return valid_[index]?&frames_[index]:nullptr;
    }
};
#endif
