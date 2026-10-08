#include "core/PackedMaskIntersection.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <vector>
using downwell::PackedMaskIntersection;
int main(){
    unsigned state=0x87654321u;
    const auto random=[&](){state^=state<<13;state^=state>>17;state^=state<<5;return state;};
    unsigned overlaps=0,empty=0;
    for(unsigned trial=0;trial<200000;++trial){
        const unsigned aw=1+random()%97,ah=1+random()%25,bw=1+random()%97,bh=1+random()%25;
        const unsigned as=(aw+7)/8+random()%4,bs=(bw+7)/8+random()%4;
        std::vector<std::uint8_t> a(as*ah),b(bs*bh);
        for(auto& value:a)value=trial%3?std::uint8_t(random()):0;
        for(auto& value:b)value=trial%3?std::uint8_t(random()):255;
        // Sparse masks and empty masks force complete scans; arbitrary padding
        // must never contribute a hit outside the clipped image width.
        if(trial%3==1){
            for(auto& value:a)value&=std::uint8_t(random()&random()&random());
            for(auto& value:b)value&=std::uint8_t(random()&random()&random());
        }
        const int ax=int(random()%81)-40,ay=int(random()%41)-20;
        const int bx=int(random()%81)-40,by=int(random()%41)-20;
        const int left=std::max({0,ax,bx}),top=std::max({0,ay,by});
        const int right=std::min({trial&1?31:160,ax+int(aw),bx+int(bw)});
        const int bottom=std::min({trial&1?29:284,ay+int(ah),by+int(bh)});
        bool reference=false;
        for(int y=top;y<bottom;++y)for(int x=left;x<right;++x)
            if((a[(y-ay)*as+(x-ax)/8]&(128u>>((x-ax)&7)))&&
               (b[(y-by)*bs+(x-bx)/8]&(128u>>((x-bx)&7))))reference=true;
        assert(reference==PackedMaskIntersection::intersects(
            {a.data(),unsigned(a.size()),aw,ah,as},ax,ay,
            {b.data(),unsigned(b.size()),bw,bh,bs},bx,by,left,top,right,bottom));
        if(reference)++overlaps;else ++empty;
    }
    assert(overlaps&&empty);
    std::printf("Packed mask intersections: 200000 scalar comparisons (%u hits, %u misses)\n",overlaps,empty);
}
