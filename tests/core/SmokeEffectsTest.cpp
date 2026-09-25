#include "core/SmokeEffects.h"
#include "core/GameMakerRandom.h"
#include <vector>
#include <cstdlib>
#include <cstdio>
static void check(bool ok){if(!ok)std::abort();}
static bool same(downwell::MotionBinary64 a,downwell::MotionBinary64 b){return a.high==b.high && a.low==b.low;}
int main(){
    unsigned births=0;
    for(unsigned seed=0;seed<128;++seed){
        downwell::SmokeEffects<16> store;
        downwell::GameMakerRandom rng(seed),reference(seed);
        const auto prior=store.beginPhase();
        check(store.skeletonBurst({}, {},rng));
        const unsigned count=4+reference.irandom(2);
        std::vector<downwell::SmokeEffect> expected(count);
        for(auto& effect:expected){
            const int y=int(reference.nextU32()%5)-2,x=int(reference.nextU32()%5)-2;
            check(effect.create(downwell::MotionBinary64::fromInteger(x),downwell::MotionBinary64::fromInteger(y),reference));
        }
        check(store.size()==count && store.advanceAnimation(prior) && store.step(prior,rng));
        for(unsigned tick=0;tick<100;++tick){
            const auto phase=store.beginPhase();
            check(store.advanceAnimation(phase));
            for(auto& effect:expected)if(!effect.destroyRequested)check(effect.advanceImage());
            // Explicit newest-first reference, independent of reusable slots.
            check(store.step(phase,rng));
            for(auto it=expected.rbegin();it!=expected.rend();++it)if(!it->destroyRequested)
                check(it->step(reference,[](auto speed,auto angle,auto& x,auto& y){
                    return downwell::SmokeDirection::vector(speed,angle,x,y,downwell::smokeTrig);
                }));
            unsigned live=0;
            for(unsigned i=0;i<count;++i){
                const auto* actual=store.get(i+1);const auto& wanted=expected[i];
                check(bool(actual)==!wanted.destroyRequested);
                if(actual){++live;check(same(actual->xx,wanted.xx) && same(actual->yy,wanted.yy));
                    check(actual->imageIndex.bits==wanted.imageIndex.bits && actual->imageSpeed.bits==wanted.imageSpeed.bits);}
            }
            check(store.size()==live && rng.drawCount()==reference.drawCount());
            for(unsigned i=0;i<16;++i)check(rng.stateWord(i)==reference.stateWord(i));
        }
        check(store.size()==0);births+=count;
    }
    downwell::SmokeEffects<2> reuse;downwell::GameMakerRandom rng(42);
    const auto a=reuse.create({}, {},rng),b=reuse.create({}, {},rng);
    const auto before=reuse.beginPhase();const auto draws=rng.drawCount();
    check(!reuse.create({}, {},rng) && draws==rng.drawCount());
    reuse.get(a)->destroyRequested=true;reuse.retireRequested();
    const auto c=reuse.create({}, {},rng);check(c>b && !reuse.get(a));
    check(reuse.step(before,rng) && reuse.get(b)->allSet && !reuse.get(c)->allSet);
    std::vector<unsigned> order;reuse.visitNewest([&](unsigned handle,const auto&){order.push_back(handle);});
    check(order==std::vector<unsigned>({c,b}));
    const auto stale=reuse.beginPhase();reuse.clear();
    const auto d=reuse.create({}, {},rng);check(d>c && !reuse.get(b) && !reuse.get(c));
    check(reuse.advanceAnimation(stale) && reuse.step(stale,rng) && !reuse.get(d)->allSet);
    reuse.get(d)->destroyRequested=true;
    const auto beforeRetirement=rng.drawCount();
    check(reuse.advanceAnimation(reuse.beginPhase()) && reuse.size()==0);
    check(reuse.step(reuse.beginPhase(),rng) && rng.drawCount()==beforeRetirement);
    std::printf("Smoke owner: %u births, phase exclusion, retirement, capacity and stable reuse passed\n",births);
}
