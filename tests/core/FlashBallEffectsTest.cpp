#include "core/FlashBallEffects.h"
#include "core/GameMakerRandom.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
static void check(bool ok){if(!ok)std::abort();}
static downwell::MotionBinary64 number(double x){
    std::uint64_t bits;std::memcpy(&bits,&x,8);return {std::uint32_t(bits>>32),std::uint32_t(bits)};
}
static float scalar(downwell::RunnerImageValue x){float value;std::memcpy(&value,&x.bits,4);return value;}
struct Reference {float x,y,index=1;};
int main(){
    unsigned births=0;
    for(unsigned seed=0;seed<1024;++seed){
        downwell::GameMakerRandom random(seed),oracle(seed);
        downwell::FlashBallEffects<16> effects;
        const auto before=effects.beginAnimation();
        const double x=seed%2?16777217.25:-64.125,y=seed%3?192.375:-16777217.25;
        check(effects.skeletonBurst(number(x),number(y),random));
        std::vector<Reference> expected;
        const unsigned count=4+unsigned(oracle.irandom(4));
        for(unsigned i=0;i<count;++i){
            const double py=y+(oracle.random(20)-10),px=x+(oracle.random(20)-10);
            const float finalX=float(double(float(px))+int(oracle.irandom(4))-2);
            const float finalY=float(double(float(py))+int(oracle.irandom(4))-2);
            expected.push_back({finalX,finalY,1});
        }
        check(random.drawCount()==oracle.drawCount());
        for(unsigned i=0;i<16;++i)check(random.stateWord(i)==oracle.stateWord(i));
        check(effects.advanceAnimation(before)); // Births do not age in an older phase.
        for(unsigned frame=0;frame<=7;++frame){
            for(unsigned i=0;i<count;++i){
                const auto& slot=effects.slot(i);
                check(bool(slot.handle)==(frame<7));
                if(frame<7){
                    check(scalar(slot.effect.x)==expected[i].x && scalar(slot.effect.y)==expected[i].y);
                    check(scalar(slot.effect.imageIndex)==expected[i].index);
                    check(slot.effect.drawFrame()==unsigned(expected[i].index));
                }
            }
            for(unsigned i=count;i<16;++i)check(!effects.slot(i).handle);
            check(effects.advanceAnimation(effects.beginAnimation()));
            for(auto& ref:expected)ref.index=float(ref.index+0.3f);
        }
        const auto draws=random.drawCount();
        check(effects.create(number(0),number(0),random)>count); // Stable handle on slot reuse.
        check(random.drawCount()==draws+4);
        births+=count;
    }
    downwell::FlashBallEffects<1> full;
    downwell::GameMakerRandom random(9);
    check(full.create(number(0),number(0),random)==1);
    const auto draws=random.drawCount();
    check(!full.create(number(0),number(0),random) && draws==random.drawCount());
    unsigned visited=0;
    full.visitNewest([&](const downwell::FlashBallEffect&){++visited;});check(visited==1);
    full.clear();visited=0;
    full.visitNewest([&](const downwell::FlashBallEffect&){++visited;});check(visited==0);
    downwell::FlashBallEffects<3> ordered;
    check(ordered.create(number(10),number(0),random));
    const auto older=ordered.beginAnimation();
    for(unsigned i=0;i<6;++i)check(ordered.advanceAnimation(older));
    check(ordered.create(number(100),number(0),random));
    check(ordered.advanceAnimation(older)); // Retire older slot only.
    check(ordered.create(number(200),number(0),random)); // Reuses lower slot.
    std::vector<int> order;
    ordered.visitNewest([&](const downwell::FlashBallEffect& effect){order.push_back(effect.pixelX());});
    check(order.size()==2 && order[0]>=198 && order[1]<=102);
    std::printf("Flash effects: %u owned births across 1024 seeds, seven-update retirement and slot reuse passed\n",births);
}
