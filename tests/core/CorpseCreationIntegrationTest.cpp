#include "core/DeadBodyCreation.h"
#include "core/CorpseEffects.h"
#include "core/DebrisEffects.h"
#include "core/GameMakerRandom.h"
#include <cassert>
#include <cstring>
#include <cstdio>
using namespace downwell;
static MotionBinary64 real(double d){std::uint64_t b;std::memcpy(&b,&d,8);return {std::uint32_t(b>>32),std::uint32_t(b)};}
static void equal(MotionBinary64 a,double d){auto b=real(d);assert(a.high==b.high&&a.low==b.low);}
struct Adapter {
    unsigned walls=0,publications=0,setters=0,misses=0;
    bool blocked=false,failTile=false,created=false;
    bool wall(MotionBinary64,MotionBinary64,bool& hit){
        ++walls;hit=blocked&&(walls==1||walls<=misses+1);return true;
    }
    bool tile(MotionBinary64,MotionBinary64,bool& hit){hit=false;return !failTile;}
    bool publish(RunnerImageValue,RunnerImageValue){assert(!created&&!setters);++publications;return true;}
    bool birth(unsigned handle,const CorpseAnimation& c){assert(handle==1&&c.spriteId==172&&!walls);return true;}
    Adapter& queries(unsigned handle,CorpseAnimation&){assert(handle==1);return *this;}
    bool retire(unsigned handle,const CorpseAnimation&){assert(handle==1);return true;}
    bool sprite(unsigned handle,const CorpseAnimation& c){assert(handle==1&&++setters==1&&c.spriteId==206);created=true;return true;}
    bool facing(unsigned handle,const CorpseAnimation& c){assert(handle==1&&created&&++setters==2);equal(c.imageXscale.promoted(),-1);return true;}
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
        Adapter adapter;adapter.blocked=path>=0;adapter.misses=path<0?0:unsigned(path);
        CorpseEffects<1> owner;
        assert(owner.createFromDeath(source,206,real(impact),102,true,random,adapter));
        const auto* live=owner.get(source.myDeadBody);assert(live);const auto& corpse=*live;
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
        equal(corpse.xx,96+dx);equal(corpse.yy,192+dy);
        equal(corpse.imageSpeed.promoted(),double(rate));
        equal(source.xsp,-1.5);equal(source.x,96);equal(source.y,192);
        const double capped=impact>4?4:impact;
        equal(corpse.ysp,capped);equal(corpse.xsp,capped?-1.5*capped:-.5);
        assert(corpse.spriteId==206&&source.myDeadBody==1&&adapter.setters==2);
        assert(corpse.imageIndex.bits==0&&corpse.timeline.bits==0);
        equal(corpse.imageXscale.promoted(),-1);
        assert(corpse.health==1&&corpse.hitEffect==1);
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
    GameMakerRandom random(42),oracle(42);Adapter failed;failed.failTile=true;CorpseEffects<1> owner;
    DeadBodySource source;source.imageXscale=real(1);
    assert(!owner.createFromDeath(source,206,real(2),102,true,random,failed));
    oracle.random(6);oracle.random(4);oracle.chooseIndexSmall(5);sameRandom(random,oracle);
    assert(!failed.created&&!failed.setters&&!source.myDeadBody&&!owner.valid());
    assert(owner.clear(failed));
    std::printf("Corpse/debris integration: %u synchronous creation chains and failed terrain admission passed\n",cases);
}
