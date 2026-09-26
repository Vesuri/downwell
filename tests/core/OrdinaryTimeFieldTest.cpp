#include "core/OrdinaryTimeField.h"
#include "core/DeferredInstanceQueue.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>
using namespace downwell;
static double value(HudReal n){return std::ldexp(double(n.mantissa),n.exponent-52);}
int main(){
    GameMakerRandom rng(42),reference(42);
    for(unsigned i=0;i<65536;++i){
        const auto n=OrdinaryTimeField::nextRadius(rng);
        assert(value(n)==24+double(reference.nextU32())/2147483648.0);
    }
    assert(rng.drawCount()==65536);
    for(std::uint32_t word:{0u,1u,0x7fffffffu,0x80000000u,0xffffffffu}){
        DeferredCreateInstance command(82,240,512);
        command.setTimeFieldRadiusWord(word);
        assert(command.timeFieldRadiusWord()==word);
        OrdinaryTimeField adopted;
        adopted.adoptCreate(command.x,command.y,command.timeFieldRadiusWord());
        assert(value(adopted.radius)==24+double(word)/2147483648.0);
        assert(rng.drawCount()==65536);
    }
    OrdinaryTimeField field;field.create(240,512,rng);
    const auto radius=field.radius;
    const auto draws=rng.drawCount();
    field.advanceDraw(true,rng);
    assert(rng.drawCount()==draws && value(field.radius)==value(radius));
    unsigned calls=0;
    auto circle=[&](int x,int y,HudReal r,bool red){
        assert(x==240 && y==512);
        assert(value(r)==value(field.radius)+(calls==0?0:calls==1?-1:1));
        assert(red==(calls==2));++calls;
    };
    field.drawSnapshot(160,480,circle);field.drawSnapshot(320,480,circle);
    assert(calls==0 && rng.drawCount()==draws);
    field.advanceDraw(false,rng); // Offscreen Draw still consumes its random sample.
    assert(rng.drawCount()==draws+1);
    field.drawSnapshot(240,480,circle);
    assert(calls==3 && rng.drawCount()==draws+1);
    std::ifstream file("generated/downwell-time-field-mask.mask",std::ios::binary);
    const std::vector<unsigned char> raw((std::istreambuf_iterator<char>(file)),{});
    assert(raw.size()==288);
    unsigned checked=0,hits=0;
    for(int y=-40;y<=40;++y)for(int x=-40;x<=40;++x){
        bool expected=false;
        for(int py=y-4;py<y+8;++py)for(int px=x-4;px<x+4;++px){
            if(px < -24 || px>=24 || py < -24 || py>=24)continue;
            const int sx=px+24,sy=py+24;
            expected|=(raw[sy*6+sx/8]&(128>>(sx%8)))!=0;
        }
        const bool actual=OrdinaryTimeField::meetsRelativeBounds({x-4,x+4,y-4,y+8});
        assert(actual==expected);++checked;hits+=actual;
    }
    assert(hits && hits<checked);
    assert(OrdinaryTimeField::meetsRelativeBounds({-2147483647,2147483647,-2147483647,2147483647}));
    assert(!OrdinaryTimeField::meetsRelativeBounds({0,0,0,1}));
    assert(!OrdinaryTimeField::meetsRelativeBounds({-2147483647,-2147483646,0,1}));
    std::printf("Ordinary time field: 65536 exact radius draws, frozen/offscreen Draw rules, %u mask comparisons\n",checked);
}
