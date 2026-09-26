#include "core/CorpseAnimation.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <initializer_list>
using namespace downwell;
static RunnerImageValue value(float f){RunnerImageValue v;std::memcpy(&v.bits,&f,4);return v;}
int main(){
    unsigned ticks=0;
    for(float speed:{-.5f,-.3f,0.f,.3f,.5f}){
        CorpseAnimation corpse;corpse.imageSpeed=value(speed);float position=0;
        unsigned published=0;
        auto publish=[&](const auto& c){assert(c.spriteId==172||c.spriteId==206);++published;return true;};
        for(unsigned tick=0;tick<4096;++tick){
            if(tick%19==0){
                const auto before=corpse.imageIndex.bits,timeline=corpse.timeline.bits;
                assert(corpse.setSprite(corpse.spriteId==172?206:172,publish));
                assert(corpse.imageIndex.bits==before&&corpse.timeline.bits==timeline);
            }
            const unsigned calls=published;assert(corpse.setSprite(corpse.spriteId,publish)&&published==calls);
            position=float(position+speed);
            position=std::fmod(position,1.f);if(position<0)position=float(position+1.f);
            if(position==0)position=0; // canonical evaluated key zero
            assert(corpse.advanceImage());
            assert(corpse.imageIndex.bits==value(position).bits&&!corpse.destroyRequested);++ticks;
        }
    }
    CorpseAnimation corpse;corpse.imageIndex=value(2.f);corpse.timeline=value(.75f);
    assert(corpse.setSprite(206,[](const auto& c){return c.imageIndex.bits==0&&c.timeline.bits==0;}));
    corpse.imageIndex=value(.5f); // Unsupported external nonzero assignment.
    assert(!corpse.advanceImage()&&!corpse.valid);
    CorpseAnimation failed;assert(!failed.setSprite(206,[](const auto&){return false;})&&!failed.valid);
    assert(!failed.advanceImage());
    CorpseAnimation retired;retired.destroyRequested=true;retired.imageSpeed=value(.5f);
    assert(retired.advanceImage()&&retired.imageIndex.bits==0);
    std::printf("Corpse animation: %u signed-rate updates, sprite publication/reset and failure gates passed\n",ticks);
}
