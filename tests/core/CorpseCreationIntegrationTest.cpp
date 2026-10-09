#include "core/DeadBodyCreation.h"
#include "core/CorpseEffects.h"
#include "core/DebrisEffects.h"
#include "core/GameMakerRandom.h"
#include "core/StoredCorpseRegistration.h"
#include "core/StoredCorpseExpiryRegistration.h"
#include "core/CorpseExpiryInstances.h"
#include "core/CorpseInstances.h"
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
struct NativeExpiryAudit {
    using Record=CorpseExpiryInstances<1>::Record;
    unsigned rejected=0,marks=0,retirements=0;
    bool birth(InstanceHandle,const Record&){return false;}
    bool birthAt(InstanceHandle,const Record& r,RunnerImageValue x,RunnerImageValue y){
        assert(r.sourceId==105187&&r.state.spriteId==102&&r.state.imageSpeed.bits==0x3f800000u&&
            r.x.bits==x.bits&&r.y.bits==y.bits);return rejected!=1;
    }
    bool sprite(InstanceHandle,const Record& r,int sprite){assert(r.state.spriteId==sprite);return rejected!=2;}
    bool imageIndex(InstanceHandle,const Record& r,RunnerImageValue value){assert(r.state.imageIndex.bits==value.bits);return rejected!=3;}
    bool markDestroyed(InstanceHandle,const Record& r){assert(r.state.destroyRequested&&!r.destroyRequested);++marks;return rejected!=4;}
    bool retire(InstanceHandle,const Record&){++retirements;return rejected!=5;}
    bool activation(InstanceHandle,const Record&,bool){return true;}
    bool membership(InstanceHandle,const Record&,bool){return true;}
    bool move(InstanceHandle,const Record&,RunnerImageValue,RunnerImageValue){return false;}
    bool mask(InstanceHandle,const Record&,int){return false;}
    bool transform(InstanceHandle,const Record&,RunnerTransformField,RunnerImageValue,RunnerImageValue,std::uint32_t){return false;}
};
struct NativeCorpseAudit {
    using Record=CorpseInstances<1>::Record;
    unsigned calls=0,reject=0,walls=0,moves=0,marks=0;bool blocked=false;
    bool accept(){return ++calls!=reject;}
    bool birth(InstanceHandle,const Record&){return false;}
    bool birthAt(InstanceHandle,const Record& r,RunnerImageValue x,RunnerImageValue y){
        assert(r.sourceId==105187&&r.state.spriteId==172&&r.x.bits==x.bits&&r.y.bits==y.bits);return accept();
    }
    bool placeMeeting(InstanceHandle,const Record& r,int object,MotionBinary64,MotionBinary64,bool& hit){
        assert(object==57||object==56||object==82);
        if(object==57){assert(r.state.alarms.get(0)==180&&r.state.alarms.get(3)==8);hit=++walls==1&&blocked;}
        else hit=false;
        return accept();
    }
    bool instancePlace(InstanceHandle,const Record&,int,MotionBinary64,MotionBinary64,InstancePlaceResult&){return false;}
    bool instanceNumber(int,unsigned&){return false;}
    bool sprite(InstanceHandle,const Record& r,int sprite){assert(r.state.spriteId==sprite);return accept();}
    bool imageIndex(InstanceHandle,const Record&,RunnerImageValue){return accept();}
    bool markDestroyed(InstanceHandle,const Record& r){assert(r.state.destroyRequested&&!r.destroyRequested);++marks;return accept();}
    bool retire(InstanceHandle,const Record&){return accept();}
    bool activation(InstanceHandle,const Record&,bool){return true;}
    bool membership(InstanceHandle,const Record&,bool){return true;}
    bool move(InstanceHandle,const Record& r,RunnerImageValue x,RunnerImageValue y){
        assert(x.bits==r.state.x.bits&&y.bits==r.state.y.bits);++moves;return accept();
    }
    bool mask(InstanceHandle,const Record&,int){return true;}
    bool transform(InstanceHandle,const Record&,RunnerTransformField,RunnerImageValue,RunnerImageValue,std::uint32_t){return accept();}
};
static void nativeCorpses(){
    using Owner=CorpseInstances<1>;
    for(unsigned failure=0;failure<=5;++failure){
        NativeCorpseAudit audit;audit.reject=failure;
        Owner owner(Owner::Binding::bindPositionedBirths(audit));
        GameMakerRandom random(23),oracleRandom(23);
        DeadBodySource source{real(240.25),real(192.5),real(.25),real(-1)};
        const bool okay=owner.createFromDeath(105187,source,206,real(8),102,false,random,
            [](auto,int layer,auto,auto,bool& hit){assert(layer==100);hit=false;return true;});
        assert(okay==!failure&&owner.valid()==!failure);
        const auto key=owner.sourceHandle(105187);const auto* record=owner.find(key);
        assert(record&&key.generation!=105187&&source.bodySprite==206);
        equal(source.impactSp,4);
        // Birth, wall query, sprite, image-index publication, then facing.
        // Failed Create does not return a child ID; later failures retain it.
        assert(source.myDeadBody==(failure==1||failure==2?0u:105187u));
        if(!failure){
            CorpseAnimation oracle;Adapter queries;
            assert(oracle.create(source.x,source.y,false,oracleRandom,queries));
            sameRandom(random,oracleRandom);
            equal(source.xsp,-1.5);equal(record->state.xsp,-6);equal(record->state.ysp,4);
            equal(record->state.imageXscale.promoted(),-1);
            assert(record->state.spriteId==206&&record->state.explosionFx==102&&
                record->state.alarms.get(3)==8&&record->state.alarms.get(0)==180);
        }else{
            equal(source.xsp,.25);
            const auto draws=random.drawCount();
            assert(!owner.createFromDeath(105188,source,206,real(1),102,false,random,
                [](auto,int,auto,auto,bool&){return false;})&&random.drawCount()==draws);
        }
        audit.reject=0;assert(owner.clear());
    }
    for(unsigned seed=0;seed<128;++seed){
        NativeCorpseAudit audit;audit.blocked=seed%2;
        Owner owner(Owner::Binding::bindPositionedBirths(audit));InstanceHandle key;
        GameMakerRandom random(seed),referenceRandom(seed);CorpseAnimation reference;Adapter queries;queries.blocked=audit.blocked;
        auto tile=[](auto,int layer,auto,auto,bool& hit){assert(layer==100);hit=false;return true;};
        assert(owner.create(105187,real(240.25),real(192.5),false,random,tile,key)&&
            reference.create(real(240.25),real(192.5),false,referenceRandom,queries));
        for(unsigned word=0;word<16;++word)assert(random.stateWord(word)==referenceRandom.stateWord(word));
        assert(random.drawCount()==referenceRandom.drawCount()&&audit.walls==queries.walls&&audit.moves==queries.publications);
        assert(owner.sprite(key,206)&&reference.setSprite(206,[](const auto&){return true;})&&owner.facing(key,real(-1)));
        for(unsigned frame=0;frame<32;++frame){
            struct Events {
                bool meeting(MotionBinary64,MotionBinary64,bool& hit){hit=false;return true;}
                bool publish(RunnerImageValue,RunnerImageValue){return true;}
                int area(){return 1;}int restInPieces(){return 0;}
                bool create(int,MotionBinary64,MotionBinary64){return false;}void destroy(){assert(false);}
            } events;
            assert(owner.advanceAnimation(key)&&reference.advanceImage());
            assert(owner.dispatchPublished(key,[&](auto& state,auto& publish,bool& good){
                return state.stepInWorld(false,480,publish,events,good);
            })&&reference.step(false,events));
            const auto* r=owner.find(key);
            assert(r&&r->x.bits==reference.x.bits&&r->y.bits==reference.y.bits&&
                r->state.imageIndex.bits==reference.imageIndex.bits&&r->state.timeline.bits==reference.timeline.bits&&
                r->state.xsp.high==reference.xsp.high&&r->state.xsp.low==reference.xsp.low&&
                r->state.ysp.high==reference.ysp.high&&r->state.ysp.low==reference.ysp.low);
        }
        assert(owner.dispatchPublished(key,[](auto& state,auto& publish,bool&){
            state.destroyRequested=true;return publish.destroy();
        })&&owner.find(key)&&audit.marks==1&&owner.retire(key)&&!owner.find(key));
    }
    for(unsigned failure=1;failure<=7;++failure){
        NativeCorpseAudit audit;audit.blocked=true;audit.reject=failure;
        Owner owner(Owner::Binding::bindPositionedBirths(audit));bool worldGood=true;owner.observeFailure(worldGood);
        GameMakerRandom random(2);InstanceHandle key;
        assert(!owner.create(105187,real(240.25),real(192.5),false,random,
            [](auto,int,auto,auto,bool& hit){hit=false;return true;},key)&&!key.generation&&!owner.valid()&&!worldGood);
        assert(owner.size()==1&&owner.find(owner.sourceHandle(105187)));
        const auto draws=random.drawCount();assert(!owner.advanceAnimation(owner.sourceHandle(105187))&&random.drawCount()==draws);
        audit.reject=0;assert(owner.clear()&&!owner.size());
    }
    std::puts("Native corpses: 4096 shared motion/animation frames and seven retained Create failures passed");
}
static void nativeExpiry(){
    {
        using Instances=CorpseExpiryInstances<1>;NativeExpiryAudit audit;
        Instances effects(Instances::Binding::bindPositionedActor(audit));InstanceHandle key;
        assert(effects.create(105187,real(96.25),real(192.5),key));
        CorpseExpiryEffect oracle;assert(oracle.create(real(96.25),real(192.5)));
        for(unsigned i=0;i<40;++i){
            assert(oracle.advanceImage()&&effects.advanceAnimation(key));
            const auto* r=effects.find(key);
            assert(r&&r->state.timeline.bits==oracle.timeline.bits&&r->state.imageIndex.bits==oracle.imageIndex.bits&&
                r->destroyRequested==oracle.destroyRequested&&effects.size()==1);
        }
        assert(oracle.destroyRequested&&audit.marks==1&&!audit.retirements&&effects.retire(key)&&!effects.find(key));
    }
    for(unsigned failure=0;failure<=5;++failure){
        NativeExpiryAudit audit;audit.rejected=failure;
        using Instances=CorpseExpiryInstances<1>;
        Instances effects(Instances::Binding::bindPositionedActor(audit));bool good=true;effects.observeFailure(good);
        InstanceHandle key;bool ok=effects.create(105187,real(96.25),real(192.5),key);
        const auto retained=effects.sourceHandle(105187);assert(retained.generation&&effects.size()==1);
        assert(ok==(failure!=1));
        if(ok){
            assert(key.generation!=105187&&effects.find(key)->state.imageSpeed.bits==0x3e99999au);
            ok=effects.sprite(key,206,1);assert(ok==(failure!=2&&failure!=3));
        }
        if(ok){
            assert(effects.activate(key,false)&&effects.advanceAnimation(key)&&effects.find(key)->state.timeline.bits==0);
            assert(effects.activate(key,true));
            for(unsigned i=0;i<4&&ok;++i)ok=effects.advanceAnimation(key);
            assert(ok==(failure!=4)&&effects.size()==1&&effects.find(key));
        }
        if(ok){
            assert(effects.find(key)->destroyRequested&&effects.advanceAnimation(key)&&audit.marks==1);
            ok=effects.retire(key);assert(ok==(failure!=5));
        }
        assert(ok==!failure&&good==!failure);
        if(failure){InstanceHandle retry;assert(!effects.create(105188,{}, {},retry)&&!retry.generation&&effects.find(retained));}
        else assert(!effects.find(retained)&&!effects.size());
        audit.rejected=0;assert(effects.clear()&&!effects.size()&&!effects.find(retained));
    }
}
struct RegistryAudit {
    unsigned calls=0,failure=0,walls=0,retired=0,destroyMarks=0;
    bool accept(){return ++calls!=failure;}
    bool birth(InstanceHandle,const InstanceRecord& r){assert(r.objectId==263&&r.spriteId==172);return accept();}
    bool birthAt(InstanceHandle h,const InstanceRecord& r,RunnerImageValue,RunnerImageValue){return birth(h,r);}
    bool markDestroyed(InstanceHandle,const InstanceRecord& r){assert(!r.destroyRequested);++destroyMarks;return false;}
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
    for(unsigned failure=0;failure<=5;++failure){
        struct ExpiryAudit:RegistryAudit {
            unsigned reject=0;
            bool birth(InstanceHandle,const InstanceRecord& r){
                assert(r.objectId==242&&r.spriteId==102&&r.imageSpeed==1);return reject!=1;
            }
            bool birthAt(InstanceHandle h,const InstanceRecord& r,RunnerImageValue,RunnerImageValue){return birth(h,r);}
            bool sprite(InstanceHandle,const InstanceRecord& r,int next){assert(r.spriteId==102&&next==206);return reject!=2;}
            bool imageIndex(InstanceHandle,const InstanceRecord&,RunnerImageValue){return reject!=3;}
            bool markDestroyed(InstanceHandle,const InstanceRecord& r){assert(!r.destroyRequested);++destroyMarks;return reject!=4;}
            bool retire(InstanceHandle,const InstanceRecord&){++retired;return reject!=5;}
        } audit;
        InstanceStore<4> records;std::uint32_t next=105187,id=0;int room=11;bool good=true;audit.reject=failure;
        StoredCorpseExpiryRegistration<4,1> expiry(records,&objects,InstanceLifecycleBinding::bindPositionedBirths(audit),next,room,good);
        bool ok=expiry.create(real(96.25),real(192.5),id);
        assert(next==105188&&records.size()==1&&ok==(failure!=1));
        const auto key=records.handleAt(0);
        if(ok){assert(id==105187&&records.resolve(key)->imageSpeed==0.3f);
            ok=expiry.sprite(id,206,1);assert(ok==(failure!=2&&failure!=3));
        }
        if(ok){
            for(unsigned i=0;i<4&&ok;++i)ok=expiry.advanceAnimation(*records.resolve(key));
            assert(ok==(failure!=4));
        }
        if(ok){assert(records.resolve(key)->destroyRequested&&expiry.state(id));
            ok=expiry.retire(*records.resolve(key));assert(ok==(failure!=5));
        }
        assert(ok==!failure&&good==!failure);
        if(failure){
            assert(records.size()==1&&expiry.state(105187));std::uint32_t other=7;
            assert(!expiry.create(real(1),real(2),other)&&!other&&next==105188);
        }else assert(records.size()==0&&!expiry.state(id));
        if(!failure){
            std::uint32_t replacement=0,overflow=9;
            assert(expiry.create(real(1),real(2),replacement)&&replacement==105188);
            assert(!expiry.create(real(3),real(4),overflow)&&!overflow&&next==105189&&records.size()==1);
            assert(!expiry.state(id)&&expiry.state(replacement));
        }
        audit.reject=0;assert(expiry.clear()&&records.size()==0);
    }
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
            assert(corpses.advanceAnimation(key));
            assert(!corpses.endStep(key,real(10000),true)&&!good&&audit.destroyMarks==1&&
                corpses.state(key)&&!records.resolve(key)->destroyRequested);
            assert(!corpses.endStep(key,real(10000),true)&&audit.destroyMarks==1);
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
    nativeCorpses();
    nativeExpiry();
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
