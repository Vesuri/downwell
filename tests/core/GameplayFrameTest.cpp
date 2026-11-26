#include "core/GameplayFrame.h"
#include <cassert>
#include <cstdio>

static void equal(const GameplayFrame& a,const GameplayFrame& b){
    assert(a.playerMotion.deltaX==b.playerMotion.deltaX&&a.playerMotion.deltaY==b.playerMotion.deltaY);
    assert(a.ammoChanged==b.ammoChanged&&a.healthChanged==b.healthChanged);
    assert(a.currencyChanged==b.currencyChanged&&a.streakChanged==b.streakChanged);
    assert(a.bulletChanged==b.bulletChanged&&a.scrolled==b.scrolled&&a.scrollDelta==b.scrollDelta);
    assert(a.cameraXChanged==b.cameraXChanged&&a.cameraXDelta==b.cameraXDelta);
    assert(a.destroyedBlockCount==b.destroyedBlockCount);
    for(unsigned i=0;i<a.destroyedBlockCount;++i)assert(a.blockX[i]==b.blockX[i]&&a.blockY[i]==b.blockY[i]);
}
int main(){
    constexpr unsigned capacity=downwell::gameplayRepaintCapacity;
    GameplayFrame retained{};
    for(unsigned iteration=0;iteration<500;++iteration){
        GameplayFrame source{};
        assert(source.destroyedBlockCount==0&&!source.ammoChanged&&!source.healthChanged&&
            !source.currencyChanged&&!source.streakChanged&&!source.bulletChanged&&!source.scrolled&&
            !source.cameraXChanged&&!source.cameraXDelta&&!source.scrollDelta&&
            !source.playerMotion.deltaX&&!source.playerMotion.deltaY);
        source.playerMotion={int(iteration),-int(iteration)};
        source.ammoChanged=iteration&1;source.healthChanged=iteration&2;
        source.currencyChanged=iteration&4;source.streakChanged=iteration&8;
        source.bulletChanged=iteration&16;source.scrolled=iteration&32;
        source.scrollDelta=-int(iteration);source.cameraXChanged=iteration&64;
        source.cameraXDelta=int(iteration)+1;
        source.destroyedBlockCount=iteration%(capacity+1);
        for(unsigned i=0;i<source.destroyedBlockCount;++i){source.blockX[i]=iteration+i;source.blockY[i]=-int(iteration+i);}
        for(unsigned i=0;i<capacity;++i){retained.blockX[i]=12345;retained.blockY[i]=-54321;}
        retained=source;equal(retained,source);
        GameplayFrame copy(source);equal(copy,source);
        auto& alias=copy;copy=alias;equal(copy,source);
        // A retained result stays a value, not a borrowed next-update buffer.
        if(source.destroyedBlockCount)source.blockX[0]^=1;
        equal(copy,retained);
    }
    std::puts("GameplayFrame: counted payload, empty/full results, copy and self-assignment passed");
}
