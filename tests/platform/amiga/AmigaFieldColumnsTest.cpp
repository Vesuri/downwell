#include "platform/amiga/AmigaFieldColumns.h"
#include "platform/amiga/AmigaFieldBands.h"
#include <array>
#include <cstdlib>
#include <iostream>

static void require(bool ok,const char* what){if(!ok){std::cerr<<what<<'\n';std::exit(1);}}
int main(){
    std::array<UBYTE,downwellGameRingBytes> terrain,actual,expected;
    unsigned random=98761;
    for(auto& byte:terrain){random=random*1664525u+1013904223u;byte=random>>24;}
    unsigned cases=0;
    for(unsigned test=0;test<4;++test)for(unsigned radius=0;radius<512;++radius)for(unsigned phase=0;phase<16;++phase){
        const int xs[4]={110,176,256,176};
        const int ys[5]={-126,0,140,283,410};
        const downwell::TimeFieldOutlines::Circle circle{
            xs[test]+int(phase),test==3?ys[(radius+phase)%5]:140,int(128*256+radius)};
        const unsigned origin=(radius*17+phase*23)%320;
        actual=expected=terrain;
        UWORD rows[20]={},oracleRows[20]={};
        downwell::GoalFieldOutline oracle;
        oracle.setRasterCircle(circle.x,circle.y,circle.radius);
        oracle.drawClipped(110,0,270,284,[&](int x,int y,unsigned pen){
            const unsigned row=(origin+unsigned(y))%320;
            oracleRows[row/16]|=UWORD(1u<<(row&15));
            for(unsigned p=0;p<2;++p){
                const unsigned offset=p*15360+row*48+unsigned(x)/8,bit=0x80u>>(x&7);
                expected[offset]=(expected[offset]&~bit)|((pen&(1u<<p))?bit:0);
            }
        });
        require(amigaFieldColumns(terrain.data(),actual.data(),circle,origin,false,rows),"draw admission");
        if(actual!=expected){std::cerr<<radius<<' '<<phase<<' '<<circle.x<<' '<<circle.y<<'\n';require(false,"column pixels");}
        for(unsigned row=0;row<20;++row)require(rows[row]==oracleRows[row],"column row footprint");
        require(amigaFieldColumns(terrain.data(),actual.data(),circle,origin,true),"restore admission");
        require(actual==terrain,"column restoration");++cases;
    }
    std::cout<<cases<<" large-field radius/shift cases match independent pixels, row footprints and restoration\n";
    unsigned bands=0;
    for(unsigned phase=0;phase<512;++phase)for(int x=182;x<=198;++x){
        const int ys[5]={-126,0,140,283,410};
        const downwell::TimeFieldOutlines::Circle circle{x,ys[(phase+x)%5],int(128*256+phase)};
        const unsigned origin=(phase*71+x*17)%320;
        actual=expected=terrain;
        downwell::GoalFieldOutline oracle;oracle.setRasterCircle(x,circle.y,circle.radius);
        oracle.drawClipped(110,0,270,284,[&](int px,int y,unsigned pen){
            const unsigned row=(origin+unsigned(y))%320,bit=0x80u>>(px&7);
            for(unsigned p=0;p<2;++p){const unsigned at=p*15360+row*48+unsigned(px)/8;
                expected[at]=(expected[at]&~bit)|((pen&(1u<<p))?bit:0);}
        });
        for(unsigned restore=0;restore<2;++restore){
            require(amigaFieldBandParts(circle,origin,[&](unsigned id,unsigned y,unsigned sx,unsigned sy,unsigned h){
                const auto& image=downwell::fieldBandImages[id];
                const auto* low=downwell::fieldBandPixels+image.wordOffset;
                const auto* mask=low+11*image.height;
                for(unsigned row=0;row<h;++row)for(unsigned px=0;px<160;++px){
                    const unsigned source=(sy+row)*11+(sx+px)/16,b=0x8000u>>((sx+px)&15);
                    if(!(mask[source]&b))continue;
                    const unsigned destX=110+px,bit=0x80u>>(destX&7);
                    const unsigned pen=(low[source]&b)?1:2;
                    for(unsigned p=0;p<2;++p){const unsigned at=p*15360+(y+row)*48+destX/8;
                        actual[at]=(actual[at]&~bit)|(restore?(terrain[at]&bit):(pen&(1u<<p))?bit:0);}
                }
            }),"resident band admission");
            require(actual==(restore?terrain:expected),"resident band draw/restore pixels");
        }
        ++bands;
    }
    require(!amigaFieldBandParts({181,140,128*256},0,[](unsigned,unsigned,unsigned,unsigned,unsigned){}),"left band fallback");
    require(!amigaFieldBandParts({199,140,128*256},0,[](unsigned,unsigned,unsigned,unsigned,unsigned){}),"right band fallback");
    std::cout<<bands<<" resident band phases/placements match draw/restore pixels, clipping and wrap\n";
}
