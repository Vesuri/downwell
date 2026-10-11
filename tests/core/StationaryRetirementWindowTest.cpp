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
                for(int y:{first,first+128}){
                    unsigned retired=0;
                    assert(retireMainRoomObjectsChecked(1,11,1,top,
                        [&](unsigned){return LifetimeInstance{92,y,true};},
                        [&](unsigned){++retired;return true;}));
                    assert(StationaryRetirementWindow::interior(y,top)==(retired==0));
                    ++checks;
                }
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
        assert(!StationaryRetirementWindow::interior(y,0));
        auto unsupported=window.begin(12);unsupported.include(y);window.complete(unsupported);
        assert(!window.contains(0,12));
    }
    // Match the whole-set certificate's safe 32-bit camera domain. Even a
    // mathematically interior point must not bypass the retained path outside
    // the domain where both original view-offset operations are defined.
    assert(!StationaryRetirementWindow::interior(-521288,-2146828289L));
    assert(!StationaryRetirementWindow::interior(524127,2135195648L));
    for(int y:{-2147483647,-10000,0,400,2147483647})
    for(unsigned count=1;count<=32;++count)for(unsigned selected=0;selected<=count;++selected){
        StationaryRetirementWindow scalar,batch;
        auto initial=scalar.begin(1);scalar.complete(initial);batch.complete(initial);
        auto stale=scalar.begin(1);
        for(unsigned i=0;i<count;++i)scalar.append(y,i<selected,i+2);
        batch.appendSpan(y,selected!=0,count+1,count);
        assert(scalar.begin(count+1).revision==batch.begin(count+1).revision);
        for(long top:{-2146828288L,-40000000L,0L,2000000L,2135195647L})
            assert(scalar.contains(top,count+1)==batch.contains(top,count+1));
        scalar.complete(stale);batch.complete(stale);
        assert(!scalar.contains(0,1)&&!batch.contains(0,1));
    }
    StationaryRetirementWindow saturated;
    saturated.complete(saturated.begin(1));
    saturated.appendSpan(0,false,2,0xffffffffu);
    assert(saturated.begin(2).revision==0xffffffffu&&!saturated.contains(0,2));
    saturated.appendSpan(0,true,3,1);
    assert(saturated.begin(3).revision==0xffffffffu&&!saturated.contains(0,3));
    std::printf("Stationary retirement: %u exact boundary/oracle checks passed\n",checks);
}
