#define NO_ASSEMBLER
#include "platform/amiga/framework/BitmapCopyPlan.h"
#include <vector>
#include <iostream>
#include <cstdlib>
static void require(bool b,const char* message){if(!b){std::cerr<<message<<'\n';std::exit(1);}}
// Independent scalar model of the channels, shifters, edge masks and minterm.
void AmigaHardware::blitterCopyWithMaskOperation(uint16_t* pixels,uint16_t* target,uint16_t* mask,
    uint16_t words,uint16_t height,int16_t pm,int16_t dm,int16_t am,int16_t ps,int16_t as,
    uint16_t first,uint16_t last,bool clear,bool invert){
    const bool reverse=ps<0||as<0;const int step=reverse?-1:1;
    unsigned previousA=0,previousB=0;
    const unsigned shiftA=as<0?-as:as,shiftB=ps<0?-ps:ps;
    auto shift=[&](unsigned value,unsigned previous,unsigned amount){
        if(!amount)return value;
        return reverse?((value<<amount)|(previous>>(16-amount)))&65535u:
            ((previous<<16)|value)>>amount&65535u;
    };
    for(unsigned y=0;y<height;++y){
        for(unsigned x=0;x<words;++x){
            unsigned a=*mask,b=*pixels;
            if(!x)a&=first;if(x+1==words)a&=last;
            const unsigned coverage=shift(a,previousA,shiftA),colour=shift(b,previousB,shiftB);
            *target=(clear?0:(*target&~coverage))|(coverage&(invert?~colour:colour));
            previousA=a;previousB=b;
            mask+=step;pixels+=step;target+=step;
        }
        mask+=step*(am/2);pixels+=step*(pm/2);target+=step*(dm/2);
    }
}
int main(){
    unsigned cases=0,random=7319;
    for(unsigned restore=0;restore<2;++restore)for(unsigned sx=0;sx<=16;++sx)
    for(unsigned h:{1u,2u,17u,39u,48u})for(unsigned invert=0;invert<2;++invert){
        const unsigned stride=restore?24:11,sourceX=restore?110:sx;
        std::vector<uint16_t> pixels(stride*48+1),mask(11*48+1),target(24*320+1);
        for(auto* v:{&pixels,&mask,&target})for(auto& word:*v){random=random*1664525u+1013904223u;word=random>>16;}
        auto expected=target;
        const unsigned py=h==48?0:3,ay=h==48?0:4,dy=73;
        for(unsigned y=0;y<h;++y)for(unsigned x=0;x<160;++x){
            if(!(mask[(ay+y)*11+(sx+x)/16]&(0x8000u>>((sx+x)&15))))continue;
            const bool bit=(pixels[(py+y)*stride+(sourceX+x)/16]&(0x8000u>>((sourceX+x)&15)))!=0;
            auto& value=expected[(dy+y)*24+(110+x)/16];const unsigned m=0x8000u>>((110+x)&15);
            value=(value&~m)|((bit!=bool(invert))?m:0);
        }
        BitmapCopyPlan plan;
        require(plan.prepare(stride,11,24,110,sourceX,sx,160),"field plan rejected");
        plan.copy(pixels.data(),mask.data(),target.data(),py,ay,dy,h,invert);
        if(target!=expected){std::cerr<<restore<<' '<<sx<<' '<<h<<' '<<invert<<'\n';require(false,"planned DMA differs from pixels");}
        ++cases;
    }
    BitmapCopyPlan invalid;
    require(!invalid.prepare(11,11,24,110,17,17,160),"source overflow admission");
    require(!invalid.prepare(11,11,24,110,0,0,0),"empty admission");
    std::cout<<cases<<" prepared copy cases match scalar pixels, complement and untouched edges\n";
}
