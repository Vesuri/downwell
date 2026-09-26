#include "core/DamageNumberText.h"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <initializer_list>
using namespace downwell;
int main(){
    for(int value:{0,1,12,2147483647,(-2147483647-1),-12}){
        DamageNumberText text;assert(text.format(MotionBinary64::fromInteger(value)));
        char expected[32];std::snprintf(expected,sizeof expected,"-%d",value);
        assert(std::strcmp(text.text,expected)==0&&text.length==std::strlen(expected));
        unsigned calls=0;
        text.draw(190,50,[&](unsigned glyph,int x,int y,unsigned pen){
            const unsigned pass=calls/text.length,index=calls%text.length;++calls;
            const int dx[]={1,-1,0,0,0},dy[]={0,0,1,-1,0};
            assert(glyph==DamageNumberText::glyph(expected[index]));
            assert(x==190-int(text.length)*4+4+int(index)*8+dx[pass]&&y==50+dy[pass]);
            assert(pen==(pass==4?2u:0u));
        });assert(calls==text.length*5);
    }
    DamageNumberText text;
    assert(!text.format({0x3fe00000u,0})&&!text.format({0x7ff00000u,0}));
    assert(!text.format({0x41e00000u,0}));
    std::puts("Damage text: exact signed integer formatting and five whole-string glyph passes passed");
}
