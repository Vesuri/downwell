#include "platform/amiga/AmigaPatternRow.h"
#include <cassert>
#include <cstdio>
#include <cstdint>

int main(){
    std::uint32_t random=0x19780523;
    auto byte=[&](){random=random*1664525u+1013904223u;return std::uint8_t(random>>24);};
    std::uint8_t walls[44],tiles[3][20];
    for(unsigned trial=0;trial<10000;++trial){
        for(auto& value:walls)value=byte();
        for(auto& group:tiles)for(auto& value:group)value=byte();
        AmigaPatternRow row{walls,{tiles[0],tiles[1],tiles[2]}};
        std::uint16_t actual[2][13],expected[2][11];
        for(auto& plane:actual)for(auto& value:plane)value=0xa55a;
        // Independent sequential wall copy plus three group overlays: the
        // existing materialized representation, including both overlap words.
        for(unsigned p=0;p<2;++p){
            for(unsigned w=0;w<11;++w)expected[p][w]=unsigned(walls[p*22+w*2])*256+walls[p*22+w*2+1];
            for(unsigned group=0;group<3;++group)for(unsigned w=0;w<5&&group*4+w<11;++w){
                const unsigned column=group*4+w;
                const unsigned clip=column==0?3:column==10?0xfffc:0xffff;
                expected[p][column]|=(unsigned(tiles[group][p*10+w*2])*256+tiles[group][p*10+w*2+1])&clip;
            }
        }
        row.write(actual[0]+1,actual[1]+1);
        for(unsigned p=0;p<2;++p){
            assert(actual[p][0]==0xa55a&&actual[p][12]==0xa55a);
            for(unsigned w=0;w<11;++w)assert(actual[p][w+1]==expected[p][w]);
        }
    }
    std::puts("Pattern row references: 10000 direct/materialized comparisons and guard words passed");
}
