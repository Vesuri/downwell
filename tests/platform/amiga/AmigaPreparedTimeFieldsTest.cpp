#include "platform/amiga/AmigaPreparedTimeFields.h"
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
    inline static bool usePrepared=true;
    static bool enabled(){return usePrepared;}
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
    std::cout<<"3600 double-buffered frames: "<<fast<<" prepared, "<<fallback<<" fallback; pixels match\n";
}
