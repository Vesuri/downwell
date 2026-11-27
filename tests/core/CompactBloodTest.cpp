#include "core/CompactUpgrades.h"
#include "core/ConsumptionBloodEffect.h"
#include "core/GameMakerRandom.h"
#include <cstdio>
#include <cstdlib>
// Knife and Fork's Q16 fxBlood against the binary64 ConsumptionBloodEffect
// (the VM's Create and Step, kept as the oracle): the same RNG words give
// the same frames, and the drops stay within a pixel over their 120 updates.
static void checkAt(bool ok,int line){if(!ok){std::fprintf(stderr,"Blood check failed at line %d\n",line);std::abort();}}
#define check(ok) checkAt((ok),__LINE__)
using R=downwell::MotionBinary64;
namespace {
struct Publish {
    bool gravity(R){return true;}
    bool imageSpeed(downwell::RunnerImageValue){return true;}
    bool imageIndex(downwell::RunnerImageValue){return true;}
};
struct Environment {
    bool position(downwell::RunnerImageValue,downwell::RunnerImageValue){return true;}
    R gravity(){return {0x3fc99999u,0x9999999au};}
    R fallSpeed(){return {};}
};
}
int main(){
    unsigned drops=0,samples=0,differ=0;
    for(unsigned seed=0;seed<4000;++seed){
        downwell::GameMakerRandom a(seed),b(seed),place(seed^0x5a5a5a5au);
        const int x=240+int(place.irandomSmall(300)),y=176+int(place.irandomSmall(8000));
        downwell::ConsumptionBloodEffect reference;Publish publish;Environment env;
        downwell::CompactBloodRecord fixed;
        check(reference.create(R::fromInteger(x),R::fromInteger(y),a,publish));
        check(fixed.create(x,y,b));
        check(a.nextU32()==b.nextU32());
        for(int tick=0;tick<120;++tick){
            unsigned rf,ff;
            check(reference.drawFrame(rf)&&fixed.drawFrame(ff)&&rf==ff);
            std::int32_t rx,ry;
            check(reference.x.roundPixel(rx)&&reference.y.roundPixel(ry));
            const int dx=fixed.pixelX()-rx,dy=fixed.pixelY()-ry;
            check(dx>=-1&&dx<=1&&dy>=-1&&dy<=1);
            differ+=dx!=0||dy!=0;++samples;
            check(reference.advanceImage(publish)&&fixed.advanceImage());
            check(reference.step(env)&&fixed.step());
        }
        ++drops;
    }
    std::printf("Blood Q16: %u drops, %u pixel samples within a pixel of binary64 (%u differ by one)\n",drops,samples,differ);
    check(differ*100<samples);
}
