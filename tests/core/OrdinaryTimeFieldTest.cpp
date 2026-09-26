#include "core/OrdinaryTimeField.h"
#include "core/OrdinaryTimeFields.h"
#include "core/GoalTimeField.h"
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
    for(unsigned seed=0;seed<64;++seed)for(unsigned boundary=0;boundary<=5;++boundary){
        OrdinaryTimeFields<5> fields;GoalTimeField goal;
        GameMakerRandom actual(seed),expected(seed);
        for(unsigned i=0;i<5;++i)assert(fields.adoptCreate(240,int(i),i));
        unsigned calls=0;
        assert(fields.advanceDraw(false,actual,true,boundary,[&]{++calls;goal.advanceDraw(false,actual);}));
        for(unsigned i=5;i>0;--i){
            if(i==boundary)assert(value(goal.radius)==128+double(expected.nextU32())/2147483648.0);
            assert(value(fields.at(i-1)->radius)==24+double(expected.nextU32())/2147483648.0);
        }
        if(boundary==0)assert(value(goal.radius)==128+double(expected.nextU32())/2147483648.0);
        assert(calls==1 && actual.drawCount()==expected.drawCount() && actual.stateIndex()==expected.stateIndex());
        for(unsigned i=0;i<16;++i)assert(actual.stateWord(i)==expected.stateWord(i));
        const auto goalRadius=goal.radius;
        assert(fields.advanceDraw(true,actual,true,boundary,[&]{goal.advanceDraw(true,actual);}));
        assert(actual.drawCount()==6 && goal.radius.equal(goalRadius));
        assert(!fields.advanceDraw(false,actual,true,6,[&]{assert(false);}));
        assert(actual.drawCount()==6);
        assert(fields.advanceDraw(false,actual,false,999,[&]{assert(false);}));
        assert(actual.drawCount()==11);
        fields.clear();assert(fields.advanceDraw(false,actual,true,0,[&]{goal.advanceDraw(false,actual);}));
        assert(actual.drawCount()==12);
    }
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
    std::printf("Ordinary time field: 384 merged Draw orders, 65536 exact radius draws, frozen/offscreen Draw rules, %u mask comparisons\n",checked);
}
