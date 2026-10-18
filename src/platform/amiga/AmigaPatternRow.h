#ifndef DOWNWELL_AMIGA_PATTERN_ROW_H
#define DOWNWELL_AMIGA_PATTERN_ROW_H
#include <cstdint>

// A row of existing offline artwork, not another pixel allocation. The three
// four-cell tile strips overlap at words 4 and 8 after their six-pixel shift.
// Admission proves tiles do not overlap live walls or each other. Decorations
// and unsupported combinations continue through the ordinary painter.
struct AmigaPatternRow {
    const std::uint8_t* walls;
    const std::uint8_t* tiles[3];
    AmigaPatternRow offset(unsigned y)const{
        return {walls+y*44,{tiles[0]?tiles[0]+y*20:nullptr,
            tiles[1]?tiles[1]+y*20:nullptr,tiles[2]?tiles[2]+y*20:nullptr}};
    }
    static std::uint16_t word(const std::uint8_t* p){
#ifdef DOWNWELL_PLATFORM_AMIGA
        return *reinterpret_cast<const std::uint16_t*>(p);
#else
        return std::uint16_t(unsigned(p[0])*256+p[1]);
#endif
    }
    void write(std::uint16_t* low,std::uint16_t* high)const{
        for(unsigned plane=0;plane<2;++plane){
            auto* output=plane?high:low;
            const auto* wall=walls+plane*22;
            const auto* a=tiles[0]+plane*10;
            const auto* b=tiles[1]+plane*10;
            const auto* c=tiles[2]+plane*10;
            for(unsigned w=0;w<11;++w){
                unsigned tile;
                if(w<4)tile=word(a+w*2);
                else if(w==4)tile=word(a+8)|word(b);
                else if(w<8)tile=word(b+(w-4)*2);
                else if(w==8)tile=word(b+8)|word(c);
                else tile=word(c+(w-8)*2);
                const unsigned clip=w==0?3:w==10?0xfffc:0xffff;
                output[w]=std::uint16_t(word(wall+w*2)|(tile&clip));
            }
        }
    }
};
static_assert(sizeof(AmigaPatternRow)<=44,"row references fit the existing raster slot");
#endif
