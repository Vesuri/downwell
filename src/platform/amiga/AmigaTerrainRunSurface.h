#ifndef DOWNWELL_AMIGA_TERRAIN_RUN_SURFACE_H
#define DOWNWELL_AMIGA_TERRAIN_RUN_SURFACE_H
#include "AmigaRenderer.h"
#include "AmigaPatternTerrain.h"
#include "core/NarrowArithmetic.h"

// A dirty span of the retained Fast-RAM raster, not an intermediate bitmap.
// Native words are row-interleaved: eleven PF1 words, then eleven PF2 words.
// Only publish after the entire ordered paint succeeds. Unsupported artwork
// leaves the span dirty so the retained compositor can replace it completely.
class AmigaTerrainRunSurface {
    UWORD* pixels_;
    int firstY_, height_, left_, right_;
    const UBYTE* const* tiles_;
    const UBYTE* prepared_;
    static ULONG longWord(const UBYTE* p) {
#ifdef DOWNWELL_PLATFORM_AMIGA
        return *reinterpret_cast<const ULONG*>(p);
#else
        return (ULONG(word(p))<<16)|word(p+2);
#endif
    }
    static void mergePair(UWORD* p,ULONG mask,ULONG color) {
#ifdef DOWNWELL_PLATFORM_AMIGA
        typedef ULONG AliasLong __attribute__((may_alias));
        auto* pair=reinterpret_cast<AliasLong*>(p);
        *pair=(*pair&~mask)|(color&mask);
#else
        const ULONG old=(ULONG(p[0])<<16)|p[1];
        const ULONG value=(old&~mask)|(color&mask);
        p[0]=UWORD(value>>16);p[1]=UWORD(value);
#endif
    }
    static UWORD word(const UBYTE* p) {
#ifdef DOWNWELL_PLATFORM_AMIGA
        return *reinterpret_cast<const UWORD*>(p);
#else
        return UWORD((unsigned(p[0])<<8)|p[1]);
#endif
    }
public:
    static constexpr int x=96, words=11;
    AmigaTerrainRunSurface(UWORD* pixels,int firstY,int height,int left,int width,
                          const UBYTE* const* tiles,const UBYTE* prepared=nullptr)
        :pixels_(pixels),firstY_(firstY),height_(height),left_(left),
         right_(left+width),tiles_(tiles),prepared_(prepared) {}
    void clear() {
        for(int i=0;i<height_*words*2;++i)pixels_[i]=0;
    }
    // Admitted non-overlapping tile cells: the immutable group can be ORed
    // over the wall image without per-tile clipping or mask composition.
    void addTileGroup(const UBYTE* source,unsigned column){
        for(unsigned w=0;w<5&&column+w<unsigned(words);++w){
            const unsigned dest=column+w;
            const UWORD clip=dest==0?0x0003:dest==10?0xfffc:0xffff;
            const auto* input=source+firstY_*20+w*2;
            auto* out=pixels_+dest;
            for(int y=0;y<height_;++y,input+=20,out+=words*2){
                out[0]|=word(input)&clip;
                out[words]|=word(input+10)&clip;
            }
        }
    }
    bool overlay(const AmigaPatternTerrain::Overlay& art,int anchorX,int anchorY){
        const int left=anchorX-art.originX,top=anchorY-art.originY;
        if((unsigned(left-x)&15u)!=art.phase)return false;
        const int start=left-int(art.phase);
        const int first=top>firstY_?top:firstY_;
        const int end=top+int(art.height)<firstY_+height_?top+int(art.height):firstY_+height_;
        if(first>=end)return true;
        for(unsigned col=0;col<art.words;++col){
            const int px=start+int(col)*16;
            if(px+16<=left_||px>=right_||px<x||px>=x+words*16)continue;
            const unsigned l=px<left_?unsigned(left_-px):0;
            const unsigned r=px+16>right_?unsigned(px+16-right_):0;
            const UWORD clip=UWORD((0xffffu>>l)&(0xffffu<<r));
            const auto* source=art.pixels+(downwell::mulu16(first-top,art.words)+col)*6; // 0<=first-top<height, both words
            auto* target=pixels_+(first-firstY_)*words*2+(px-x)/16;
            for(int y=first;y<end;++y,source+=art.words*6,target+=words*2){
                const UWORD mask=word(source)&clip;
                target[0]=(target[0]&UWORD(~mask))|(word(source+2)&mask);
                target[words]=(target[words]&UWORD(~mask))|(word(source+4)&mask);
            }
        }
        return true;
    }
    // Resolve a whole occupancy row once. Its cells share vertical clipping,
    // source-row offset and alignment; do not rediscover these per wall.
    void wallRow(int anchorX,int anchorY,ULONG above,ULONG middle,ULONG below,unsigned columns){
        const int tileX=anchorX-8,tileY=anchorY-8;
        const int column0=tileX>=x?(tileX-x)/16:-1;
        const int first=tileY>firstY_?tileY:firstY_;
        const int end=tileY+16<firstY_+height_?tileY+16:firstY_+height_;
        if(first>=end)return;
        if(!prepared_||tileX-x-column0*16!=6||left_!=110||right_!=270){
            for(unsigned cell=0;cell<columns;++cell){
                const ULONG bit=2UL<<cell;
                if(middle&bit)tile(((above&bit)?1:0)|((middle&(bit<<1))?2:0)|
                    ((below&bit)?4:0)|((middle&(bit>>1))?8:0),anchorX+int(cell)*16,anchorY);
            }
            return;
        }
        UWORD* const destination=pixels_+(first-firstY_)*words*2;
        const unsigned sourceOffset=(first-tileY)*12;
        for(unsigned cell=0;cell<columns;++cell){
            const ULONG bit=2UL<<cell;
            const int column=column0+int(cell);
            if(!(middle&bit)||column<0||column>=words)continue;
            const unsigned frame=((above&bit)?1:0)|((middle&(bit<<1))?2:0)|
                ((below&bit)?4:0)|((middle&(bit>>1))?8:0);
            const UBYTE* source=prepared_+frame*192+sourceOffset;
            UWORD* target=destination+column;
            if(column==10){
                for(int row=first;row<end;++row,source+=12,target+=words*2){
                    const UWORD mask=word(source)&0xfffcu;
                    target[0]=(target[0]&UWORD(~mask))|(word(source+4)&mask);
                    target[words]=(target[words]&UWORD(~mask))|(word(source+8)&mask);
                }
            }else{
                const ULONG clip=column==0?0x0003ffffUL:column==9?0xfffffffcUL:0xffffffffUL;
                for(int row=first;row<end;++row,source+=12,target+=words*2){
                    const ULONG mask=longWord(source)&clip;
                    mergePair(target,mask,longWord(source+4));
                    mergePair(target+words,mask,longWord(source+8));
                }
            }
        }
    }
    bool tile(unsigned frame,int anchorX,int anchorY) {
        if(frame>=26)return false;
        const int tileX=anchorX-8,tileY=anchorY-8;
        int first=tileY>firstY_?tileY:firstY_;
        int end=tileY+16<firstY_+height_?tileY+16:firstY_+height_;
        int left=tileX>left_?tileX:left_;
        int right=tileX+16<right_?tileX+16:right_;
        if(first>=end||left>=right)return true;
        const UWORD clip=UWORD((0xffffu>>(left-tileX))&
                              (0xffffu<<(16-(right-tileX))));
        // Floor division also covers a tile beginning just before the raster.
        const int column=(tileX-x)>=0?(tileX-x)/16:-1;
        const unsigned shift=unsigned(tileX-x-column*16);
        const UBYTE* source=tiles_[frame]+(first-tileY)*2;
        UWORD* row=pixels_+(first-firstY_)*words*2;
        if(prepared_&&shift==6&&column>=0&&column+1<words){
            const ULONG coverage=ULONG(clip)<<10;
            const UBYTE* ready=prepared_+frame*192+(first-tileY)*12;
            UWORD* destination=row+column;
            for(int y=first;y<end;++y,ready+=12,destination+=words*2){
                const ULONG mask=longWord(ready)&coverage;
                mergePair(destination,mask,longWord(ready+4));
                mergePair(destination+words,mask,longWord(ready+8));
            }
            return true;
        }
        for(int y=first;y<end;++y,source+=2,row+=words*2){
            const UWORD low=word(source),high=word(source+32);
            const UWORD mask=(low|high)&clip;
            if(column>=0&&column<words){
                const UWORD m=UWORD(mask>>shift);
                row[column]=(row[column]&UWORD(~m))|(UWORD(low>>shift)&m);
                row[column+words]=(row[column+words]&UWORD(~m))|(UWORD(high>>shift)&m);
            }
            if(shift&&column+1>=0&&column+1<words){
                const UWORD m=UWORD(unsigned(mask)<<(16-shift));
                row[column+1]=(row[column+1]&UWORD(~m))|(UWORD(unsigned(low)<<(16-shift))&m);
                row[column+1+words]=(row[column+1+words]&UWORD(~m))|
                    (UWORD(unsigned(high)<<(16-shift))&m);
            }
        }
        return true;
    }
};
#endif
