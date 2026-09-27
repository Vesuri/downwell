#include "core/StartupRuntime.h"
#include "core/RoomPack.h"
#include "core/StoredCollisionLifecycle.h"
#include "core/StoredWorldCollision.h"
#include "core/RunnerObjectFamilies.h"
#include "core/SmokeDirection.h"
#include "core/SmokeTrig.h"
#include "core/GameplayContactProfileInput.h"
#include "core/TimeShardBurst.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <memory>
#include <unordered_map>
#include <vector>
using namespace downwell;
static void require(bool ok,const char* message){if(!ok){std::fprintf(stderr,"%s\n",message);std::exit(1);}}
static std::vector<std::uint8_t> read(const char* path){
    std::ifstream in(path,std::ios::binary);return {std::istreambuf_iterator<char>(in),{}};
}
struct Trig {
    bool operator()(RunnerImageValue angle,RunnerImageValue& c,RunnerImageValue& s)const{
        RunnerImageValue radians;MotionBinary64 cosine,sine;
        return SmokeDirection::radians(angle,radians)&&smokeTrig(radians.promoted(),cosine,sine)&&
            c.assign(cosine)&&s.assign(sine);
    }
};
struct Round {bool operator()(RunnerImageValue v,std::int32_t& out)const{return v.promoted().roundPixel(out);}};
constexpr unsigned capacity=StartupRuntime::instanceCapacity,nodes=4096;
using Owner=RunnerSpatialInstances<capacity,nodes>;
using Families=RunnerObjectFamilies<386>;
using Lifecycle=StoredCollisionLifecycle<capacity,nodes,Families,Trig,Round>;
struct Storage {Owner owner;Owner::Properties properties[capacity];Families families;};
struct RecordingLifecycle:Lifecycle {
    std::unordered_map<std::uint32_t,InstanceHandle> handles;
    std::uint32_t ids[capacity]{};
    unsigned births=0,retires=0;
    unsigned goalWrites=0,rejectGoalStage=0;
    unsigned startWrites=0,rejectStartStage=0,startRetires=0;
    const StartupRuntime* runtime=nullptr;
    const SpriteMaskPack* masks=nullptr;
    unsigned meetingCalls=0;
    bool rejectMeeting=false;
    explicit RecordingLifecycle(Lifecycle base):Lifecycle(base){}
    bool placeMeeting(InstanceHandle h,const InstanceRecord& r,int object,MotionBinary64 x,MotionBinary64 y,bool& hit){
        ++meetingCalls;
        const auto px=MotionBinary64::fromInteger(r.x),py=MotionBinary64::fromInteger(r.y);
        require(runtime&&masks&&r.objectId==35&&object==255&&
            x.high==px.high&&x.low==px.low&&y.high==py.high&&y.low==py.low,
            "clock query uses published player position and goal-family selector");
        if(rejectMeeting)return false;
        struct View {
            const RecordingLifecycle& life;
            const InstanceRecord* resolve(InstanceHandle key)const{
                return life.owner.resolve(key)?life.runtime->instance(life.ids[key.slot]):nullptr;
            }
        } view{*this};
        auto inverse=[](RunnerImageValue angle,RunnerImageValue& c,RunnerImageValue& s){
            angle.bits^=0x80000000u;return Trig{}(angle,c,s);
        };
        return meetingVm(view,h,object,x,y,*masks,RunnerMaskDispatch{},inverse,hit);
    }
    bool birth(InstanceHandle h,const InstanceRecord& r){
        if((rejectGoalStage==1&&r.objectId==74)||(rejectGoalStage==2&&r.objectId==255))return false;
        if(r.objectId==255||r.objectId==371)require(r.scaleX==1.0F&&r.scaleY==1.0F,"field birth precedes Create scales");
        if(!Lifecycle::birth(h,r)){std::fprintf(stderr,"birth failed: object=%d id=%u\n",r.objectId,r.instanceId);return false;}
        require(handles.emplace(r.instanceId,h).second,"duplicate live instance ID");ids[h.slot]=r.instanceId;++births;return true;
    }
    bool transform(InstanceHandle h,const InstanceRecord& r,RunnerTransformField field,
                   RunnerImageValue value,RunnerImageValue alpha,std::uint32_t blend){
        if(r.objectId==255){
            ++goalWrites;
            require(value.bits==0x40000000u&&r.scaleY==1.0F&&
                ((field==RunnerTransformField::scaleX&&r.scaleX==1.0F)||
                 (field==RunnerTransformField::scaleY&&r.scaleX==2.0F)),"goal scales publish X then Y before record writes");
            if((rejectGoalStage==3&&field==RunnerTransformField::scaleX)||
               (rejectGoalStage==4&&field==RunnerTransformField::scaleY))return false;
        }
        if(r.objectId==371){
            ++startWrites;
            require(value.bits==0x40000000u&&r.scaleY==1.0F&&
                ((field==RunnerTransformField::scaleX&&r.scaleX==1.0F)||
                 (field==RunnerTransformField::scaleY&&r.scaleX==2.0F)),"start scales publish X then Y before record writes");
            if((rejectStartStage==1&&field==RunnerTransformField::scaleX)||
               (rejectStartStage==2&&field==RunnerTransformField::scaleY))return false;
        }
        return Lifecycle::transform(h,r,field,value,alpha,blend);
    }
    bool retire(InstanceHandle h,const InstanceRecord& r){
        if(r.objectId==371){
            require(r.scaleX==2.0F&&r.scaleY==2.0F,"start retirement follows both scale writes");
            ++startRetires;
            if(rejectStartStage==3)return false;
        }
        if(!Lifecycle::retire(h,r))return false;
        require(handles.erase(r.instanceId)==1,"retire unknown instance ID");ids[h.slot]=0;++retires;return true;
    }
};
static bool tracked(int id){
    return id==35||id==60||id==92||id==111||id==112||id==113||id==116||id==117||id==119;
}
int main(){
    const auto rb=read("generated/downwell-rooms.dwrm"),ob=read("generated/downwell-objects.dwob"),
        bb=read("generated/downwell-builder-patterns.dwbp"),sb=read("generated/downwell-sprites.dwsc"),
        mb=read("generated/downwell-sprite-masks.dwsm");
    RoomPack rooms(rb.data(),rb.size());ObjectPack objects(ob.data(),ob.size());
    BuilderPatternPack patterns(bb.data(),bb.size());SpriteCollisionPack sprites(sb.data(),sb.size());
    SpriteMaskPack masks(mb.data(),mb.size());
    require(rooms.valid()&&objects.valid()&&patterns.valid()&&sprites.valid()&&masks.valid(),"real collision resources validate");
    {
        auto positioned=std::make_unique<Storage>();
        require(positioned->families.bindInstanceFlags(objects),"positioned birth families");
        Lifecycle adapter{positioned->owner,positioned->properties,objects,sprites,positioned->families,{}, {}};
        InstanceRecord shard;shard.instanceId=105186;shard.objectId=238;
        PackedObject definition;require(objects.findObject(238,definition),"shard resource");
        shard.spriteId=definition.spriteId;shard.maskId=definition.textureMaskId;shard.depth=definition.creationDepth;
        shard.x=1000;shard.y=1000; // Drawing snapshot deliberately differs.
        const InstanceHandle key{0,1};
        const RunnerImageValue x{0x41820000u},y{0xbf000000u}; // 16.25, -0.5
        require(!InstanceLifecycleBinding{}.birthAt(key,shard,x,y),"missing positioned birth is not silent success");
        require(!adapter.birthAt(key,shard,{0x7f800000u},y)&&!positioned->owner.resolve(key),
                "nonfinite birth rejected before attaching lifetime");
        require(adapter.birthAt(key,shard,x,y),"fractional shard inserted directly");
        const auto* born=positioned->owner.resolve(key);
        require(born&&born->x.bits==x.bits&&born->y.bits==y.bits,"birth preserves published fractional coordinates");
        require(adapter.retire(key,shard)&&!positioned->owner.resolve(key),"positioned lifetime retirement");
        auto production=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        const auto binding=production->binding();
        require(!binding.birthAt(key,shard,x,{0x7fc00000u})&&production->valid(),
                "production binding rejects NaN before ownership mutation");
        require(binding.birthAt(key,shard,x,y)&&binding.retire(key,shard),
                "production positioned birth and retirement share ID ownership");
        // Legacy room/terrain births retain their integer coordinate contract.
        require(adapter.birth({0,2},shard),"legacy integer birth unchanged");
        born=positioned->owner.resolve({0,2});
        require(born&&born->x.bits==0x447a0000u&&born->y.bits==0x447a0000u,"legacy birth remains at integer snapshot");
    }
    auto storage=std::make_unique<Storage>();require(storage->families.bindInstanceFlags(objects),"bind full source families");
    RecordingLifecycle lifecycle({storage->owner,storage->properties,objects,sprites,storage->families,{}, {}});
    auto runtime=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
        InstanceLifecycleBinding::bindQueries(lifecycle));
    lifecycle.runtime=runtime.get();lifecycle.masks=&masks;
    auto plain=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns);
    {
        SpriteMaskPack missing(nullptr,0);
        auto invalid=std::make_unique<StoredWorldCollision>(objects,sprites,missing);
        bool hit=true;InstanceRecord record;
        require(!invalid->valid()&&!invalid->attach(*plain)&&
            !invalid->placeMeeting({},record,255,{},{},hit)&&hit,
            "missing collision resources cannot attach or silently answer a successful miss");
    }
    auto productionOwner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
    {
        auto shardOwner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,shardOwner->binding());
        require(shardOwner->attach(*world),"attach shard world");world->skipToGameForTesting();
        PackedObject definition;require(objects.findObject(238,definition)&&definition.parentId==-1,
            "source shard PreCreate has no parent handler");
        auto registration=world->timeShardRegistration<3>();TimeShardEffects<3> shards;TimeShardBurst burst;
        require(shards.bindPhases(registration),"bind actual world shard phases");
        const auto first=world->nextRuntimeInstanceId();const auto draws=world->randomDrawCount();
        const auto count=world->activeInstanceCount();
        require(world->emitTimeShards(burst,shards,{0x43704000u},{0x43308000u},
                MotionBinary64::fromInteger(2),MotionBinary64::fromInteger(-3),false),"world burst completes");
        require(world->randomDrawCount()==draws+24&&world->nextRuntimeInstanceId()==first+3&&
                burst.shard==first+2&&world->activeInstanceCount()==count+3,"burst uses shared world RNG and IDs");
        for(unsigned i=0;i<3;++i){
            const auto* r=world->instance(first+i);const auto* effect=shards.get(i+1);
            require(r&&effect&&r->objectId==238&&r->spriteId==definition.spriteId&&r->maskId==definition.textureMaskId&&
                    r->preciseX==240.25f&&r->preciseY==176.5f&&r->alarms.get(0)==effect->alarm0,
                    "registered world record retains source resource and Create fields");
        }
        bool hit=false;
        for(unsigned i=0;i<world->activeInstanceCount();++i){
            const auto& wall=world->instanceAt(i);if(wall.objectId!=60)continue;
            require(registration.meeting(1,first,MotionBinary64::fromInteger(wall.x),MotionBinary64::fromInteger(wall.y),hit)&&hit,
                    "registered shard queries real solid-family masks");break;
        }
        require(hit,"test queried an actual generated wall");
        require(registration.meeting(1,first,MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(-1000),hit)&&!hit,
                "registered shard empty query succeeds without fabricated hit");
        // Test phase publication in empty space, preserving the old published
        // position during every Step query until the final x/y assignments.
        for(unsigned i=0;i<3;++i){auto* s=shards.get(i+1);s->yy=MotionBinary64::fromInteger(-1000);
            require(s->y.assign(s->yy)&&registration.publishStep(i+1,first+i,*s),"stage above terrain");}
        const auto phase=shards.beginPhase();
        shards.advanceAlarm(phase,2);
        require(shards.succeeded()&&world->instance(first)->alarms.get(2)==29,"publish alarm phase before Step");
        require(shards.advanceAnimation(phase),"publish shard animation phase");
        require(world->stepTimeShards(registration,shards,phase,true,
            [](auto,const auto&,bool& retire){retire=false;return true;}),"registered shard motion with source queries");
        for(unsigned i=0;i<3;++i){
            const auto* r=world->instance(first+i);auto* s=shards.get(i+1);std::int32_t x,y;
            require(r&&s&&s->x.promoted().roundPixel(x)&&s->y.promoted().roundPixel(y)&&
                    r->x==x&&r->y==y&&r->imageSpeed==0&&r->alarms.get(0)==s->alarm0,
                    "stopped shard still publishes motion, zero animation speed and paused lifetime");
        }
        require(shards.clear()&&world->activeInstanceCount()==count&&shardOwner->valid(),"world retirement removes every shard record");
        for(unsigned i=0;i<3;++i)require(!world->instance(first+i),"retired source ID no longer resolves");
        world->step({},false);require(world->effectsSucceeded(),"world continues after registered burst retirement");
        // Exhaust registration capacity after one successful child: preserve
        // that birth and the next scatter/speed draws, never replay the burst.
        auto limitedRegistration=world->timeShardRegistration<1>();TimeShardEffects<3> limited;
        require(limited.bind(limitedRegistration),"bind limited world registration");
        const auto before=world->randomDrawCount();const auto next=world->nextRuntimeInstanceId();
        require(!world->emitTimeShards(burst,limited,{0x43704000u},{0x43308000u},{},{},false)&&
                !world->effectsSucceeded()&&world->randomDrawCount()==before+10&&
                world->nextRuntimeInstanceId()==next+1&&world->instance(next),
                "partial world burst preserves completed birth and exact RNG prefix");
        require(!world->emitTimeShards(burst,limited,{},{},{},{},false)&&world->randomDrawCount()==before+10,
                "failed world burst is terminal");
    }
    require(productionOwner->valid(),"production owner validates resource packs");
    auto production=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
        productionOwner->binding());
    require(productionOwner->attach(*production)&&!productionOwner->attach(*plain),
        "production owner attaches exactly one runtime without sharing its identities");
    production->skipToGameForTesting();
    runtime->skipToGameForTesting();plain->skipToGameForTesting();
    require(!runtime->instance(101097)&&!runtime->startTimeField()&&runtime->randomDrawCount()==331,
        "first-level start field consumes Create RNG then retires its placed lifetime");
    require(lifecycle.startWrites==2&&lifecycle.startRetires==1&&!lifecycle.handles.count(101097),
        "first-level start field retires its registered collision identity");
    unsigned checks=0;
    for(unsigned tick=0;tick<1800;++tick){
        const auto input=gameplayContactProfileInput(tick,runtime->mainPlayerX(),runtime->mainPlayerY());
        runtime->step(input,false);plain->step(input,false);
        production->step(input,false);
        require(production->effectsSucceeded()&&production->roomLoadSucceeded()&&production->deferredCommandsSucceeded()&&
            production->stateDigest()==runtime->stateDigest(),"production collision owner matches audited live query state and RNG");
        if(!runtime->effectsSucceeded()||!runtime->roomLoadSucceeded()||!runtime->deferredCommandsSucceeded()){
            std::fprintf(stderr,"live collision publication failed at tick %u\n",tick);return 1;
        }
        require(runtime->stateDigest()==plain->stateDigest(),"resource-backed lifecycle must preserve state/RNG");
        require(lifecycle.handles.size()==runtime->activeInstanceCount(),"all live stored identities registered");
        for(unsigned i=0;i<runtime->activeInstanceCount();++i){
            const auto& record=runtime->instanceAt(i);const auto found=lifecycle.handles.find(record.instanceId);
            require(found!=lifecycle.handles.end(),"resolve live instance key");
            const auto* state=storage->owner.resolve(found->second);
            require(state&&storage->owner.objectId(found->second)==record.objectId,"owner identity matches live object");
            if(!tracked(record.objectId))continue;
            std::int32_t x,y;
            if(!state->x.promoted().roundPixel(x)||!state->y.promoted().roundPixel(y)||x!=record.x||y!=record.y||
               bool(state->flags&2u)==record.active){
                std::fprintf(stderr,"stale owner tick=%u object=%d id=%u\n",tick,record.objectId,record.instanceId);return 1;
            }
            // This audit checks publication, not source traversal history. Do
            // not refresh/reinsert every object or inject diagnostic queries.
            ++checks;
        }
    }
    require(lifecycle.meetingCalls>0,"natural player updates execute the maintained collision query");
    production->reset(PlatformProfile::nonDesktop);production->skipToGameForTesting();
    require(production->effectsSucceeded()&&production->roomLoadSucceeded()&&production->deferredCommandsSucceeded(),
        "production collision owner survives room retirement and new generations");
    {
        const RunnerImageValue x{0x43700000u},y{0x43300000u}; // 240,176
        InstancePlaceResult selected;selected.id=17;
        require(production->playerTimeFieldInstance(x,y,selected)&&selected.id==-4,
            "first-level identity lookup sees no phantom entrance field");
        production->registerLevelExit(176);
        require(production->playerTimeFieldInstance(x,y,selected)&&selected.id>0,
            "parent-field identity lookup returns a live source instance ID");
        const auto* record=productionOwner->resolve(selected.handle);
        require(record&&record==production->instance(selected.id)&&record->objectId==255&&
            selected.x.high==MotionBinary64::fromInteger(record->x).high&&
            selected.x.low==MotionBinary64::fromInteger(record->x).low&&
            selected.y.high==MotionBinary64::fromInteger(record->y).high&&
            selected.y.low==MotionBinary64::fromInteger(record->y).low,
            "selected field identity and published position refer to the same lifetime");
        // Explicit setter fixture: query results must use the collision owner's
        // published binary32 position, not the legacy integer record snapshot.
        const RunnerImageValue fractionalX{0x43704000u},fractionalY{0x43508000u}; // 240.25,208.5
        const auto originalId=selected.id;
        require(productionOwner->move(selected.handle,*record,fractionalX,fractionalY)&&
            production->playerTimeFieldInstance(x,y,selected)&&selected.id==originalId&&
            selected.x.high==fractionalX.promoted().high&&selected.x.low==fractionalX.promoted().low&&
            selected.y.high==fractionalY.promoted().high&&selected.y.low==fractionalY.promoted().low,
            "field lookup preserves fractional published coordinates");
        RunnerImageValue restoredX,restoredY;
        require(restoredX.assign(MotionBinary64::fromInteger(record->x))&&restoredY.assign(MotionBinary64::fromInteger(record->y))&&
            productionOwner->move(selected.handle,*record,restoredX,restoredY),"restore explicit field setter fixture");
        production->registerLevelExit(186); // Deliberate overlapping-field fixture.
        const auto digest=production->stateDigest();
        TimeShardEffects<3> effects;TimeShardBurst burst;GameMakerRandom random(42);
        unsigned lookups=0;InstancePlaceResult last;
        require(burst.emit(x,y,{}, {},false,random,effects,
            [&](auto px,auto py,auto& id,auto& fx,auto& fy){
                ++lookups;
                if(!production->playerTimeFieldInstance(px,py,last))return false;
                id=last.id;fx=last.x;fy=last.y;return true;
            },[&](auto ax,auto ay,auto bx,auto by,auto& angle){
                require(lookups==2&&ax.high==last.x.high&&ax.low==last.x.low&&ay.high==last.y.high&&ay.low==last.y.low&&
                    bx.high==x.promoted().high&&by.high==y.promoted().high,
                    "shard direction uses the second maintained query's selected field");
                angle=MotionBinary64::fromInteger(30);return true;
            },[](auto,auto,bool,auto& out){out={};return true;}),"source shard script consumes live field identities");
        require(lookups==2&&burst.field==last.id&&effects.size()==3&&random.drawCount()==24&&
            production->stateDigest()==digest,"identity queries preserve world/RNG while shard Create consumes its ordered draws");
        const auto old=selected.handle;
        production->reset(PlatformProfile::nonDesktop);production->skipToGameForTesting();
        require(!productionOwner->resolve(old)&&production->playerTimeFieldInstance(x,y,selected)&&selected.id==-4&&
            !selected.handle.generation&&!selected.x.high&&!selected.x.low&&!selected.y.high&&!selected.y.low,
            "retired field result cannot alias a new room or persist through a miss");
        production->registerLevelExit(int(production->mainPlayerY()));
        Input confirm;confirm.fire=true;bool heartGoal=false;
        for(unsigned frame=0;frame<600&&production->runStage().level==1;++frame){
            if(production->levelClearActive()){
                bool fire=true;
                production->stepLevelClear(fire,false,false,heartGoal,[](LevelClearSelection::Effect,unsigned){});
                if(production->nextLevelRequested())require(production->loadNextAreaOneLevel(confirm),"identity-owned next room");
            }else{
                production->step(confirm,false);
                if(production->levelResultRequested())require(production->enterLevelClear(0,heartGoal),"identity-owned clear");
            }
        }
        require(production->effectsSucceeded()&&production->runStage().level==2&&
            production->playerTimeFieldInstance(x,y,selected)&&selected.id==101097&&
            productionOwner->resolve(selected.handle)&&
            productionOwner->resolve(selected.handle)->objectId==371,
            "identity lookup resolves the later-level entrance descendant through parent family 82");
    }
    // Resolve through actual world ownership, not a copied InstanceStore or a
    // rebuilt tree. Generation validation precedes looking up its instance ID.
    struct View {
        const StartupRuntime& runtime;const RecordingLifecycle& lifecycle;
        const InstanceRecord* resolve(InstanceHandle h)const{
            if(!lifecycle.owner.resolve(h))return nullptr;
            return runtime.instance(lifecycle.ids[h.slot]);
        }
    } view{*runtime,lifecycle};
    InstanceHandle player;
    for(unsigned i=0;i<runtime->activeInstanceCount();++i){
        const auto& r=runtime->instanceAt(i);
        if(r.objectId==35)player=lifecycle.handles.at(r.instanceId);
    }
    require(view.resolve(player),"resolve live player for precise terrain queries");
    const auto original=*storage->owner.resolve(player);
    const auto digest=runtime->stateDigest();
    RunnerMaskDispatch dispatch;unsigned queries=0;
    auto inverse=[](RunnerImageValue angle,RunnerImageValue& c,RunnerImageValue& s){
        angle.bits^=0x80000000u;return Trig{}(angle,c,s);
    };
    for(unsigned i=0;i<runtime->activeInstanceCount()&&queries<128;++i){
        const auto& wall=runtime->instanceAt(i);
        if(!wall.active||wall.objectId!=60)continue;
        Owner::Result result;
        require(lifecycle.placeVm(view,player,57,MotionBinary64::fromInteger(wall.x),
                    MotionBinary64::fromInteger(wall.y),masks,dispatch,inverse,result),"live precise instance_place succeeds");
        const auto* selected=result.found?view.resolve(result.handle):nullptr;
        require(selected&&selected->active&&storage->families.isA(selected->objectId,57),
                "terrain query returns a resolvable live wall-family identity");
        require(storage->owner.resolve(player)->x.bits==original.x.bits&&storage->owner.resolve(player)->y.bits==original.y.bits,
                "temporary instance_place displacement restores published player position");
        ++queries;
    }
    Owner::Result miss;
    require(queries==128&&lifecycle.placeVm(view,player,57,MotionBinary64::fromInteger(-100000),
                MotionBinary64::fromInteger(-100000),masks,dispatch,inverse,miss)&&!miss.found,
                "empty query cannot reuse a previous hit");
    require(runtime->stateDigest()==digest,"diagnostic queries leave world state and RNG untouched");
    const auto beforeExit=runtime->activeInstanceCount();
    auto expectedRandom=runtime->randomState();
    const auto expectedRadius=GoalTimeField::nextRadius(expectedRandom);
    runtime->registerLevelExit(6000);
    require(runtime->effectsSucceeded()&&runtime->deferredCommandsSucceeded()&&runtime->levelExitPresent()&&
        runtime->activeInstanceCount()==beforeExit+3&&lifecycle.goalWrites==2,"exit owns parent and synchronous children");
    for(unsigned i=0;i<3;++i){
        const auto& r=runtime->instanceAt(beforeExit+i);
        require(r.objectId==(i==0?306:i==1?74:255)&&r.x==240&&r.y==(i==2?6032:6000)&&r.active,
                "exit birth order and published positions match VM Create");
        if(i)require(r.instanceId==runtime->instanceAt(beforeExit+i-1).instanceId+1,"children consume IDs synchronously");
    }
    const auto& goal=runtime->instanceAt(beforeExit+2);
    require(goal.scaleX==2.0F&&goal.scaleY==2.0F&&runtime->goalField().radius.equal(expectedRadius)&&
        runtime->randomDrawCount()==expectedRandom.drawCount(),"goal Create consumes one radius draw after scaling");
    for(unsigned i=0;i<16;++i)require(runtime->randomState().stateWord(i)==expectedRandom.stateWord(i),"exit full RNG state");
    // Ninety pixels from center misses the unscaled circle, but lies inside
    // the actual doubled mask. Both direct and parent selectors must find it.
    for(int selector:{255,82}){
        Owner::Result result;
        require(lifecycle.placeVm(view,player,selector,MotionBinary64::fromInteger(330),
            MotionBinary64::fromInteger(6032),masks,dispatch,inverse,result)&&result.found&&
            view.resolve(result.handle)==&goal,"live precise query resolves scaled goal child through its family");
    }
    runtime->reset(PlatformProfile::nonDesktop);runtime->skipToGameForTesting();
    require(runtime->effectsSucceeded()&&runtime->roomLoadSucceeded()&&runtime->deferredCommandsSucceeded()&&
            !storage->owner.resolve(player)&&lifecycle.handles.size()==runtime->activeInstanceCount(),
            "room reset retires queried identities before new births reuse slots");
    std::printf("Live resource owner: %u births, %u retirements, %u position/activation checks\n",
        lifecycle.births,lifecycle.retires,checks);
    std::printf("Live precise terrain queries: %u hits and one empty query\n",queries);
    for(bool reject:{false,true}){
        auto clockStorage=std::make_unique<Storage>();
        require(clockStorage->families.bindInstanceFlags(objects),"clock query family fixture");
        RecordingLifecycle clockLife({clockStorage->owner,clockStorage->properties,objects,sprites,clockStorage->families,{}, {}});
        auto clock=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
            InstanceLifecycleBinding::bindQueries(clockLife));
        clockLife.runtime=clock.get();clockLife.masks=&masks;
        clock->skipToGameForTesting();
        clock->registerLevelExit(int(clock->mainPlayerY()));
        require(clock->effectsSucceeded()&&clock->levelExitPresent()&&!clock->goalStopped(),"live overlapping clock fixture");
        const auto ticks=clock->runTimeTicks(),calls=clockLife.meetingCalls;
        clockLife.rejectMeeting=reject;
        clock->step(Input{},false);
        require(clockLife.meetingCalls==calls+1&&clock->effectsSucceeded()==!reject&&clock->runTimeTicks()==ticks,
                "live field hit suppresses clock; failed query stops rather than falling back to a miss");
        if(reject){
            clock->step(Input{},false);
            require(clockLife.meetingCalls==calls+1&&clock->runTimeTicks()==ticks,"latched query failure prevents retry");
        }else{
            Input confirm;confirm.fire=true;bool heartGoal=false;
            for(unsigned frame=0;frame<600&&clock->runStage().level==1;++frame){
                if(clock->levelClearActive()){
                    bool fire=true;
                    clock->stepLevelClear(fire,false,false,heartGoal,[](LevelClearSelection::Effect,unsigned){});
                    if(clock->nextLevelRequested())require(clock->loadNextAreaOneLevel(confirm),"load next collision-owned room");
                }else{
                    clock->step(confirm,false);
                    if(clock->levelResultRequested())require(clock->enterLevelClear(0,heartGoal),"enter collision-owned level clear");
                }
            }
            if(clock->runStage().level!=2||!clock->effectsSucceeded())std::fprintf(stderr,
                "clock reentry: level=%u good=%d load=%d deferred=%d clear=%d next=%d exit=%d stopped=%d xy=%.1f,%.1f\n",
                unsigned(clock->runStage().level),clock->effectsSucceeded(),clock->roomLoadSucceeded(),clock->deferredCommandsSucceeded(),
                clock->levelClearActive(),clock->nextLevelRequested(),clock->levelResultRequested(),clock->goalStopped(),
                double(clock->mainPlayerX()),double(clock->mainPlayerY()));
            require(clock->effectsSucceeded()&&clock->runStage().level==2&&clock->startTimeField()&&
                clock->instance(101097)&&clockLife.handles.count(101097),"next room publishes its live entrance descendant");
            const auto entryTicks=clock->runTimeTicks(),entryCalls=clockLife.meetingCalls;
            clock->step(Input{},false);
            require(clock->effectsSucceeded()&&clockLife.meetingCalls==entryCalls+1&&clock->runTimeTicks()==entryTicks,
                "maintained goal-family query finds the later-level entrance descendant");
        }
    }
    for(unsigned failure=1;failure<=3;++failure){
        auto failedStorage=std::make_unique<Storage>();
        require(failedStorage->families.bindInstanceFlags(objects),"start failure family fixture");
        RecordingLifecycle failedLifecycle({failedStorage->owner,failedStorage->properties,objects,sprites,failedStorage->families,{}, {}});
        failedLifecycle.rejectStartStage=failure;
        auto failed=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
            InstanceLifecycleBinding::bindMasks(failedLifecycle));
        failed->skipToGameForTesting();
        const auto* partial=failed->instance(101097);
        require(!failed->effectsSucceeded()&&partial&&partial->scaleX==(failure==1?1.0F:2.0F)&&
            partial->scaleY==(failure==3?2.0F:1.0F)&&failed->randomDrawCount()==(failure==3?331u:330u),
            "start failure preserves accepted scales and samples radius only before retirement");
        require(failedLifecycle.startWrites==(failure==1?1u:2u)&&failedLifecycle.startRetires==(failure==3?1u:0u),
            "start failure stops subsequent publication");
        const auto digest=failed->stateDigest();
        failed->step({},false);
        require(failed->stateDigest()==digest,"failed start Create remains terminal");
    }
    for(unsigned failure=1;failure<=4;++failure){
        auto failedStorage=std::make_unique<Storage>();
        require(failedStorage->families.bindInstanceFlags(objects),"failed-birth family fixture");
        RecordingLifecycle failedLifecycle({failedStorage->owner,failedStorage->properties,objects,sprites,failedStorage->families,{}, {}});
        auto failed=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
            InstanceLifecycleBinding::bindMasks(failedLifecycle));
        failed->skipToGameForTesting();
        failedLifecycle.rejectGoalStage=failure;
        const auto records=failed->activeInstanceCount();const auto draws=failed->randomDrawCount();
        failed->registerLevelExit(6000);
        require(!failed->effectsSucceeded()&&!failed->deferredCommandsSucceeded()&&!failed->levelExitPresent()&&
            failed->randomDrawCount()==draws&&failed->activeInstanceCount()==records+(failure==1?2:3),
            "rejected child birth/scale stops Create before radius RNG and preserves earlier records");
        if(failure>=2){
            const auto& partial=failed->instanceAt(records+2);
            require(partial.objectId==255&&partial.scaleX==(failure==4?2.0F:1.0F)&&partial.scaleY==1.0F,
                    "failed goal creation retains exactly the accepted scale writes");
        }
        const auto count=failed->activeInstanceCount();const auto writes=failedLifecycle.goalWrites;
        failed->registerLevelExit(7000);
        require(failed->activeInstanceCount()==count&&failed->randomDrawCount()==draws&&failedLifecycle.goalWrites==writes,
                "failed exit creation cannot replay children or scale writes");
    }
}
