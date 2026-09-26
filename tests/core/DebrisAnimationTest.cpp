#include "core/DebrisAnimation.h"
#include "core/GameMakerRandom.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <cstdio>
using namespace downwell;
static unsigned bits(float n){unsigned b;std::memcpy(&b,&n,4);return b;}
static float value(RunnerImageValue n){float f;std::memcpy(&f,&n.bits,4);return f;}
static float inverse(float index,unsigned frames){
    const float cycles=index/float(frames);
    const float position=float(frames)*float(cycles-float(int(cycles)));
    int key=int(position);float fraction=position-float(key);
    if(double(fraction)+.00001>=1){++key;fraction=0;}
    if(key>=int(frames))key=int(frames)-1;
    return (fraction+float(key))+float(int(cycles)*frames);
}
static float wrap(float n,unsigned frames){return std::fmod(float(std::fmod(n,float(frames))+frames),float(frames));}
int main(){
    GameMakerRandom samples(321);
    for(unsigned frames:{4u,6u}){
        for(unsigned i=0;i<100000;++i){
            const float index=float(samples.random(7));RunnerImageValue mapped;
            assert(EffectSequenceAnimation::inverse({bits(index)},frames,mapped));
            assert(mapped.bits==bits(inverse(index,frames)));
        }
        for(float index:{0.f,1.f,3.999999f,4.f,5.999999f,6.f,7.f}){
            RunnerImageValue mapped;assert(EffectSequenceAnimation::inverse({bits(index)},frames,mapped));
            assert(mapped.bits==bits(inverse(index,frames)));
        }
    }
    for(unsigned seed=0;seed<128;++seed)for(int area=0;area<=5;++area){
        GameMakerRandom random(seed);DebrisAnimation effect;
        assert(effect.create({}, {},area,random));const auto draws=random.drawCount();
        const unsigned frames=effect.spriteId==91?6:4;
        float timeline=0,index=value(effect.imageIndex);
        for(unsigned tick=0;tick<100;++tick){
            if(tick==20)effect.imageSpeed={};
            if(tick==40)effect.imageSpeed={bits(.3f)};
            if(tick==60)effect.imageSpeed={bits(-.5f)};
            if(std::abs(double(float(index-timeline)))>.00001){
                timeline=inverse(index,frames);if(timeline>=frames)timeline=wrap(timeline,frames);
            }
            timeline+=value(effect.imageSpeed);
            if(timeline<0||timeline>=frames)timeline=wrap(timeline,frames);
            index=timeline;
            assert(effect.advanceImage()&&!effect.destroyRequested);
            assert(effect.timeline.bits==bits(timeline)&&effect.imageIndex.bits==bits(index));
        }
        assert(random.drawCount()==draws);
        effect.destroyRequested=true;const auto old=effect.imageIndex.bits;
        assert(effect.advanceImage()&&effect.imageIndex.bits==old);
        assert(effect.create({}, {},area,random)&&effect.timeline.bits==0&&!effect.destroyRequested);
    }
    RunnerImageValue output{123};
    assert(!EffectSequenceAnimation::inverse({},5,output)&&output.bits==123);
    DebrisAnimation unknown;unknown.spriteId=999;
    assert(!unknown.advanceImage());
    std::puts("Debris animation: 200000 inverse mappings, 76800 updates, wrapping/pause/reuse passed");
}
