#ifndef DOWNWELL_CORE_DAMAGE_NUMBER_TEXT_H
#define DOWNWELL_CORE_DAMAGE_NUMBER_TEXT_H
#include "MotionBinary64.h"
namespace downwell {
// Integer damage used by the integrated encounter domain. Reject unsupported
// VM real formatting rather than silently rounding it to another damage value.
struct DamageNumberText {
    char text[13]{};unsigned length=0;
    std::int32_t integerValue=0;
    bool format(MotionBinary64 value){
        if(value.high==0xc1e00000u&&!value.low){formatInteger(-2147483647-1);return true;}
        std::int32_t integer;
        if(!value.roundPixel(integer))return false;
        const auto exact=MotionBinary64::fromInteger(integer);
        if(!value.zero()&&(value.high!=exact.high||value.low!=exact.low))return false;
        formatInteger(integer);return true;
    }
    void formatInteger(std::int32_t value){
        integerValue=value;
        length=0;text[length++]='-';
        std::uint32_t magnitude=static_cast<std::uint32_t>(value);
        if(value<0){text[length++]='-';magnitude=0u-magnitude;}
        char reversed[10];unsigned digits=0;
        do{reversed[digits++]=char('0'+magnitude%10);magnitude/=10;}while(magnitude);
        while(digits)text[length++]=reversed[--digits];text[length]=0;
    }
    static unsigned glyph(char c){return c=='-'?65110u:65100u+unsigned(c-'0');}
    // Font 2 advances eight pixels, with no kerning for these characters.
    // Each packed glyph has origin (4,0). Emit whole-string outline layers.
    template<class Draw> void draw(int x,int y,Draw draw)const{
        const int dx[]={1,-1,0,0,0},dy[]={0,0,1,-1,0};
        for(unsigned pass=0;pass<5;++pass)
            for(unsigned i=0;i<length;++i)
                draw(glyph(text[i]),x+(1-int(length))*4+int(i)*8+dx[pass],y+dy[pass],pass==4?2u:0u);
    }
};
}
#endif
