#include "core/GoalTimeField.h"
#include <fstream>
#include <iterator>
#include <vector>
#include <cassert>
#include <climits>
#include <cstdio>
using namespace downwell;
int main(){
    std::ifstream file("generated/downwell-goal-mask.mask",std::ios::binary);
    const std::vector<unsigned char> mask((std::istreambuf_iterator<char>(file)),{});
    assert(mask.size()==2048);
    unsigned checked=0;
    // Independent bitmap scan, not the compact row representation under test.
    auto oracle=[&](ContactBounds b){
        for(int y=-128;y<128;++y)for(int x=-128;x<128;++x){
            if(x<b.left || x>=b.right || y<b.top || y>=b.bottom)continue;
            const int sx=(x+128)/2,sy=(y+128)/2;
            if(mask[sy*16+sx/8]&(128>>(sx%8)))return true;
        }
        return false;
    };
    for(int y=-140;y<=140;y+=2)for(int x=-140;x<=140;x+=2)
        for(const auto shape:{ContactBounds{-2,2,-1,2},ContactBounds{-4,4,-4,8},ContactBounds{-7,9,-3,5}}){
            const ContactBounds b{x+shape.left,x+shape.right,y+shape.top,y+shape.bottom};
            assert(GoalTimeField::meetsRelativeBounds(b)==oracle(b));++checked;
        }
    for(const auto b:{ContactBounds{INT_MIN,INT_MAX,INT_MIN,INT_MAX},
        ContactBounds{0,0,-128,128},ContactBounds{2,1,0,10},
        ContactBounds{-128,128,128,INT_MAX},ContactBounds{INT_MIN,-128,-128,128},
        ContactBounds{-128,128,-128,-127},ContactBounds{-128,128,127,128}}){
        assert(GoalTimeField::meetsRelativeBounds(b)==oracle(b));++checked;
    }
    std::printf("Goal field: %u rectangle queries match original precise mask pixels\n",checked);
}
