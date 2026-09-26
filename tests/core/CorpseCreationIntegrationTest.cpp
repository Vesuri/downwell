#include "core/DeadBodyCreation.h"
#include "core/CorpseMotion.h"
#include "core/DebrisEffects.h"
#include "core/GameMakerRandom.h"
#include <cassert>
#include <cstring>
#include <cstdio>
using namespace downwell;
static MotionBinary64 real(double d){std::uint64_t b;std::memcpy(&b,&d,8);return {std::uint32_t(b>>32),std::uint32_t(b)};}
static void equal(MotionBinary64 a,double d){auto b=real(d);assert(a.high==b.high&&a.low==b.low);}
struct Adapter {
    GameMakerRandom& random;
    explicit Adapter(GameMakerRandom& rng):random(rng){}
    CorpseMotion corpse;
    unsigned walls=0,publications=0,setters=0,misses=0;
    bool blocked=false,failTile=false,created=false;
    int spriteId=172;
    RunnerImageValue facingValue{};
    bool wall(MotionBinary64,MotionBinary64,bool& hit){
        ++walls;hit=blocked&&(walls==1||walls<=misses+1);return true;
    }
    bool tile(MotionBinary64,MotionBinary64,bool& hit){hit=false;return !failTile;}
    bool publish(RunnerImageValue,RunnerImageValue){assert(!created&&!setters);++publications;return true;}
    bool create(int object,MotionBinary64 x,MotionBinary64 y,std::uint32_t& handle){
        assert(object==263&&!created&&!setters);
        if(!corpse.create(x,y,true,random,*this))return false;
        created=true;handle=7;return true;
    }
    bool sprite(unsigned handle,int id){assert(handle==7&&created&&++setters==1);spriteId=id;return true;}
    bool verticalSpeed(unsigned handle,MotionBinary64 speed){assert(handle==7&&++setters==2);corpse.ysp=speed;return true;}
    bool facing(unsigned handle,MotionBinary64 scale){assert(handle==7&&++setters==3);return facingValue.assign(scale);}
    bool explosion(unsigned handle,int id){assert(handle==7&&++setters==4);corpse.explosionFx=id;return true;}
    bool horizontalSpeed(unsigned handle,MotionBinary64 speed){assert(handle==7&&++setters==5);corpse.xsp=speed;return true;}
};
static void sameRandom(const GameMakerRandom& a,const GameMakerRandom& b){
    assert(a.drawCount()==b.drawCount()&&a.stateIndex()==b.stateIndex());
    for(unsigned i=0;i<16;++i)assert(a.stateWord(i)==b.stateWord(i));
}
int main(){
    unsigned cases=0;
    for(unsigned seed=0;seed<256;++seed)for(int path:{-1,0,7,32,33})for(double impact:{0.,2.,9.}){
        GameMakerRandom random(seed),oracle(seed);
        DeadBodySource source;source.x=real(96);source.y=real(192);source.imageXscale=real(-1);source.xsp=real(.1);
        Adapter adapter{random};adapter.blocked=path>=0;adapter.misses=path<0?0:unsigned(path);
        assert(createDeadBody(source,206,real(impact),102,adapter));
        // Source Create samples are consumed even though wrapper replaces speeds.
        oracle.random(6);oracle.random(4);
        const double rates[]={-.5,-.3,0,.3,.5};const float rate=float(rates[oracle.chooseIndexSmall(5)]);
        int dx=0,dy=0,radius=4;
        if(path>=0)for(unsigned n=0;n<unsigned(path<33?path+1:33);++n){
            dx=int(oracle.irandomSmall(radius*2))-radius;
            dy=int(oracle.irandomSmall(radius*2))-radius;
            if(n<unsigned(path)&&radius<=32)radius+=4;
        }
        sameRandom(random,oracle);
        equal(adapter.corpse.xx,96+dx);equal(adapter.corpse.yy,192+dy);
        equal(adapter.corpse.imageSpeed.promoted(),double(rate));
        equal(source.xsp,-1.5);equal(source.x,96);equal(source.y,192);
        const double capped=impact>4?4:impact;
        equal(adapter.corpse.ysp,capped);equal(adapter.corpse.xsp,capped?-1.5*capped:-.5);
        assert(adapter.spriteId==206&&source.myDeadBody==7&&adapter.setters==5);
        assert(adapter.corpse.health==1&&adapter.corpse.hitEffect==1);
        // Skeleton then creates two debris at its own position, not the relocated corpse.
        DebrisEffects<2> debris;
        for(unsigned n=0;n<2;++n){
            const auto handle=debris.create(source.x,real(187),1,random);assert(handle);
            const double sx=oracle.random(4)-2,sy=oracle.random(1.5)-.5;
            const float speed=float(rates[oracle.chooseIndexSmall(5)]),index=float(oracle.random(7));
            const auto* effect=debris.get(handle);assert(effect);
            equal(effect->x.promoted(),96);equal(effect->y.promoted(),187);
            equal(effect->xsp,sx);equal(effect->ysp,sy);
            equal(effect->imageSpeed.promoted(),speed);equal(effect->imageIndex.promoted(),index);
            sameRandom(random,oracle);
        }
        ++cases;
    }
    GameMakerRandom random(42),oracle(42);Adapter failed{random};failed.failTile=true;
    DeadBodySource source;source.imageXscale=real(1);
    assert(!createDeadBody(source,206,real(2),102,failed));
    oracle.random(6);oracle.random(4);oracle.chooseIndexSmall(5);sameRandom(random,oracle);
    assert(!failed.created&&!failed.setters&&!source.myDeadBody&&!failed.corpse.valid);
    std::printf("Corpse/debris integration: %u synchronous creation chains and failed terrain admission passed\n",cases);
}
