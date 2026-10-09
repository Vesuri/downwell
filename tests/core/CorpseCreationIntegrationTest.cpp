#include "core/DeadBodyCreation.h"
#include "core/CorpseEffects.h"
#include "core/DebrisEffects.h"
#include "core/GameMakerRandom.h"
#include "core/StoredCorpseRegistration.h"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>
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
struct RegistryAudit {
    unsigned calls=0,failure=0,walls=0,retired=0;
    bool accept(){return ++calls!=failure;}
    bool birth(InstanceHandle,const InstanceRecord& r){assert(r.objectId==263&&r.spriteId==172);return accept();}
    bool birthAt(InstanceHandle h,const InstanceRecord& r,RunnerImageValue,RunnerImageValue){return birth(h,r);}
    bool markDestroyed(InstanceHandle,const InstanceRecord&){return false;}
    bool instanceNumber(int,unsigned&){return false;}
    bool instancePlace(InstanceHandle,const InstanceRecord&,int,MotionBinary64,MotionBinary64,InstancePlaceResult&){return false;}
    bool retire(InstanceHandle,const InstanceRecord&){++retired;return true;}
    bool activation(InstanceHandle,const InstanceRecord&,bool){return true;}
    bool membership(InstanceHandle,const InstanceRecord&,bool){return true;}
    bool move(InstanceHandle,const InstanceRecord&,RunnerImageValue,RunnerImageValue){return accept();}
    bool sprite(InstanceHandle,const InstanceRecord& r,int sprite){assert(r.spriteId==172&&sprite==206);return accept();}
    bool imageIndex(InstanceHandle,const InstanceRecord&,RunnerImageValue){return true;}
    bool mask(InstanceHandle,const InstanceRecord&,int){return true;}
    bool transform(InstanceHandle,const InstanceRecord& r,RunnerTransformField field,
        RunnerImageValue value,RunnerImageValue,std::uint32_t){
        assert(r.scaleX==1&&field==RunnerTransformField::scaleX&&value.bits==0xbf800000u);return accept();
    }
    bool placeMeeting(InstanceHandle,const InstanceRecord& r,int object,MotionBinary64,MotionBinary64,bool& hit){
        assert(object==57&&r.alarms.get(0)==180&&r.alarms.get(3)==8);
        hit=++walls==1;return accept();
    }
};
static void registeredCorpses(){
    std::ifstream file("generated/downwell-objects.dwob",std::ios::binary);
    std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
    ObjectPack objects(bytes.data(),bytes.size());assert(objects.valid());
    for(unsigned failure=0;failure<=10;++failure){
        InstanceStore<4> records;RegistryAudit audit;audit.failure=failure;
        std::uint32_t next=105187;int room=11;bool good=true;GameMakerRandom rng(42);
        StoredCorpseRegistration<4,1> corpses(records,&objects,InstanceLifecycleBinding::bindPositionedBirths(audit),next,room,good);
        DeadBodySource source;source.x=real(96.25);source.y=real(192.5);source.imageXscale=real(-1);
        auto tile=[&](InstanceHandle h,int layer,MotionBinary64,MotionBinary64,bool& hit){
            assert(records.resolve(h)&&layer==100);hit=false;return audit.accept();
        };
        const bool ok=corpses.createFromDeath(source,206,real(2),102,true,rng,tile);
        assert(ok==!failure&&good==!failure&&next==105188&&records.size()==1);
        const auto key=corpses.sourceHandle(105187);assert(key.generation&&corpses.state(key));
        assert(audit.calls==(failure?failure:10));
        if(ok){
            const auto* r=records.resolve(key);const auto* state=corpses.state(key);
            assert(source.myDeadBody==105187&&r->spriteId==206&&r->scaleX==-1&&r->imageIndex==0);
            equal(state->xsp,-3);equal(state->ysp,2);equal(source.xsp,-1.5);
            GameMakerRandom oracle(42);oracle.nextU32();oracle.nextU32();oracle.chooseIndexSmall(5);
            oracle.irandomSmall(8);oracle.irandomSmall(8);sameRandom(rng,oracle);
            assert(state->explosionFx==102&&state->health==1&&state->hitEffect==1);
        }else{
            const auto draws=rng.drawCount();const auto calls=audit.calls;
            assert(!corpses.createFromDeath(source,206,real(2),102,true,rng,tile)&&
                rng.drawCount()==draws&&audit.calls==calls);
            assert(source.myDeadBody==(failure>=9?105187u:0u));
        }
        assert(corpses.clear()&&records.size()==0&&!corpses.state(key)&&audit.retired==1);
    }
}
int main(){
    registeredCorpses();
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
