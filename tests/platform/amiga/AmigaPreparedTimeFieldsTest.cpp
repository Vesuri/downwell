#include "platform/amiga/AmigaPreparedTimeFields.h"
#include "platform/amiga/AmigaFieldColumns.h"
#include "platform/amiga/AmigaFieldComposite.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <vector>

static void require(bool ok,const char* message){
    if(!ok){std::cerr<<message<<'\n';std::exit(1);}
}
struct Pixels {
    static constexpr bool verify=true;
    inline static bool usePrepared=true;
    inline static bool useStatic=false;
    inline static bool useColumns=true;
    inline static unsigned compositeCalls=0;
    static bool enabled(){return usePrepared;}
    static bool staticEnabled(){return useStatic;}
    static bool columnsEnabled(){return useColumns&&useStatic&&usePrepared;}
    static bool compositeEnabled(){return useColumns&&useStatic&&usePrepared;}
    static void band(const UBYTE* terrain,UBYTE* target,unsigned id,unsigned y,unsigned sx,unsigned sy,unsigned h,bool restore){
        const auto& image=downwell::fieldBandImages[id];
        const auto* low=downwell::fieldBandPixels+image.wordOffset;const auto* mask=low+11*image.height;
        for(unsigned row=0;row<h;++row)for(unsigned x=0;x<160;++x){
            const unsigned at=(sy+row)*11+(sx+x)/16,bit=0x8000u>>((sx+x)&15);
            const bool cover=(mask[at]&bit)!=0;if(restore&&!cover)continue;
            const unsigned pen=(low[at]&bit)?1:2;
            for(unsigned p=0;p<2;++p)put(target+p*15360,48,110+x,y+row,
                restore||!cover?get(terrain+p*15360,48,110+x,y+row):(pen>>p)&1);
        }
    }
    static bool retireOutside(const UBYTE* terrain,UBYTE* target,const downwell::TimeFieldOutlines::Circle& old,
        const downwell::TimeFieldOutlines::Circle& next,unsigned origin){
        return amigaFieldRetireOutside(old,next,origin,[&](unsigned id,unsigned y,unsigned sx,unsigned sy,unsigned h){
            band(terrain,target,id,y,sx,sy,h,true);return true;});
    }
    static bool compose(const UBYTE* terrain,UBYTE* target,const downwell::TimeFieldOutlines::Circle&,
        const downwell::TimeFieldOutlines::Circle& next,unsigned origin,UWORD* rows){
        ++compositeCalls;
        return amigaFieldBandParts(next,origin,[&](unsigned id,unsigned y,unsigned sx,unsigned sy,unsigned h){
            band(terrain,target,id,y,sx,sy,h,false);
            for(unsigned r=y;r<y+h;++r)rows[r/16]|=UWORD(1u<<(r&15));
        });
    }
    static bool columns(const UBYTE* terrain,UBYTE* target,const downwell::TimeFieldOutlines::Circle& circle,
        unsigned origin,bool restore,UWORD* rows){return amigaFieldColumns(terrain,target,circle,origin,restore,rows);}
    static void validateColumns(const UBYTE* terrain,UBYTE* target,const downwell::TimeFieldOutlines::Circle& circle,
        unsigned origin){
        downwell::GoalFieldOutline shape;shape.setRasterCircle(circle.x,circle.y,circle.radius);
        shape.drawClipped(110,0,270,284,[&](int x,int y,unsigned){
            const unsigned row=(origin+unsigned(y))%320;
            for(unsigned p=0;p<2;++p)require(get(terrain+p*15360,48,x,row)==get(target+p*15360,48,x,row),
                "large field has pristine underlay");
        });
    }
    static unsigned get(const UBYTE* p,unsigned stride,unsigned x,unsigned y){
        return (p[y*stride+x/8]>>(7-(x&7)))&1;
    }
    static void put(UBYTE* p,unsigned stride,unsigned x,unsigned y,unsigned value){
        const unsigned mask=0x80u>>(x&7), offset=y*stride+x/8;
        p[offset]=(p[offset]&~mask)|(value?mask:0);
    }
    static bool capture(UBYTE* saved,const UBYTE* target,const UBYTE* mask,unsigned x,unsigned y,
                        unsigned sx,unsigned sy,unsigned w,unsigned h){
        require(x+w<=384&&y+h<=320&&sx+w<=64&&sy+h<=56,"capture bounds");
        for(unsigned p=0;p<2;++p)for(unsigned row=0;row<h;++row)for(unsigned col=0;col<w;++col)
            if(get(mask,8,sx+col,sy+row))
                put(saved+p*448,8,sx+col,sy+row,get(target+p*15360,48,x+col,y+row));
        return true;
    }
    static bool masked(const UBYTE* source,const UBYTE* mask,UBYTE* target,
                       unsigned x,unsigned y,unsigned sx,unsigned sy,unsigned w,unsigned h){
        require(x+w<=384&&y+h<=320&&sx+w<=64&&sy+h<=56,"masked bounds");
        for(unsigned row=0;row<h;++row)for(unsigned col=0;col<w;++col)
            if(get(mask,8,sx+col,sy+row))for(unsigned p=0;p<2;++p)
                put(target+p*15360,48,x+col,y+row,get(source+p*448,8,sx+col,sy+row));
        return true;
    }
    static bool restoreTerrain(const UBYTE* source,const UBYTE* mask,UBYTE* target,
                       unsigned x,unsigned y,unsigned sx,unsigned sy,unsigned w,unsigned h){
        for(unsigned row=0;row<h;++row)for(unsigned col=0;col<w;++col)
            if(get(mask,8,sx+col,sy+row))for(unsigned p=0;p<2;++p)
                put(target+p*15360,48,x+col,y+row,get(source+p*15360,48,x+col,y+row));
        return true;
    }
    static void validateTerrain(const UBYTE* source,const UBYTE* mask,UBYTE* target,
                       unsigned x,unsigned y,unsigned sx,unsigned sy,unsigned w,unsigned h){
        for(unsigned row=0;row<h;++row)for(unsigned col=0;col<w;++col)
            if(get(mask,8,sx+col,sy+row))for(unsigned p=0;p<2;++p)
                require(get(target+p*15360,48,x+col,y+row)==get(source+p*15360,48,x+col,y+row),
                    "new field has pristine underlay");
    }
};
int main(){
    std::ifstream input("generated/amiga-field-images.bin",std::ios::binary);
    std::vector<UBYTE> pack((std::istreambuf_iterator<char>(input)),{});
    require(pack.size()==29264,"generated pack");
    using Pair=AmigaPreparedTimeFieldPair<Pixels>;
    auto candidate=std::make_unique<Pair>();
    auto reference=std::make_unique<AmigaTimeFieldPair>();
    std::vector<UBYTE> storage(Pair::storageBytes(pack.size()),0x81);
    require(candidate->bind(storage.data(),storage.size(),pack.data(),pack.size()),"bind");
    require(!candidate->bind(storage.data(),storage.size()-1,pack.data(),pack.size()),"short backing");
    auto malformed=pack;malformed[16]=0xff;
    require(!candidate->bind(storage.data(),storage.size(),malformed.data(),malformed.size()),"bad index");
    require(!candidate->bind(storage.data(),storage.size(),pack.data(),0xffffffffu),"overflow size");
    require(candidate->resident(storage.data(),pack.data(),pack.size()),"failed admission preserves resident bank");
    // A canary proves the resident branch performs no upload; restore it
    // before pixel tests because the DMA image contract is immutable.
    storage[Pair::headerBytes]^=0xff;
    require(candidate->bind(storage.data(),storage.size(),pack.data(),pack.size()),"resident rebind");
    require(storage[Pair::headerBytes]==UBYTE(pack[Pair::headerBytes]^0xff),"resident rebind must not recopy images");
    candidate->invalidateBank();
    require(!candidate->resident(storage.data(),pack.data(),pack.size()),"scene load invalidates resident bank");
    std::fill(storage.begin(),storage.end(),0x59);
    require(candidate->bind(storage.data(),storage.size(),pack.data(),pack.size()),"upload after scene overwrite");
    require(std::equal(pack.begin(),pack.end(),storage.begin()),"reload restores every image byte");
    auto alternate=storage;
    require(candidate->bind(alternate.data(),alternate.size(),pack.data(),pack.size()),"different destination admission");
    require(!candidate->resident(storage.data(),pack.data(),pack.size()),"old region no longer owned");
    require(candidate->bind(storage.data(),storage.size(),pack.data(),pack.size()),"restore destination");
    std::vector<UBYTE> actual[2],expected[2];
    unsigned random=192387;
    auto next=[&](){random=random*1664525u+1013904223u;return random;};
    for(unsigned b=0;b<2;++b){
        actual[b].resize(downwellGameRingBytes);
        for(auto& byte:actual[b])byte=next()>>24;
        expected[b]=actual[b];
    }
    unsigned fast=0,fallback=0;
    for(unsigned frame=0;frame<3600;++frame){
        if(frame&&frame%181==0){
            // Both a drawn and a pending overlay must be forgotten when a
            // fresh level reuses the bank. Exercise alternating bank loads.
            downwell::TimeFieldOutlines pending;pending.visible=true;pending.count=1;
            pending.circles[0]={190,140,24*256};
            require((*candidate)[1].prepare(actual[1].data(),0,pending,true,[](unsigned){return false;}),"pending before level rebind");
            if(frame%362==0){candidate->invalidateBank();std::fill(storage.begin(),storage.end(),0x59);}
            require(candidate->bind(storage.data(),storage.size(),pack.data(),pack.size()),"level rebind");
            for(unsigned ring=0;ring<2;++ring){
                (*reference)[ring].discard();
                require(!(*candidate)[ring].present(),"level rebind discards both overlays");
                std::fill(actual[ring].begin(),actual[ring].end(),0);
                expected[ring]=actual[ring];
                require((*candidate)[ring].draw()&&actual[ring]==expected[ring],"no stale pending draw after rebind");
            }
        }
        Pixels::usePrepared=frame%47!=0;
        const unsigned b=frame&1,origin=(frame*7)%320;
        downwell::TimeFieldOutlines shape;
        shape.count=frame%11==0?0:frame%13==0?2:1;
        shape.visible=shape.count!=0;
        for(unsigned i=0;i<shape.count;++i)shape.circles[i]={
            int(90+next()%210),int(next()%350)-33,
            int((frame%17==0?128:24)*256+(frame+i)%512)};
        bool dirty[320]={};
        if(frame%3==0)dirty[next()%320]=true;
        auto rowDirty=[&](unsigned row){return dirty[row];};
        require((*candidate)[b].prepare(actual[b].data(),origin,shape,true,rowDirty),"prepare");
        require((*reference)[b].prepare(expected[b].data(),origin,shape,true,rowDirty),"reference prepare");
        if(frame%7==0){ // Pending state is replaced before it has ever been drawn.
            // Oracle completes the first transaction; the optimized path may
            // skip it, but must produce the same final pixels and underlay.
            (*reference)[b].draw();
            if(shape.count)shape.circles[0].radius=24*256+(frame+19)%512;
            require((*candidate)[b].prepare(actual[b].data(),origin,shape,true,rowDirty),"reprepare");
            require((*reference)[b].prepare(expected[b].data(),origin,shape,true,rowDirty),"reference reprepare");
        }
        for(unsigned row=0;row<320;++row)if(dirty[row])for(unsigned p=0;p<2;++p)
            for(unsigned x=0;x<48;++x){const unsigned offset=p*15360+row*48+x;
                actual[b][offset]=expected[b][offset]=next()>>24;}
        require((*candidate)[b].draw(),"draw");(*reference)[b].draw();
        if(actual[b]!=expected[b]){
            std::cerr<<"frame "<<frame<<" origin "<<origin<<" count "<<shape.count<<'\n';
            for(unsigned i=0;i<shape.count;++i)std::cerr<<shape.circles[i].x<<','<<shape.circles[i].y<<','<<shape.circles[i].radius<<'\n';
            for(unsigned i=0;i<actual[b].size();++i)if(actual[b][i]!=expected[b][i]){
                std::cerr<<"offset "<<i<<" actual "<<unsigned(actual[b][i])<<" reference "<<unsigned(expected[b][i])<<'\n';break;}
            require(false,"field pixels");
        }
        fast+=(*candidate)[b].preparedImage();fallback+=!(*candidate)[b].preparedImage();
        // Unchanged frames must not re-save or repaint, including foreground
        // actors. Alter a pixel after drawing, then repeat the same frame.
        const unsigned offset=next()%actual[b].size();
        actual[b][offset]=expected[b][offset]=next()>>24;
        require((*candidate)[b].prepare(actual[b].data(),origin,shape,false,rowDirty),"same prepare");
        require((*reference)[b].prepare(expected[b].data(),origin,shape,false,rowDirty),"same reference");
        require((*candidate)[b].draw(),"same draw");(*reference)[b].draw();
        require(actual[b]==expected[b],"unchanged foreground preserved");
        if(frame%101==0){ // New scene replaces storage; binding invalidates both displays.
            require(candidate->bind(storage.data(),storage.size(),pack.data(),pack.size()),"rebind");
            for(auto& overlay:*reference)overlay.discard();
        }
    }
    require(fast>2000&&fallback>100,"both paths exercised");
    Pixels::usePrepared=true;
    require(candidate->bind(storage.data(),storage.size(),pack.data(),pack.size()),"final bind");
    for(auto& overlay:*reference)overlay.discard();
    actual[0].assign(downwellGameRingBytes,0x6c);expected[0]=actual[0];
    auto clean=[](unsigned){return false;};
    downwell::TimeFieldOutlines steady;
    steady.visible=true;steady.count=1;steady.circles[0]={190,120,24*256};
    require((*candidate)[0].prepare(actual[0].data(),250,steady,true,clean),"wrapped field");
    require((*candidate)[0].draw(),"wrapped draw");
    // A new actor in the transparent centre is not part of the saved mask.
    Pixels::put(actual[0].data(),48,190,50,1);
    Pixels::put(expected[0].data(),48,190,50,1);
    steady.circles[0].y-=4;
    require((*candidate)[0].matches(actual[0].data(),254,steady),"physical coverage retention");
    require((*candidate)[0].prepare(actual[0].data(),254,steady,false,clean),"retained prepare");
    require((*candidate)[0].draw(),"retained draw");
    downwell::TimeFieldOutlines empty;
    require((*candidate)[0].prepare(actual[0].data(),254,empty,true,clean),"remove field");
    require((*candidate)[0].draw(),"empty draw");
    require(actual[0]==expected[0],"restore mask only; preserve transparent-centre actor");
    // Changing the target must not apply another display's saved background.
    require((*candidate)[0].prepare(actual[1].data(),0,steady,true,clean),"target replacement");
    require((*reference)[0].prepare(expected[1].data(),0,steady,true,clean),"target oracle");
    require((*candidate)[0].draw(),"target draw");(*reference)[0].draw();
    require(actual[1]==expected[1],"target replacement pixels");
    std::vector<UBYTE> terrain(downwellGameRingBytes);
    for(auto& byte:terrain)byte=next()>>24;
    require(candidate->bind(storage.data(),storage.size(),pack.data(),pack.size(),terrain.data()),"terrain bind");
    for(unsigned b=0;b<2;++b){(*reference)[b].discard();actual[b]=expected[b]=terrain;}
    bool rowDebt[2][320]={};
    for(unsigned frame=0;frame<2400;++frame){
        Pixels::useStatic=frame%41!=0;
        Pixels::useColumns=frame%31!=0;
        const unsigned b=frame&1,origin=(frame*11)%320;
        const unsigned changed=next()%320;
        for(unsigned p=0;p<2;++p)for(unsigned x=0;x<48;++x)
            terrain[p*15360+changed*48+x]=next()>>24;
        rowDebt[0][changed]=rowDebt[1][changed]=true;
        downwell::TimeFieldOutlines shape;
        shape.visible=frame%19!=0;shape.count=shape.visible?(frame%17==0?2:1):0;
        for(unsigned i=0;i<shape.count;++i)shape.circles[i]={int(94+next()%196),int(next()%336)-26,
            int((frame%23==0?128:24)*256+next()%512)};
        auto dirty=[&](unsigned row){return rowDebt[b][row];};
        require((*candidate)[b].prepare(actual[b].data(),origin,shape,true,dirty),"terrain prepare");
        require((*reference)[b].prepare(expected[b].data(),origin,shape,true,dirty),"terrain oracle prepare");
        for(unsigned row=0;row<320;++row)if(rowDebt[b][row]){
            for(unsigned p=0;p<2;++p)for(unsigned x=0;x<48;++x){
                const unsigned offset=p*15360+row*48+x;
                actual[b][offset]=expected[b][offset]=terrain[offset];
            }
            rowDebt[b][row]=false;
        }
        require((*candidate)[b].draw(),"terrain draw");(*reference)[b].draw();
        require(actual[b]==expected[b],"pristine terrain equals captured-underlay pixels");
        // Moving objects are drawn after fields and owe their row next frame.
        const unsigned row=next()%320,x=next()%384;
        Pixels::put(actual[b].data(),48,x,row,1);Pixels::put(expected[b].data(),48,x,row,1);
        rowDebt[b][row]=true;
    }
    std::cout<<"2400 pristine-terrain frames match captured underlay, including dirty rows and BOB damage\n";
    (*candidate)[0].discard();(*reference)[0].discard();
    actual[0]=expected[0]=terrain;Pixels::usePrepared=Pixels::useStatic=true;
    for(unsigned frame=0;frame<13;++frame){
        downwell::TimeFieldOutlines crowded;
        crowded.count=frame==12?0:66;crowded.visible=crowded.count!=0;
        for(unsigned i=0;i<crowded.count;++i){
            const unsigned index=frame&1?65-i:i;
            crowded.circles[i]={frame%3==0?-100:190+int(index%11)-5,
                frame%3==1?4:270,int(24*256+(index*97+frame*13)%512)};
        }
        const unsigned origin=frame*23%320;
        require((*candidate)[0].prepare(actual[0].data(),origin,crowded,true,clean),"capacity prepare");
        require((*reference)[0].prepare(expected[0].data(),origin,crowded,true,clean),"capacity oracle");
        // Replace an undrawn transaction; no per-field captured state exists.
        (*reference)[0].draw();
        if(crowded.count)crowded.circles[0].radius=24*256+(frame*41)%512;
        require((*candidate)[0].prepare(actual[0].data(),origin,crowded,true,clean),"capacity reprepare");
        require((*reference)[0].prepare(expected[0].data(),origin,crowded,true,clean),"capacity oracle reprepare");
        require((*candidate)[0].draw(),"capacity draw");(*reference)[0].draw();
        require(actual[0]==expected[0],"66-field overlap, offscreen and pending replacement pixels");
    }
    std::cout<<"66-field capacity: overlap order, offscreen footprints and pending replacement match\n";
    for(unsigned frame=0;frame<96;++frame){
        downwell::TimeFieldOutlines mixed;
        mixed.count=3;mixed.visible=true;
        mixed.circles[0]={180,250,int(24*256+frame*3%512)};
        mixed.circles[1]={190,140,int(128*256+frame*7%512)};
        mixed.circles[2]={200,250,int(24*256+frame*11%512)};
        if(frame&1)std::swap(mixed.circles[0],mixed.circles[1]);
        for(unsigned i=0;i<mixed.count;++i){
            mixed.circles[i].x+=int(frame%16)-8;
            mixed.circles[i].y+=int(frame%3)*100-100;
        }
        const unsigned origin=frame*37%320;
        require((*candidate)[0].prepare(actual[0].data(),origin,mixed,true,clean),"mixed prepare");
        require((*reference)[0].prepare(expected[0].data(),origin,mixed,true,clean),"mixed oracle");
        require((*candidate)[0].draw(),"mixed draw");(*reference)[0].draw();
        require(actual[0]==expected[0],"mixed intersections and source order");
    }
    std::cout<<"96 mixed-size intersection frames: all X phases, clipping, wrap and source order match\n";
    Pixels::usePrepared=Pixels::useStatic=Pixels::useColumns=true;
    for(int x=182;x<=198;++x){
        (*candidate)[0].discard();(*reference)[0].discard();actual[0]=expected[0]=terrain;
        for(unsigned frame=0;frame<120;++frame){
            downwell::TimeFieldOutlines shape;shape.visible=true;shape.count=frame%23==0?2:1;
            shape.circles[0]={x,frame<60?140:0,int(128*256+(frame*19)%512)};
            shape.circles[1]={190,120,24*256};
            const unsigned origin=frame<60?291:103,row=(origin+15)%320;
            const bool changed=frame%7==0;
            auto dirty=[&](unsigned r){return changed&&r==row;};
            if(changed)for(unsigned p=0;p<2;++p)for(unsigned col=12;col<34;++col)
                terrain[p*15360+row*48+col]=next()>>24;
            // A retained transparent-corner foreground denies rectangle ownership.
            const bool owned=frame%11!=0;
            if(!owned){
                Pixels::put(actual[0].data(),48,112,(origin+25)%320,1);
                Pixels::put(expected[0].data(),48,112,(origin+25)%320,1);
            }
            require((*candidate)[0].prepare(actual[0].data(),origin,shape,true,dirty,owned),"composite prepare");
            require((*reference)[0].prepare(expected[0].data(),origin,shape,true,dirty),"composite oracle prepare");
            if(changed)for(unsigned p=0;p<2;++p)for(unsigned col=12;col<34;++col){
                const unsigned at=p*15360+row*48+col;actual[0][at]=expected[0][at]=terrain[at];
            }
            require((*candidate)[0].draw(),"composite draw");(*reference)[0].draw();
            require(actual[0]==expected[0],"composite preserves dirty sync, mixed-field order and denied ownership");
            // Retire the synthetic foreground before granting ownership again.
            if(!owned)for(unsigned p=0;p<2;++p){
                const unsigned at=p*15360+((origin+25)%320)*48+112/8;
                actual[0][at]=expected[0][at]=terrain[at];
            }
        }
    }
    require(Pixels::compositeCalls>1000,"fused path was not exercised");
    std::cout<<"2040 fused ownership frames: "<<Pixels::compositeCalls<<" compositions; pixels match\n";
    for(unsigned frame=0;frame<128;++frame){
        (*candidate)[0].discard();(*reference)[0].discard();actual[0]=expected[0]=terrain;
        downwell::TimeFieldOutlines shape;shape.visible=true;shape.count=1;
        shape.circles[0]={182+int(frame%17),int(frame%3)*140,int(128*256+(frame*19)%512)};
        const unsigned origin=(frame*37)%320;
        auto clean=[](unsigned){return false;};
        require((*candidate)[0].prepare(actual[0].data(),origin,shape,true,clean,true),"initial fused replacement prepare");
        require((*candidate)[0].draw(),"initial fused replacement draw");
        require((*reference)[0].prepare(expected[0].data(),origin,shape,true,clean),"initial replacement oracle");
        (*reference)[0].draw();
        shape.circles[0].radius=128*256+(frame*19+101)%512;
        require((*candidate)[0].prepare(actual[0].data(),origin,shape,true,clean,true),"pending fused replacement");
        require((*reference)[0].prepare(expected[0].data(),origin,shape,true,clean),"pending replacement oracle");
        (*reference)[0].draw();
        // A second prepare must retire/materialize the pending fused owner,
        // including when the replacement selects the masked fallback.
        if(frame%3==0){shape.visible=false;shape.count=0;}
        else if(frame%3==1){shape.count=2;shape.circles[1]={190,120,24*256};}
        else shape.circles[0].radius=128*256+(frame*19+233)%512;
        require((*candidate)[0].prepare(actual[0].data(),origin,shape,true,clean,true),"repeated fused prepare");
        require((*reference)[0].prepare(expected[0].data(),origin,shape,true,clean),"repeated oracle prepare");
        require((*candidate)[0].draw(),"repeated fused draw");(*reference)[0].draw();
        require(actual[0]==expected[0],"pending fused replacement preserves pixels and fallback ownership");
    }
    std::cout<<"128 pending fused replacements preserve pixels and fallback ownership\n";
    std::cout<<"3600 double-buffered frames: "<<fast<<" prepared, "<<fallback<<" fallback; pixels match\n";
    std::cout<<"Field bank residency: no repeated upload, explicit overwrite invalidation, destination changes and level rebind pixels match\n";
}
