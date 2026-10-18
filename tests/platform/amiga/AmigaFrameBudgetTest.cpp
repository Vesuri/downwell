#include "platform/amiga/AmigaFrameBudget.h"
#include <cassert>
#include <cstdio>
int main(){
    using B=AmigaFrameBudget;
    constexpr unsigned f=B::fieldTicks;
    B quiet({10,0});
    assert(quiet.canPrepare({10,f*17/20}));
    assert(!quiet.canPrepare({10,f*18/20}));
    assert(!quiet.canPrepare({11,0}));
    B wrapped({65535,f-100});
    assert(wrapped.work({0,100})==200);
    B waiting({20,1000});
    waiting.excludeWait({20,2000},{21,3000});
    assert(waiting.work({21,4000})==2000);
    assert(waiting.canPrepare({21,4000}));
    waiting.excludeWait({21,5000},{22,5000});
    assert(waiting.work({22,6000})==4000);
    // IRQ/beam disagreement or a stale sample cannot accidentally grant an
    // unlimited preparation budget through unsigned subtraction.
    B invalid({10,2000});
    assert(!invalid.canPrepare({10,1000}));
    assert(!invalid.canPrepare({9,3000}));
    assert(!invalid.canPrepare({10,f}));
    invalid.excludeWait({10,1000},{10,0});
    assert(!invalid.canPrepare({11,1000}));
    B beyond({1,0});
    beyond.excludeWait({1,0},{2,0});
    assert(!beyond.canPrepare({1,1000}));
    std::puts("PAL row preparation budget: deadline, wait subtraction, field wrap and invalid samples");
}
