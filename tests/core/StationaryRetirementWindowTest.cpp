#include "core/StationaryRetirementWindow.h"
#include "core/MainRoomLifetime.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
int main() {
    using namespace downwell;
    StationaryRetirementWindow window;
    assert(!window.contains(0,1));
    unsigned checks=0;
    for(int first : {-10000,0,176,300000}) {
        auto proof=window.begin(9);
        proof.include(first);proof.include(first+128);
        window.complete(proof);
        for(int pixel=first-3002;pixel<=first+162;++pixel)
            for(int fraction : {-1,0,1}) {
                const long top=long(pixel)*4096+fraction;
                unsigned destroyed=0;
                assert(retireMainRoomObjectsChecked(2,11,1,top,
                    [&](unsigned i){return LifetimeInstance{93,first+int(i)*128,true};},
                    [&](unsigned){++destroyed;return true;}));
                assert(window.contains(top,9)==(destroyed==0));++checks;
            }
        assert(!window.contains(first*4096L,10)); // A different phase birth ceiling.
        window.invalidate();assert(!window.contains(first*4096L,9));
    }
    auto stale=window.begin(10);window.invalidate();window.complete(stale);
    assert(!window.contains(0,10)); // A mutation during the retained walk.
    auto empty=window.begin(11);window.complete(empty);assert(window.contains(0,11));
    window.append(400,true,12);assert(window.contains(0,12));
    window.append(-10000,false,13);assert(window.contains(0,13)); // Unrelated wall.
    auto duringBirth=window.begin(13);
    window.append(-10000,true,14);assert(!window.contains(0,14));
    window.complete(duringBirth);assert(!window.contains(0,13));
    for(int y : {-2147483647,2147483647}) {
        auto unsupported=window.begin(12);unsupported.include(y);window.complete(unsupported);
        assert(!window.contains(0,12));
    }
    std::printf("Stationary retirement: %u exact boundary/oracle checks passed\n",checks);
}
