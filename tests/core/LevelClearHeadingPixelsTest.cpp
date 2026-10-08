#include "core/LevelClearHeadingPixels.h"
#include "core/LevelClearRaster.h"
#include "core/LevelClearTrig.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <initializer_list>
using namespace downwell;
static void check(std::uint32_t q, bool portable) {
    LevelClearHeadingPixels pixels;
    assert(pixels.compute(q));
    assert(pixels.composedFrame(false) == (pixels.nearOffset[6] ? pixels.nearOffset[6]+1 : 0));
    bool diagonal = true;
    for(unsigned i=0;i<=6;++i) diagonal &= pixels.nearOffset[i]==pixels.farOffset[i];
    assert(pixels.composedFrame(true) == (diagonal ? pixels.composedFrame(false) : -1));
    const auto amplitude=LevelClearTrig::amplitude(q);
    for(unsigned pass=0;pass<=6;++pass) {
        const int bases[]={80,96,278};
        const int actual[]={pixels.x(pass),pixels.stageY(pass),pixels.choiceY(pass)};
        for(unsigned n=0;n<3;++n) {
            const double offset=double(q)*pass/1073741824.0;
            assert(actual[n]==int(std::floor(float(bases[n]-offset))));
            if(portable) {
                const LevelClearCoordinate delta{amplitude.timesSmall(pass),true};
                assert(actual[n]==LevelClearRaster::floorBinary32(LevelClearCoordinate::integer(bases[n]).add(delta)));
            }
        }
    }
}
int main() {
    check(0,true); check(1u<<30,true);
    for(unsigned degree=0;degree<360;++degree)
        check(LevelClearTrig::sineQ30(LevelClearTrig::degreePhase(degree)+0x40000000u),true);
    // Exercise both float precisions at integer and halfway boundaries for
    // every pass, including neighbours that straddle an exact tie.
    for(unsigned pass=1;pass<=6;++pass) for(unsigned whole=0;whole<=pass;++whole)
        for(unsigned boundary : {0u,4096u,16384u}) {
            const std::uint64_t center=((std::uint64_t(whole)<<30)+boundary)/pass;
            for(int neighbour=-2;neighbour<=2;++neighbour) {
                const std::int64_t q=std::int64_t(center)+neighbour;
                if(q>=0 && q<=1073741824) check(std::uint32_t(q),true);
            }
        }
    std::uint32_t random=17;
    for(unsigned i=0;i<1000000;++i) {
        random=random*1664525u+1013904223u;
        check(random & ((1u<<30)-1),i<10000);
    }
    LevelClearHeadingPixels invalid;
    assert(!invalid.compute((1u<<30)+1));
    invalid.nearOffset[0]=1;
    assert(invalid.composedFrame(false)==-1);
    invalid={}; invalid.nearOffset[3]=2;
    assert(invalid.composedFrame(false)==-1);
    invalid={}; invalid.farOffset[3]=1;
    assert(invalid.composedFrame(true)==-1);
    std::puts("1,000,000 Q30 heading samples, all 360 angles and rounding-boundary neighbours match");
}
