#define DOWNWELL_PROBES
#include "core/RunnerSpatialBoundsUpdate.h"
#include <cassert>
#include <cstdio>
extern "C" {volatile unsigned long gDownwellProfileCounters[profileCounterCount]{};}
using Update=downwell::RunnerSpatialBoundsUpdate;
struct Index {
    unsigned inserts=0,removes=0;
    bool insert(Update::Box,unsigned){++inserts;return true;}
    bool remove(Update::Box,unsigned,bool retry,bool& removed){assert(retry);++removes;removed=true;return true;}
};
int main(){
    Index index;std::uint32_t flags=8;
    Update::Box box{};box.right.assignInteger(16);box.bottom.assignInteger(16);
    assert(Update::publish(index,1,0x18,flags,false,{},box));
    assert(Update::publish(index,1,0x18,flags,false,box,box));
    auto reversed=box;reversed.left=box.right;reversed.right=box.left;
    assert(Update::publish(index,1,0x18,flags,false,box,reversed));
    auto moved=box;moved.right.assignInteger(32);
    assert(Update::publish(index,1,0x18,flags,false,box,moved));
    assert(gDownwellProfileCounters[counterSpatialFirstPublication]==1);
    assert(gDownwellProfileCounters[counterSpatialEqualRepublication]==2);
    assert(gDownwellProfileCounters[counterSpatialChangedRepublication]==1);
    assert(index.inserts==4&&index.removes==3); // Equal updates are NOT elided.
    flags=2;assert(Update::publish(index,1,0x18,flags,false,box,box));
    flags=8;auto invalid=box;invalid.left.bits=0x7fc00000u;
    assert(!Update::publish(index,1,0x18,flags,false,invalid,box));
    assert(index.inserts==4&&index.removes==3);
    assert(gDownwellProfileCounters[counterSpatialFirstPublication]==1);
    std::puts("Spatial publication counters: admitted, normalized equal, changed and suppressed paths passed");
}
