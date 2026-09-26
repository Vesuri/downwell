#include "core/CorpseLifecycle.h"
#include <cassert>
#include <cstdio>
using namespace downwell;
struct Events {
    CorpseLifecycle& corpse;
    unsigned created=0,sprites=0,destroys=0,eaten=0,sounds=0,reads=0,fail=0;
    bool rejectSprite=false,rejectCounter=false;
    bool incrementBodiesEaten(){++eaten;return !rejectCounter;}
    void eatSound(int a,int b,int c,int d){assert(eaten==1&&a==83&&b==80&&c==0&&d==1);++sounds;}
    MotionBinary64 playerY(){assert(reads++==created*2);return MotionBinary64::fromInteger(20+created);}
    MotionBinary64 playerX(){assert(reads++==created*2+1);return MotionBinary64::fromInteger(10+created);}
    bool create(int object,MotionBinary64 x,MotionBinary64 y,std::uint32_t& id){
        if(object==242){assert(x.zero()&&y.zero());corpse.explosionFx=116;}
        else {
            assert(object==223&&eaten==1&&sounds==1);
            assert(x.high==MotionBinary64::fromInteger(10+created).high);
            assert(y.high==MotionBinary64::fromInteger(20+created).high);
        }
        id=++created;return created!=fail;
    }
    bool sprite(unsigned id,int sprite){assert(id==1&&sprite==116&&created==1);++sprites;return !rejectSprite;}
    void destroy(){assert(corpse.destroyRequested);++destroys;}
};
int main(){
    for(unsigned index=0;index<4;++index){
        CorpseLifecycle corpse;Events events{corpse};corpse.hit=true;corpse.alarms.set(index,2);
        assert(corpse.advanceAlarm(index,events)&&!corpse.destroyRequested);
        assert(corpse.advanceAlarm(index,events));
        assert(corpse.destroyRequested==(index==0)&&corpse.disappearing==(index==1));
        assert(corpse.hit==(index!=2)&&corpse.active==(index==3));
        assert(events.created==(index==0)&&events.sprites==(index==0)&&events.destroys==(index==0));
        assert(corpse.advanceAlarm(index,events));assert(events.destroys<=1);
    }
    for(bool upgrade:{false,true})for(bool dead:{false,true})for(bool active:{false,true}){
        CorpseLifecycle corpse;corpse.active=active;Events events{corpse};
        const bool eat=upgrade&&!dead&&active;
        assert(corpse.collidePlayer(upgrade,dead,events));
        assert(events.created==(eat?5u:0u)&&events.eaten==eat&&events.sounds==eat&&events.destroys==eat);
        assert(corpse.collidePlayer(upgrade,dead,events));assert(events.created<=5);
    }
    for(unsigned fail=1;fail<=5;++fail){
        CorpseLifecycle corpse;corpse.active=true;Events events{corpse};events.fail=fail;
        assert(!corpse.collidePlayer(true,false,events)&&!corpse.valid&&!corpse.destroyRequested);
        assert(events.created==fail&&events.eaten==1&&events.sounds==1&&!events.destroys);
        assert(!corpse.collidePlayer(true,false,events)&&events.created==fail);
        assert(!corpse.advanceAlarm(0,events));
    }
    for(bool failCreate:{false,true}){
        CorpseLifecycle corpse;Events events{corpse};corpse.alarms.set(0,1);
        events.fail=failCreate?1:0;events.rejectSprite=!failCreate;
        assert(!corpse.advanceAlarm(0,events)&&!corpse.valid&&!corpse.destroyRequested);
        assert(events.created==1&&events.sprites==!failCreate&&!events.destroys);
        assert(!corpse.advanceAlarm(0,events)&&events.created==1);
    }
    CorpseLifecycle corpse;corpse.active=true;Events events{corpse};events.rejectCounter=true;
    assert(!corpse.collidePlayer(true,false,events)&&!events.sounds&&!events.created);
    std::puts("Corpse lifecycle: four alarms, eight consumption gates, synchronous field reads and eight failure boundaries passed");
}
