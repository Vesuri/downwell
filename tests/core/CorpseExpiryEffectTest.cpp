#include "core/CorpseExpiryEffect.h"
#include "core/CorpseLifecycle.h"
#include <cassert>
#include <cstdio>
#include <cstring>
using namespace downwell;
static unsigned bits(float value){unsigned result;std::memcpy(&result,&value,4);return result;}
struct Events {
    CorpseLifecycle& corpse;
    explicit Events(CorpseLifecycle& value):corpse(value){}
    CorpseExpiryEffect child;
    unsigned births=0,sets=0,deaths=0;
    bool create(int object,MotionBinary64 x,MotionBinary64 y,std::uint32_t& id){
        assert(object==242&&!corpse.destroyRequested);
        ++births;id=1;return child.create(x,y);
    }
    bool sprite(unsigned id,int sprite){
        assert(id==1&&births==1&&!corpse.destroyRequested);++sets;
        return child.setSprite(sprite,12,[](const auto&){assert(false);return false;});
    }
    void destroy(){assert(corpse.destroyRequested&&births==1&&sets==1);++deaths;}
};
int main(){
    CorpseLifecycle corpse;corpse.x={0x42c10000u};corpse.y={0xc1400000u};
    corpse.explosionFx=102;corpse.alarms.set(0,1);Events events{corpse};
    assert(corpse.advanceAlarm(0,events)&&corpse.destroyRequested&&events.deaths==1);
    assert(events.child.x.bits==corpse.x.bits&&events.child.y.bits==corpse.y.bits);
    assert(events.child.imageSpeed.bits==bits(0.3f));
    float position=0;
    unsigned ticks=0;
    do {
        position=float(position+0.3f);const bool crossed=position>=12;
        if(crossed)position=float(position-12);
        assert(events.child.advanceImage());++ticks;
        assert(events.child.imageIndex.bits==bits(position));
        assert(events.child.destroyRequested==crossed);
    } while(!events.child.destroyRequested);
    assert(ticks==40);
    assert(events.child.advanceImage()&&events.child.imageIndex.bits==bits(position));
    assert(corpse.advanceAlarm(0,events)&&events.births==1&&events.deaths==1);
    CorpseExpiryEffect effect;assert(effect.create({},{}));
    for(unsigned i=0;i<20;++i)assert(effect.advanceImage());
    unsigned publications=0;
    assert(effect.setSprite(116,4,[&](const auto& changed){
        ++publications;return changed.spriteId==116&&!changed.imageIndex.bits&&!changed.timeline.bits;
    }));
    assert(publications==1&&effect.frames==4);
    assert(!effect.setSprite(115,4,[](const auto&){return false;})&&!effect.valid);
    assert(!effect.advanceImage());
    assert(effect.create({},{}));
    assert(!effect.setSprite(159,0,[](const auto&){return true;}));
    std::puts("Corpse expiry: synchronous child creation, 40-frame lifetime, sprite reset and publication failure passed");
}
