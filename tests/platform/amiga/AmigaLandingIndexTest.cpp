#include "platform/amiga/AmigaLandingIndex.h"
#include <cassert>
#include <cstdio>
#include <vector>
int main() {
    AmigaLandingIndex index;
    for (unsigned mask=0;mask<65536;++mask) {
        index.clear();
        for(unsigned bit=0;bit<16;++bit) if(mask&(1u<<bit)) index.insert(127,496+bit);
        for(unsigned start=0;start<=16;++start) {
            unsigned expected=0;
            for(unsigned bit=start;bit<16;++bit) if(mask&(1u<<bit)){expected=497+bit;break;}
            assert(index.next(127,496+start)==expected);
        }
        assert(index.next(126,0)==0);
    }
    index.clear();
    std::vector<unsigned> reference(128*512);
    unsigned random=17;
    for(unsigned insertion=0;insertion<4096;++insertion) {
        random=random*1664525u+1013904223u;
        const unsigned column=(random>>16)&127, row=(random>>7)&511;
        index.insert(column,row);
        index.insert(column,row); // Idempotent duplicates.
        for(int y=row;y>=0;--y) {
            auto& next=reference[y*128+column];
            if(next && next<=row+1) break;
            next=row+1;
        }
        for(unsigned y=0;y<512;++y) assert(index[y*128+column]==reference[y*128+column]);
    }
    for(unsigned cell=0;cell<65536;++cell) assert(index[cell]==reference[cell]);
    index.insert(128,0); index.insert(0,512);
    assert(index.next(128,0)==0 && index.next(0,512)==0);
    index.clear();
    for(unsigned cell=0;cell<65536;++cell) assert(index[cell]==0);
    std::puts("Landing index: all 65536 word masks, 4096 intermediate insertion states, boundaries and reset");
}
