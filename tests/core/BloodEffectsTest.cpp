#include "core/BloodEffects.h"
#include "core/GameMakerRandom.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace downwell;
int main(){
    unsigned updates=0;
    for(unsigned seed=0;seed<128;++seed){
        BloodEffects<4> owner;GameMakerRandom random(seed),reference(seed);
        BloodEffect expected[2];unsigned births=0;
        const auto stale=owner.beginPhase();
        assert(owner.burst({}, {},random));
        assert(bloodBurst({}, {},reference,[&](auto x,auto y,auto angle){return expected[births++].create(x,y,angle);}));
        assert(owner.advanceAnimation(stale));
        assert(owner.step(stale,[](auto,const auto&){assert(false);return true;}));
        for(unsigned tick=0;tick<80;++tick){
            const auto phase=owner.beginPhase();assert(owner.advanceAnimation(phase));
            for(auto& effect:expected)if(!effect.destroyRequested)assert(effect.advanceImage());
            std::vector<unsigned> visited;
            assert(owner.step(phase,[&](unsigned handle,const auto&){visited.push_back(handle);return (tick+handle)%5>=2;}));
            for(unsigned i=2;i>0;--i)if(!expected[i-1].destroyRequested)assert(expected[i-1].step((tick+i)%5>=2));
            unsigned alive=0;
            for(unsigned i=0;i<2;++i){
                const auto* actual=owner.get(i+1);const auto& wanted=expected[i];
                assert(bool(actual)==!wanted.destroyRequested);
                if(!actual)continue;
                ++alive;++updates;
                assert(actual->xx.high==wanted.xx.high && actual->xx.low==wanted.xx.low &&
                    actual->yy.high==wanted.yy.high && actual->yy.low==wanted.yy.low &&
                    actual->imageIndex.bits==wanted.imageIndex.bits && actual->imageSpeed.bits==wanted.imageSpeed.bits);
            }
            assert(owner.size()==alive && visited.size()==alive);
            if(visited.size()==2)assert(visited[0]==2 && visited[1]==1);
        }
        assert(owner.size()==0 && random.drawCount()==reference.drawCount());
        for(unsigned i=0;i<16;++i)assert(random.stateWord(i)==reference.stateWord(i));
    }
    BloodEffects<2> owner;GameMakerRandom random(1),reference(1);
    const auto first=owner.create({}, {},{}),second=owner.create({}, {},{});
    const auto cutoff=owner.beginPhase();
    assert(first && second && !owner.create({}, {},{}));
    owner.get(first)->destroyRequested=true;owner.retireRequested();
    const auto third=owner.create({}, {},{});assert(third>second && !owner.get(first));
    std::vector<unsigned> order;owner.visitNewest([&](auto h,const auto&){order.push_back(h);});
    assert(order==std::vector<unsigned>({third,second}));
    assert(owner.advanceAnimation(cutoff));
    assert(owner.get(third)->timeline.bits==0 && owner.get(second)->timeline.bits==0x3f000000u);
    owner.clear();assert(owner.size()==0 && !owner.get(third));
    const auto fourth=owner.create({}, {},{});assert(fourth>third);
    assert(owner.step(cutoff,[](auto,const auto&){assert(false);return true;}));
    // One free slot: preserve the first birth and both already-evaluated random
    // angles when the second allocation fails; do not preflight or roll back.
    reference.nextU32();reference.nextU32();
    assert(!owner.burst({}, {},random) && owner.size()==2);
    assert(random.drawCount()==reference.drawCount());
    for(unsigned i=0;i<16;++i)assert(random.stateWord(i)==reference.stateWord(i));
    std::printf("Blood owner: %u live updates; cutoffs, retirement, slot reuse, reset and partial burst failure passed\n",updates);
}
