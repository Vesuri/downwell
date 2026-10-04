#include "platform/amiga/AmigaFieldColumns.h"
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
}
