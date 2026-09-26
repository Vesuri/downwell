#include "core/TimeShardAnimation.h"
#include "core/GameMakerRandom.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <cstdio>
using namespace downwell;
static unsigned bits(float n){unsigned b;std::memcpy(&b,&n,4);return b;}
static float value(RunnerImageValue n){float f;std::memcpy(&f,&n.bits,4);return f;}
static float inverse(float index){
    float cycles=index/6.0f;
    float fraction=cycles-float(int(cycles));
    float position=6.0f*fraction;
    int key=int(position);fraction=position-float(key);
    if(double(fraction)+.00001>=1){++key;fraction=0;}
    if(key>5)key=5;
    return (fraction+float(key))+float(int(cycles)*6);
}
int main(){
    GameMakerRandom samples(321);unsigned changed=0;
    for(unsigned i=0;i<100000;++i){
        const float index=float(samples.random(7));RunnerImageValue mapped;
        assert(TimeShardAnimation::inverse({bits(index)},mapped));
        assert(mapped.bits==bits(inverse(index)));changed+=mapped.bits!=bits(index);
    }
    for(unsigned seed=0;seed<1024;++seed){
        GameMakerRandom rng(seed);TimeShardAnimation shard;assert(shard.create({}, {},false,rng));
        float timeline=0,index=value(shard.imageIndex);
        for(unsigned tick=0;tick<100;++tick){
            if(tick==20)shard.imageSpeed={};
            if(tick==40)shard.imageSpeed={bits(.3f)};
            if(tick==60)shard.imageSpeed={bits(-.5f)};
            if(std::abs(double(float(index-timeline)))>.00001){
                timeline=inverse(index);
                if(timeline>=6)timeline=std::fmod(float(std::fmod(timeline,6.0f)+6),6.0f);
            }
            timeline+=value(shard.imageSpeed);
            if(timeline<0 || timeline>=6)timeline=std::fmod(float(std::fmod(timeline,6.0f)+6),6.0f);
            index=timeline;
            assert(shard.advanceImage());assert(shard.timeline.bits==bits(timeline));
            assert(shard.imageIndex.bits==bits(index) && !shard.destroyRequested);
        }
        assert(rng.drawCount()==6);
        assert(shard.create({}, {},true,rng));assert(shard.timeline.bits==0);
    }
    for(float index:{0.f,1.f,5.999999f,6.f,7.f}){
        RunnerImageValue mapped;assert(TimeShardAnimation::inverse({bits(index)},mapped));
        assert(mapped.bits==bits(inverse(index)));
    }
    std::printf("Shard animation: 100000 inverse mappings (%u non-identity), 102400 updates, wrapping/pause/reuse passed\n",changed);
}
