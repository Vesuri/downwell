#include "platform/amiga/AmigaFieldComposite.h"
#include <array>
#include <iostream>
#include <cstdlib>
static void require(bool ok,const char* why){if(!ok){std::cerr<<why<<'\n';std::exit(1);}}
using Buffer=std::array<UBYTE,downwellGameRingBytes>;
static void raster(Buffer& target,const downwell::TimeFieldOutlines::Circle& circle,unsigned origin){
    downwell::GoalFieldOutline oracle;oracle.setRasterCircle(circle.x,circle.y,circle.radius);
    oracle.drawClipped(110,0,270,284,[&](int x,int y,unsigned pen){
        const unsigned row=(origin+unsigned(y))%320,bit=0x80u>>(x&7);
        for(unsigned p=0;p<2;++p){auto& b=target[p*15360+row*48+unsigned(x)/8];
            b=(b&~bit)|((pen&(1u<<p))?bit:0);}
    });
}
int main(){
    Buffer terrain,actual,expected;unsigned rng=87129,cases=0,retiredRows=0,oldRows=0;
    for(auto& b:terrain){rng=rng*1664525+1013904223;b=rng>>24;}
    auto transfer=[&](unsigned image,unsigned y,unsigned sx,unsigned sy,unsigned h,bool retire){
        const auto& desc=downwell::fieldBandImages[image];
        const auto* white=downwell::fieldBandPixels+desc.wordOffset;
        const auto* mask=white+11*desc.height;
        for(unsigned r=0;r<h;++r)for(unsigned x=0;x<160;++x){
            const unsigned offset=(sy+r)*11+(sx+x)/16,bit=0x8000u>>((sx+x)&15);
            const bool covered=(mask[offset]&bit)!=0;
            if(retire&&!covered)continue;
            const unsigned pen=(white[offset]&bit)?1:2,destX=110+x,b=0x80u>>(destX&7);
            for(unsigned p=0;p<2;++p){const unsigned at=p*15360+(y+r)*48+destX/8;
                const unsigned value=retire||!covered?terrain[at]&b:(pen&(1u<<p))?b:0;
                actual[at]=(actual[at]&~b)|value;}
        }
    };
    for(unsigned phase=0;phase<512;++phase)for(int x=182;x<=198;++x)for(unsigned jump:{1u,19u,101u,511u}){
        const int ys[]={-130,0,140,283,413};
        const unsigned origin=(phase*17+x*31)%320;
        const int y=ys[(phase+x)%5];
        const downwell::TimeFieldOutlines::Circle old{x,y,int(128*256+phase)},
            next{x,y,int(128*256+(phase+jump)%512)};
        actual=expected=terrain;raster(actual,old,origin);raster(expected,next,origin);
        amigaFieldBandParts(old,origin,[&](unsigned,unsigned,unsigned,unsigned,unsigned h){oldRows+=h;});
        require(amigaFieldRetireOutside(old,next,origin,[&](unsigned id,unsigned row,unsigned sx,unsigned sy,unsigned h){
            retiredRows+=h;transfer(id,row,sx,sy,h,true);return true;
        }),"stationary composite rejected");
        // Models the post-sync opaque combination of background and new image,
        // not a second masked draw over the old destination.
        require(amigaFieldBandParts(next,origin,[&](unsigned id,unsigned row,unsigned sx,unsigned sy,unsigned h){
            transfer(id,row,sx,sy,h,false);
        }),"new image rejected");
        require(actual==expected,"fused geometry differs from scalar new raster");++cases;
    }
    unsigned callbacks=0;
    auto count=[&](unsigned,unsigned,unsigned,unsigned,unsigned){++callbacks;return true;};
    require(!amigaFieldRetireOutside({190,140,128*256},{191,140,128*256},0,count),"moved centre admitted");
    require(!amigaFieldRetireOutside({190,140,24*256},{190,140,128*256},0,count),"small old field admitted");
    require(!amigaFieldRetireOutside({181,140,128*256},{181,140,128*256},0,count),"unsupported X admitted");
    require(callbacks==0,"rejected geometry performed retirement");
    std::cout<<cases<<" stationary old/new cases match full scalar pixels; retiring "<<retiredRows
        <<" of "<<oldRows<<" old rectangle rows\n";
}
