#ifndef DOWNWELL_AMIGA_SELECTION_BORDER_PREPARED_H
#define DOWNWELL_AMIGA_SELECTION_BORDER_PREPARED_H
#include "AmigaSelectionLayer.h"
#include "framework/Bitmap.h"
#include "core/LevelUpgradeArt.h"

// Host-composed repetitions, not a runtime tile cache. Keep the original last
// tile's lower tail: the next repetition must not paint over a finite loop.
struct AmigaSelectionBorderPrepared {
    static unsigned word(const UBYTE* p){return unsigned(p[0])*256+p[1];}
    static unsigned longword(const UBYTE* p){return word(p)*65536u+word(p+2);}
    static const UBYTE* find(const UBYTE* pack,const UBYTE* end,unsigned id,unsigned frame){
        if(!pack||!end||end<pack||end-pack<20||longword(pack)!=0x44574150u||
            word(pack+4)!=2||word(pack+6)!=20)return nullptr;
        unsigned first=0,last=longword(pack+8);
        if(last>unsigned(end-pack-20)/24)return nullptr;
        const unsigned payload=20+last*24,key=id*65536u+frame;
        while(first<last){
            const unsigned mid=first+(last-first)/2;const auto* r=pack+20+mid*24;
            if(longword(r)<key)first=mid+1;
            else if(longword(r)>key)last=mid;
            else {
                const unsigned w=word(r+8),h=word(r+10),dma=word(r+12),
                    offset=longword(r+16),bytes=longword(r+20);
                if(!w||!h||dma<w||(dma&15)||word(r+14)!=3||offset<payload||
                    offset>unsigned(end-pack)||bytes>unsigned(end-pack)-offset||
                    bytes!=(dma/8)*h*3||bytes/3>65535)return nullptr;
                return r;
            }
        }
        return nullptr;
    }
    static void copy(Bitmap& target,const UBYTE* pack,const UBYTE* r,
                     int x,int y,unsigned sourceY,unsigned height){
        int sx=0,sy=int(sourceY),w=int(word(r+8)),h=int(height);
        if(x<0){sx=-x;w+=x;x=0;}
        if(y<0){sy-=y;h+=y;y=0;}
        if(x+w>160)w=160-x;if(y+h>608)h=608-y;
        if(w<=0||h<=0)return;
        Bitmap source(const_cast<UBYTE*>(pack+longword(r+16)),word(r+8),word(r+10),2,false,false,word(r+12));
        Bitmap mask(static_cast<UBYTE*>(source.data)+source.bitplaneSizeInBytes*2,
            source.width,source.height,1,false,false,source.dataWidth);
        target.copyWithMask(source,mask,x,y,sx,sy,sx,sy,w,h);
    }
    static bool render(UBYTE* target,const UBYTE* pack,const UBYTE* end,int area,int row){
        if(!target||row<0||row>36)return false;
        const UBYTE* tiles[4];const UBYTE* strips[4];
        const unsigned counts[4]={unsigned(row+1),unsigned(row+1),unsigned(37-row),unsigned(37-row)};
        for(unsigned i=0;i<4;++i){
            const unsigned id=downwell::levelBorderSprite(area+int(i/2)),frame=i%2?7:13;
            tiles[i]=find(pack,end,id,frame);strips[i]=find(pack,end,id,256+frame);
            if(!tiles[i]||!strips[i])return false;
            const auto* t=tiles[i];const auto* s=strips[i];
            if((word(t+10)!=16&&word(t+10)!=32)||word(s+10)!=9*16+word(t+10)-16||
                word(s+4)!=word(t+4)||word(s+6)!=word(t+6)||word(s+8)!=word(t+8))return false;
        }
        Bitmap output(target,160,608,2,false,false,176);
        output.clear(0,0,176,608);
        const int anchors[4]={0,160,8,152};
        for(unsigned i=0;i<4;++i){
            const auto* t=tiles[i];
            const int x=anchors[i]-static_cast<short>(word(t+4));
            const int y=(i<2?0:row*16)-static_cast<short>(word(t+6));
            const unsigned body=counts[i]*16;
            for(unsigned offset=0;offset<body;){
                const unsigned height=body-offset<128?body-offset:128;
                copy(output,pack,strips[i],x,y+int(offset),offset?16:0,height);
                offset+=height;
            }
            if(word(t+10)>16)copy(output,pack,t,x,y+int(body),16,word(t+10)-16);
        }
        return true;
    }
};
#endif
