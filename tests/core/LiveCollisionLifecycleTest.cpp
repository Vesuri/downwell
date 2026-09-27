#include "core/StartupRuntime.h"
#include "core/TerrainCreate.h"
#include "core/RoomPack.h"
#include "core/StoredCollisionLifecycle.h"
#include "core/StoredWorldCollision.h"
#include "core/TimeChangeEffects.h"
#include "core/EventScheduler.h"
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
        auto storage=std::make_unique<Storage>();require(storage->families.bindInstanceFlags(objects),"scheduled-change families");
        // This scheduler fixture owns its records directly, not through a
        // StartupRuntime. Collision queries are deliberately unavailable.
        struct AlarmLifecycle:Lifecycle {
            explicit AlarmLifecycle(Lifecycle base):Lifecycle(base){}
            bool placeMeeting(InstanceHandle,const InstanceRecord&,int,MotionBinary64,MotionBinary64,bool&){return false;}
            bool instancePlace(InstanceHandle,const InstanceRecord&,int,MotionBinary64,MotionBinary64,InstancePlaceResult&){return false;}
            bool instanceNumber(int,unsigned&){return false;}
        } lifecycle({storage->owner,storage->properties,objects,sprites,storage->families,{}, {}});
        InstanceStore<4> records;std::uint32_t nextId=100;int room=11,speed=60;bool healthy=true;
        StoredTimeChangeRegistration<4,2> registration(records,&objects,InstanceLifecycleBinding::bindPositionedBirths(lifecycle),nextId,room,healthy);
        TimeChangeEffects<2> effects;require(effects.bind(registration),"bind scheduler-selected changes");
        require(effects.createAt({},{},speed,[](unsigned){return true;}),"first scheduled change");
        require(records.create(99,36,room,false,0)==CreateInstanceResult::success,"interleaved scheduler actor");
        records.find(99)->alarms.set(1,2);
        require(effects.createAt({},{},speed,[](unsigned){return true;}),"second scheduled change");
        speed=55;std::vector<unsigned> order;
        auto tick=[&]{return EventScheduler<4>::runStepChecked(records,
            [](const auto& r){return r.objectId==211?3u:2u;},
            [&](auto& r,StepEvent event){
                if(event.phase!=StepEventPhase::alarm)return;
                if(event.subtype==1)order.push_back(r.instanceId);
                if(r.objectId==211){
                    healthy=registration.dispatchAlarm(r,effects,event.subtype,speed);
                }else speed=58;
            },[](auto){},[](auto,const auto&,bool){return true;},[&]{return healthy;});};
        require(tick()&&speed==55&&order.empty()&&records.find(100)->alarms.get(1)==1,
            "global scheduler decrements each registered alarm exactly once");
        require(tick()&&speed==63&&order==std::vector<unsigned>({100,99,101})&&
            records.find(100)->alarms.get(0)==0&&records.find(101)->alarms.get(1)==2,
            "selected effects interleave with other actors and preserve source unclamped speed");
        const bool third=tick();const auto thirdAlarm=records.find(100)->alarms.get(0);const bool fourth=tick();
        require(third&&thirdAlarm==-1&&fourth&&speed==60&&effects.size()==0&&
            records.find(100)->destroyRequested&&records.find(101)->destroyRequested&&
            records.find(100)->alarms.get(0)==-1&&records.find(100)->alarms.get(1)==0,
            "event publication cannot resurrect a stale sibling countdown; retirement is deferred");
        unsigned retired=0;
        require(records.cleanupDestroyed([&](InstanceHandle key,const InstanceRecord& record){
            const auto id=record.instanceId;
            if(!registration.retireMarked(key,record,effects))return false;
            ++retired;
            require(!records.find(id)&&records.size()==3-retired&&
                (retired!=1||records.find(id==100?101:100)),
                "selected cleanup retires exactly one lifetime without sweeping marked siblings");
            return true;
        })&&retired==2&&records.size()==1,"scheduled changes retire through global cleanup");
        require(effects.createAt({},{},speed,[](unsigned){return true;}),"new lifetime after scheduled retirement");
        const auto changeId=nextId-1;
        StoredTimeShardRegistration<4,2> shardRegistration(records,&objects,InstanceLifecycleBinding::bindPositionedBirths(lifecycle),nextId,room,healthy);
        TimeShardEffects<2> shards;GameMakerRandom random(123);
        require(shards.bindPhases(shardRegistration)&&shards.create({},{},false,random)&&
            shards.create({},{},false,random),"register shards in mixed alarm fixture");
        const auto a=shards.sourceId(1),b=shards.sourceId(2);const auto draws=random.drawCount();
        for(auto id:{a,b}){auto& alarms=records.find(id)->alarms;alarms.set(0,-1);alarms.set(1,1);alarms.set(2,5);}
        records.find(99)->alarms.set(1,1);records.find(changeId)->alarms.set(1,1);
        auto mixedTick=[&]{return EventScheduler<4>::runStepChecked(records,
            [](const auto& r){return r.objectId==238?7u:r.objectId==211?3u:2u;},
            [&](auto& r,StepEvent event){
                if(event.phase!=StepEventPhase::alarm)return;
                if(r.objectId==238)healthy=shardRegistration.dispatchAlarm(r,shards,event.subtype);
                else if(r.objectId==211)healthy=registration.dispatchAlarm(r,effects,event.subtype,speed);
                else records.find(a)->alarms.set(2,1);
            },[](auto){},[](auto,const auto&,bool){return true;},[&]{return healthy;});};
        require(mixedTick()&&speed==35&&shards.get(1)->disappearing&&shards.get(2)->disappearing&&
            !shards.get(1)->noTouch&&shards.get(2)->noTouch&&records.find(a)->alarms.get(2)==0&&
            records.find(b)->alarms.get(2)==4&&records.find(a)->alarms.get(0)==-1,
            "mixed alarm phase observes another actor's later-alarm write without stale counter publication");
        for(auto id:{a,b}){auto& alarms=records.find(id)->alarms;alarms.set(0,1);alarms.set(1,1);alarms.set(2,1);}
        require(mixedTick()&&shards.size()==0&&records.find(a)->destroyRequested&&records.find(b)->destroyRequested&&
            records.find(a)->alarms.get(0)==0&&records.find(a)->alarms.get(1)==1&&records.find(a)->alarms.get(2)==1&&
            random.drawCount()==draws,"shard expiry publishes zero then excludes later alarms without RNG");
        require(records.cleanupDestroyed([&](InstanceHandle key,const InstanceRecord& r){
            return r.objectId==238?shardRegistration.retireMarked(key,r,shards):registration.retireMarked(key,r,effects);
        })&&records.size()==2,"mixed scheduler shard cleanup");
        require(effects.clear()&&effects.createAt({},{},speed,[](unsigned){return true;}),"replace remaining change for identity test");
        const InstanceRecord copied=*records.find(nextId-1);
        require(!registration.dispatchAlarm(copied,effects,1,speed)&&!healthy&&speed==30,
            "world-selected dispatch rejects a copied record rather than trusting its numeric ID");
    }
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
        const auto pending=positioned->owner.pendingSpatial();
        require(adapter.markDestroyed(key,shard)&&positioned->owner.resolve(key)&&
                (positioned->owner.resolve(key)->flags&1u)&&positioned->owner.pendingSpatial()==pending,
                "Destroy marks query exclusion without releasing lifetime or queue membership");
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
    {
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach collision collection world");
        world->skipToGameForTesting();
        auto registration=world->timeShardRegistration<3>();TimeShardEffects<3> shards;TimeShardBurst burst;
        require(shards.bindPhases(registration),"bind collection fixture shards");
        RunnerImageValue x,y;
        require(x.assign(MotionBinary64::fromInteger(static_cast<int>(world->mainPlayerX())))&&
            y.assign(MotionBinary64::fromInteger(static_cast<int>(world->mainPlayerY())))&&
            world->emitTimeShards(burst,shards,x,y,{},{},false),
            "create real collision-capable shards at the player");
        auto pairs=std::make_unique<RunnerCollisionPairs<capacity,16384>>();
        require(world->roomLoadSucceeded()&&world->deferredCommandsSucceeded()&&owner->collectCollisionPairs(*pairs,true),
            "collect source-resource collision candidates from live room births");
        require(pairs->size()>0,"generated room exercises collision candidates");
        unsigned preciseHits=0;
        RunnerCollisionEventContext eventContext;unsigned handlers=0;
        for(unsigned i=0;i<pairs->size();++i){
            const auto* pair=pairs->at(i);
            const auto* a=owner->resolve(pair->self);const auto* b=owner->resolve(pair->other);
            bool ab=false,ba=false;
            require(a&&b&&a!=b&&objects.collisionEventExists(a->objectId,b->objectId,ab)&&
                objects.collisionEventExists(b->objectId,a->objectId,ba)&&(ab||ba),
                "live candidates retain real identities and inherited event eligibility");
            PackedObjectEvent forward{},backward{};bool hasForward=false,hasBackward=false;
            require(owner->collisionEvent(pair->self,pair->other,hasForward,forward)&&
                owner->collisionEvent(pair->other,pair->self,hasBackward,backward)&&
                hasForward==ab&&hasBackward==ba,
                "live pair directions resolve handlers using actual lifetime identities");
            if(hasForward)require(forward.eventType==4&&forward.codeId>=0,"forward handler has source code binding");
            if(hasBackward)require(backward.eventType==4&&backward.codeId>=0,"reverse handler has source code binding");
            const auto beforeHandlers=handlers;
            require(owner->dispatchCollisionEvent(eventContext,pair->self,pair->other,{true,false},
                [&](const RunnerCollisionEventContext::Frame& frame){
                    ++handlers;return frame.subtype==b->objectId&&frame.owner==forward.ownerObjectId&&
                        frame.code==forward.codeId&&owner->resolve(frame.self)==a&&owner->resolve(frame.other)==b;
                })&&handlers==beforeHandlers+unsigned(hasForward)&&!eventContext.current(),
                "live collision context separates actual subtype from inherited event owner");
            bool hit=false;std::uint32_t before=0,after=0;
            require(owner->collisionOverlap(pair->self,pair->other,hit)&&
                owner->collisionFlags(pair->other,before)&&owner->queueCollision(pair->other)&&
                owner->collisionFlags(pair->other,after)&&before==after,
                "live precise pair query and collision requeue preserve instance flags");
            preciseHits+=hit;
        }
        require(preciseHits>0,"live shard/player pairs exercise precise-mask overlap");
        const auto phase=shards.beginPhase();
        for(unsigned tick=0;tick<30;++tick)shards.advanceAlarm(phase,2);
        require(shards.succeeded()&&!shards.get(1)->noTouch,"expire fixture shard contact cooldown");
        struct NoSolidMotion {
            bool restorePath(InstanceHandle){return false;}
            bool preparePath(InstanceHandle){return false;}
            bool engineVelocity(InstanceHandle,RunnerCollisionMotion::Velocity&){return false;}
        } motion;
        const auto beforeContactDraws=world->randomDrawCount();unsigned contacts=0;
        require(owner->executeCollisionPairs(*world,*pairs,eventContext,motion,
            []{return RunnerCollisionEventContext::RoomState{true,false};},
            [&](const RunnerCollisionEventContext::Frame& frame){
                const auto* self=owner->resolve(frame.self);const auto* other=owner->resolve(frame.other);
                if(!self||!other||frame.code!=2081||self->objectId!=238||other->objectId!=35)return false;
                ++contacts;
                return world->dispatchTimeShardContact(registration,shards,frame)&&
                    world->instance(self->instanceId)->alarms.get(2)==5;
            })&&contacts==preciseHits&&contacts>0&&world->randomDrawCount()==beforeContactDraws+2*contacts,
            "source-ordered live pair execution dispatches real shard contacts and exact RNG/cooldown effects");
        const auto tracked=pairs->at(0)->self;
        const auto* beforeFrame=owner->resolve(tracked);
        require(beforeFrame,"snapshot fixture lifetime resolves");
        RunnerImageValue expectedX,expectedY;
        std::memcpy(&expectedX.bits,&beforeFrame->preciseX,4);
        std::memcpy(&expectedY.bits,&beforeFrame->preciseY,4);
        unsigned scheduledPasses=0,scheduledContacts=0;
        require(world->stepWithCollisionPass(Input{},false,[&]{
            ++scheduledPasses;
            const auto before=world->randomDrawCount();
            return owner->collectCollisionPairs(*pairs,true)&&
                owner->executeCollisionPairs(*world,*pairs,eventContext,motion,
                    []{return RunnerCollisionEventContext::RoomState{true,false};},
                    [&](const RunnerCollisionEventContext::Frame& frame){
                        const auto* self=owner->resolve(frame.self);const auto* other=owner->resolve(frame.other);
                        if(!self||!other||frame.code!=2081||self->objectId!=238||other->objectId!=35)return false;
                        ++scheduledContacts;
                        return world->dispatchTimeShardContact(registration,shards,frame);
                    })&&world->randomDrawCount()==before;
        })&&scheduledPasses==1&&scheduledContacts>0,
            "ordinary scheduler executes live collected contacts once; cooldown consumes no RNG");
        RunnerCollisionMotion::Previous previous;
        require(world->effectsSucceeded()&&owner->collisionPreviousPosition(tracked,previous)&&
            previous.x.bits==expectedX.bits&&previous.y.bits==expectedY.bits,
            "ordinary live tick snapshots authoritative position before animation and Step");
        world->step(Input{},false);
        require(world->effectsSucceeded()&&scheduledPasses==1,
            "borrowed collision callback is removed before the next ordinary tick");
        const auto savedRecord=*owner->resolve(tracked);
        const auto drawsBeforeMove=world->randomDrawCount();
        RunnerImageValue movedX{0x43706000u},movedY{0x4330a000u}; // 240.375, 176.625
        require(world->publishCollisionPosition(tracked,movedX,movedY),"publish fractional collision x/y");
        RunnerImageValue actualX,actualY;
        const auto* movedRecord=owner->resolve(tracked);
        require(owner->collisionPosition(tracked,actualX,actualY)&&actualX.bits==movedX.bits&&actualY.bits==movedY.bits&&
            movedRecord->x==240&&movedRecord->y==177&&movedRecord->preciseX==savedRecord.preciseX&&
            movedRecord->preciseY==savedRecord.preciseY&&world->randomDrawCount()==drawsBeforeMove,
            "collision publication preserves fractional owner coordinates without rewriting motion caches or RNG");
        require(owner->rollbackCollisionPosition(*world,tracked)&&owner->collisionPosition(tracked,actualX,actualY)&&
            actualX.bits==previous.x.bits&&actualY.bits==previous.y.bits,
            "live collision rollback publishes this frame's captured position");
        RunnerCollisionMotion::Velocity engine{{0x3e800000u},{0xbf000000u}}; // +.25, -.5
        RunnerImageValue expectedMovedX,expectedMovedY;
        require(RunnerCollisionMotion::advance(previous.x,previous.y,engine,[&](auto x,auto y){
            expectedMovedX=x;expectedMovedY=y;return true;
        })&&owner->advanceCollisionPosition(*world,tracked,engine)&&owner->collisionPosition(tracked,actualX,actualY)&&
            actualX.bits==expectedMovedX.bits&&actualY.bits==expectedMovedY.bits,
            "live collision reapplication uses explicit engine speeds and binary32 coordinates");
        require(owner->collectCollisionPairs(*pairs,true)&&owner->valid(),
            "next collection resolves retained animated masks through real sprite resources");
        require(!world->publishCollisionPosition({tracked.slot,tracked.generation+1},movedX,movedY)&&
            !world->effectsSucceeded()&&!world->publishCollisionPosition(tracked,movedX,movedY),
            "stale collision publication latches world failure and prevents retry");
    }
    for(unsigned mode=0;mode<5;++mode){
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach live shard handler world");world->skipToGameForTesting();
        for(unsigned tick=0;tick<240&&!world->mainPlayerGrounded();++tick)world->step(Input{},false);
        world->step(Input{},false); // Release the style-selection fire edge.
        require(world->effectsSucceeded()&&world->mainPlayerGrounded(),"land before directional jump contact test");
        auto registration=world->timeShardRegistration<3>();TimeShardEffects<3> shards;TimeShardBurst burst;
        require(shards.bindPhases(registration),"bind directional contact shards");
        RunnerImageValue x,y;
        require(x.assign(MotionBinary64::fromInteger(int(world->mainPlayerX())))&&
            y.assign(MotionBinary64::fromInteger(int(world->mainPlayerY())))&&
            world->emitTimeShards(burst,shards,x,y,{},{},false),"emit directional contact shards");
        auto pairs=std::make_unique<RunnerCollisionPairs<capacity,16384>>();
        require(owner->collectCollisionPairs(*pairs,true),"collect directional fixture identities");
        RunnerCollisionEventContext::Frame event;bool found=false;
        for(unsigned i=0;i<pairs->size()&&!found;++i){
            const auto pair=*pairs->at(i);
            for(unsigned reverse=0;reverse<2&&!found;++reverse){
                const auto self=reverse?pair.other:pair.self,other=reverse?pair.self:pair.other;
                if(owner->resolve(self)->instanceId==shards.sourceId(1)&&owner->resolve(other)->objectId==35){
                    event={self,other,4,35,238,2081};found=true;
                }
            }
        }
        require(found,"resolve exact player and registered shard lifetime for handler routing");
        Input jump; jump.fire=true;jump.right=mode!=1;jump.left=mode==1;
        world->step(jump,true);
        require(world->effectsSucceeded()&&world->mainPlayerSpeedY()<0&&
            (mode==1?world->mainPlayerSpeedX()<0:world->mainPlayerSpeedX()>0),
            "directional fixture has real nonzero player jump velocities");
        shards.get(1)->noTouch=false;
        if(mode==2)++event.code;
        if(mode==3)++event.self.generation;
        if(mode==4)++event.other.generation;
        const auto draws=world->randomDrawCount();
        const bool ok=world->dispatchTimeShardContact(registration,shards,event);
        require(ok==(mode<2)&&world->randomDrawCount()==draws+(mode<2?2:0),
            "event identity validation precedes contact RNG and fails loudly");
        if(mode<2){
            const auto* shard=shards.get(1);
            require(!shard->xsp.zero()&&!shard->ysp.zero()&&
                bool(shard->xsp.high&0x80000000u)==(mode==1)&&bool(shard->ysp.high&0x80000000u)&&
                shard->noTouch&&world->instance(shards.sourceId(1))->alarms.get(2)==5,
                "live handler inherits actual horizontal and upward jump signs and publishes cooldown");
        }else require(!world->effectsSucceeded()&&!world->dispatchTimeShardContact(registration,shards,event)&&
            world->randomDrawCount()==draws,"invalid collision routing stays terminal");
    }
    for(bool recursive:{false,true}){
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach failing collision phase world");world->skipToGameForTesting();
        unsigned calls=0;std::uint32_t draws=0;
        require(!world->stepWithCollisionPass(Input{},false,[&]{
            ++calls;draws=world->randomDrawCount();
            if(!recursive)return false;
            require(!world->stepWithCollisionPass(Input{},false,[]{return true;}),"reject recursive scoped collision tick");
            return true; // Ignoring nested failure must not resume End Step.
        })&&!world->effectsSucceeded()&&calls==1&&world->randomDrawCount()==draws,
            "collision failure stops later phases and latches ignored nested errors");
        const auto tick=world->tick();
        require(!world->stepWithCollisionPass(Input{},false,[&]{++calls;return true;})&&
            calls==1&&world->tick()==tick,"failed collision world cannot resume or reuse borrowed callback");
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
    {
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach time-change world");world->skipToGameForTesting();
        PackedObject definition;
        require(objects.findObject(211,definition)&&definition.parentId==-1&&definition.spriteId==-1,
            "time-change resource has no parent or visible sprite");
        auto registration=world->timeChangeRegistration<1>();TimeChangeEffects<1> changes;
        require(changes.bind(registration),"bind real time-change registration");
        const auto first=world->nextRuntimeInstanceId();const auto draws=world->randomDrawCount();
        const auto count=world->activeInstanceCount();int speed=60;unsigned pauses=0;
        require(world->createTimeChange(registration,changes,RunnerImageValue{0x43704000u}.promoted(),RunnerImageValue{0x43308000u}.promoted(),
            speed,[&](unsigned ms){
                const auto* r=world->instance(first);
                require(ms==30&&r&&r->objectId==211&&r->preciseX==240.25f&&r->preciseY==176.5f&&
                    r->alarms.get(0)==2&&r->alarms.get(1)==2&&speed==30,
                    "world record and initialized alarms exist before synchronous pause");
                ++pauses;return true;
            }),"registered time-change Create");
        require(pauses==1&&changes.sourceId(1)==first&&world->nextRuntimeInstanceId()==first+1&&
            world->randomDrawCount()==draws,"time-change consumes one shared ID but no RNG");
        auto shardRegistration=world->timeShardRegistration<3>();TimeShardEffects<3> shards;TimeShardBurst burst;
        require(shards.bindPhases(shardRegistration)&&world->emitTimeShards(burst,shards,
            {0x43700000u},{0x43300000u},{},{},false),"following burst uses same world registry");
        require(shards.sourceId(1)==first+1&&shards.sourceId(3)==first+3&&world->randomDrawCount()==draws+24,
            "time-change ID precedes following shard IDs without perturbing RNG");
        for(unsigned i=0;i<14;++i){const auto phase=changes.beginPhase();
            require(world->advanceTimeChangeAlarm(registration,changes,phase,0,speed)&&
                world->advanceTimeChangeAlarm(registration,changes,phase,1,speed),"world time-change alarm dispatch");}
        require(changes.succeeded()&&changes.size()==0&&speed==60&&world->instance(first)&&
            world->instance(first)->destroyRequested&&world->instance(first)->alarms.get(1)==0,
            "final alarm value is published before marked time-change awaits cleanup");
        require(changes.retireRequested()&&!world->instance(first)&&shards.clear()&&world->activeInstanceCount()==count,
            "time-change and shard registrations retire without stale world records");
        const auto failedId=world->nextRuntimeInstanceId();const auto failedDraws=world->randomDrawCount();
        require(!world->createTimeChange(registration,changes,{},{},speed,[](unsigned){return false;})&&
            !world->effectsSucceeded()&&speed==30&&world->instance(failedId)&&
            world->instance(failedId)->alarms.get(1)==2&&world->randomDrawCount()==failedDraws,
            "failed pause preserves registered Create prefix and stops the world");
        require(!world->createTimeChange(registration,changes,{},{},speed,[](unsigned){return true;})&&
            world->nextRuntimeInstanceId()==failedId+1,"failed world creation cannot consume another ID on retry");
    }
    {
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach transition world");world->skipToGameForTesting();
        world->registerLevelExit(176);
        TimeStopEffects<16> state;
        auto shards=world->timeShardRegistration<16>();auto changes=world->timeChangeRegistration<8>();
        require(state.shards.bindPhases(shards)&&state.changes.bind(changes),"bind both transition families");
        TimeStopEffects<16>::PlayerPose pose{{0x43700000u},{0x43300000u},{},{},
            RunnerImageValue{0x43704000u}.promoted(),RunnerImageValue{0x43308000u}.promoted()};
        const auto first=world->nextRuntimeInstanceId();const auto draws=world->randomDrawCount();
        unsigned pauses=0;
        auto emit=[&](PlayerTimeStop::Effect effect){
            if(effect==PlayerTimeStop::Effect::change){
                const auto id=pauses?first+10:first+3;const auto* r=world->instance(id);
                require(r&&r->objectId==211&&r->preciseX==240.25f&&
                    r->preciseY==(pauses?1000.5f:176.5f)&&state.roomSpeed==30&&r->alarms.get(1)==2,
                    "transition pause follows private-coordinate registered Create");
                ++pauses;
            }
            return true;
        };
        auto step=[&]{return world->beginTimeStop(shards,changes,state,pose,false,
            [](auto){return true;},emit);};
        require(step()&&state.player.stopped&&state.shards.size()==6&&pauses==1&&
            world->randomDrawCount()==draws+48&&world->nextRuntimeInstanceId()==first+7,
            "entry shares actual world RNG and interleaves shard/change IDs");
        require(state.shards.sourceId(3)==first+2&&state.changes.sourceId(1)==first+3&&
            state.shards.sourceId(4)==first+4,"source first-burst/change/second-burst creation order");
        require(step()&&world->randomDrawCount()==draws+48&&pauses==1,"held stop creates nothing");
        pose.y={0x447a0000u};pose.yy=RunnerImageValue{0x447a2000u}.promoted();
        require(step()&&!state.player.stopped&&state.shards.size()==12&&pauses==2&&
            world->randomDrawCount()==draws+96&&world->nextRuntimeInstanceId()==first+14,
            "exit preserves both bursts and registered transition ordering");
        for(unsigned tick=0;tick<160;++tick){
            const auto sp=state.shards.beginPhase();const auto cp=state.changes.beginPhase();
            for(unsigned alarm=0;alarm<3;++alarm){
                state.shards.advanceAlarm(sp,alarm);
                if(alarm<2)require(world->advanceTimeChangeAlarm(changes,state.changes,cp,alarm,state.roomSpeed),
                    "registered transition expiry");
            }
        }
        require(state.shards.succeeded()&&state.shards.size()==0&&state.changes.size()==0&&
            world->instance(first)&&world->instance(first+3),"both expired families remain registered until cleanup");
        require(world->cleanupDestroyed(shards,changes,state,[](auto,const auto&){return false;}),
            "world cleanup routes both transition families without a family-wide sweep");
        for(auto id=first;id<first+14;++id)require(!world->instance(id),"cleanup removes each transition lifetime");
        require(world->randomDrawCount()==draws+96,"expiry and mixed cleanup do not consume RNG");
        require(state.reset(),"retire both transition families before bindings leave scope");
        // No field at this height: room-width side exemptions use PRIVATE x,
        // not the published x=240 kept here for scrTimeShard.
        for(int x:{159,160,320,321}){
            pose.xx=MotionBinary64::fromInteger(x);
            require(world->beginTimeStop(shards,changes,state,pose,false,[](auto){return false;},[](auto){return true;})&&
                state.player.stopped==(x==159||x==321),"strict source room-width side boundaries");
            require(state.reset(),"reset side-boundary fixture");
        }
        pose.xx=RunnerImageValue{0x43704000u}.promoted();
        pose.y={0x43300000u};pose.yy=RunnerImageValue{0x43308000u}.promoted();
        const auto failedId=world->nextRuntimeInstanceId();const auto failedDraws=world->randomDrawCount();
        auto failPause=[](auto effect){return effect!=PlayerTimeStop::Effect::change;};
        require(!world->beginTimeStop(shards,changes,state,pose,false,[](auto){return true;},failPause)&&
            !world->effectsSucceeded()&&state.shards.size()==3&&state.changes.size()==1&&
            world->instance(failedId+3)&&world->randomDrawCount()==failedDraws+24&&
            world->nextRuntimeInstanceId()==failedId+4,
            "failed transition pause retains first burst and Create but prevents second burst");
        require(!step()&&world->randomDrawCount()==failedDraws+24&&world->nextRuntimeInstanceId()==failedId+4,
            "failed transition cannot restart through a later Begin Step");
    }
    {
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach live alarm-routing world");world->skipToGameForTesting();
        TimeStopEffects<16> state;auto shards=world->timeShardRegistration<16>();auto changes=world->timeChangeRegistration<8>();
        require(state.shards.bindPhases(shards)&&state.changes.bind(changes),"bind live alarm families");
        require(world->createTimeChange(changes,state.changes,MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(176),
            state.roomSpeed,[](unsigned){return true;}),"live routed time change");
        require(world->emitTimeShards(state.burst,state.shards,{0x43700000u},{0x43300000u},{},{},false),"live routed shards");
        const auto changeId=state.changes.sourceId(1),shardId=state.shards.sourceId(1);
        const auto lifetime=world->instance(shardId)->alarms.get(0);
        for(unsigned tick=1;tick<=14;++tick){
            auto expectedAnimation=*state.shards.get(1);
            require(expectedAnimation.advanceImage(),"portable source shard animation oracle");
            unsigned collisions=0;
            require(world->stepWithTimeStopEffects(shards,changes,state,Input{},false,StartupRuntime::NoTimeTransition{},[&]{
                ++collisions;
                return state.shards.get(1)->imageIndex.bits==expectedAnimation.imageIndex.bits&&
                    world->instance(shardId)->alarms.get(0)==lifetime-int(tick);
            })&&collisions==1,"time effects and collision pass share one tick after animation and alarms");
            require(state.shards.get(1)->imageIndex.bits==expectedAnimation.imageIndex.bits&&
                state.shards.get(1)->timeline.bits==expectedAnimation.timeline.bits,
                "world animation traversal advances each registered shard exactly once");
            require(state.roomSpeed==int(tick<12?30+5*(tick/2):60)&&
                world->instance(shardId)->alarms.get(0)==lifetime-int(tick)&&
                world->instance(changeId)->destroyRequested==(tick==14),
                "live world alarm counters advance once and execute recovered time-change ramp");
        }
        require(world->cleanupDestroyed(shards,changes,state,[](auto,const auto&){return false;})&&!world->instance(changeId),
            "live routed expiry awaits normal explicit cleanup");
        state.player.stopped=true;
        const auto frozenLife=world->instance(shardId)->alarms.get(0),frozenFade=world->instance(shardId)->alarms.get(1);
        const auto cooldown=world->instance(shardId)->alarms.get(2);
        for(unsigned tick=1;tick<=5;++tick){
            require(world->stepWithTimeStopEffects(shards,changes,state,Input{})&&state.shards.get(1)&&
                world->instance(shardId)->alarms.get(0)==frozenLife&&world->instance(shardId)->alarms.get(1)==frozenFade&&
                world->instance(shardId)->alarms.get(2)==cooldown-int(tick),
                "live stopped Step compensates lifetime/fade decrements but not contact cooldown");
            std::int32_t px,py;
            require(state.shards.get(1)->xx.roundPixel(px)&&state.shards.get(1)->yy.roundPixel(py)&&
                world->instance(shardId)->x==px&&world->instance(shardId)->y==py,
                "registered stopped shard publishes rounded private motion coordinates");
        }
        require(state.reset(),"retire live alarm test owners");
        world->step(false);
        require(world->effectsSucceeded(),"ordinary world step retains no scoped alarm callback");
        auto foreignRegistration=world->timeChangeRegistration<1>();TimeChangeEffects<1> foreignChanges;int foreignSpeed=60;
        require(foreignChanges.bind(foreignRegistration)&&world->createTimeChange(foreignRegistration,foreignChanges,{},{},
            foreignSpeed,[](unsigned){return true;}),"unrouted family lifetime fixture");
        const auto foreignId=foreignChanges.sourceId(1);
        require(world->stepWithTimeStopEffects(shards,changes,state,Input{}),"unrouted lifetime has not fired yet");
        require(!world->stepWithTimeStopEffects(shards,changes,state,Input{})&&!world->effectsSucceeded()&&
            world->instance(foreignId)->alarms.get(0)==0&&world->instance(foreignId)->alarms.get(1)==1,
            "missing registered alarm owner stops phase before later counter changes");
        require(!world->stepWithTimeStopEffects(shards,changes,state,Input{})&&
            world->instance(foreignId)->alarms.get(1)==1,"failed live alarm dispatch cannot be retried");
    }
    {
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach live player-transition world");world->skipToGameForTesting();world->registerLevelExit(176);
        TimeStopEffects<16> state;auto shards=world->timeShardRegistration<16>();auto changes=world->timeChangeRegistration<8>();
        require(state.shards.bindPhases(shards)&&state.changes.bind(changes),"bind player-transition families");
        unsigned pauses=0,hooks=0;std::uint32_t first=0;
        auto transition=[&](const InstanceRecord& player,const PlayerMotion& motion,int ox,int oy){
            ++hooks;if(!first)first=world->nextRuntimeInstanceId();
            TimeStopEffects<16>::PlayerPose pose;
            // Exact projection of the current portable motion representation;
            // this does not assert that its Q8 motion matches all original FP paths.
            require(pose.x.assign(MotionBinary64::fromInteger(player.x))&&pose.y.assign(MotionBinary64::fromInteger(player.y))&&
                divideBinary64(MotionBinary64::fromInteger(motion.offsetXFixed()),MotionBinary64::fromInteger(256),pose.xx)&&
                divideBinary64(MotionBinary64::fromInteger(motion.offsetYFixed()),MotionBinary64::fromInteger(256),pose.yy)&&
                pose.xx.add(MotionBinary64::fromInteger(ox))&&pose.yy.add(MotionBinary64::fromInteger(oy))&&
                divideBinary64(MotionBinary64::fromInteger(motion.speedXFixed()),MotionBinary64::fromInteger(256),pose.xsp)&&
                divideBinary64(MotionBinary64::fromInteger(motion.speedYFifths()),MotionBinary64::fromInteger(1280),pose.ysp),
                "project current motion at actual Begin Step boundary");
            const auto draws=world->randomDrawCount();
            return world->beginTimeStop(shards,changes,state,pose,false,[](auto){return false;},[&](auto effect){
                if(effect==PlayerTimeStop::Effect::change)++pauses;
                if(effect==PlayerTimeStop::Effect::enterSound)require(world->randomDrawCount()==draws+48,
                    "both transition bursts precede later world phases");
                return true;
            });
        };
        require(world->stepWithTimeStopEffects(shards,changes,state,Input{},false,transition)&&hooks==1&&pauses==1&&
            state.player.stopped&&state.shards.size()==6&&state.changes.sourceId(1)==first+3&&
            state.shards.sourceId(4)==first+4,"normal player Begin Step creates source-ordered transition families");
        require(state.shards.get(1)->timeline.bits==0&&world->instance(first)->alarms.get(2)==29&&
            world->instance(first+3)->alarms.get(1)==1,
            "Begin Step births miss preceding animation but join same-tick alarms and Step");
        require(world->stepWithTimeStopEffects(shards,changes,state,Input{},false,transition)&&hooks==2&&pauses==1,
            "held field does not recreate the transition on the next world tick");
        require(state.reset(),"retire live transition fixture before teardown");
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
        unsigned population=999;
        require(shardOwner->instanceNumber(238,population)&&population==3,"world counts registered shard family");
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
        const auto contactDraws=world->randomDrawCount();shards.get(1)->noTouch=false;
        require(world->contactTimeShard(registration,shards,1,{},{})&&world->randomDrawCount()==contactDraws+2&&
                world->instance(first)->alarms.get(2)==5,
                "registered contact publishes cooldown before any subsequent world phase");
        require(world->stepTimeShards(registration,shards,phase,true,
            [](auto,const auto&,bool& retire){retire=false;return true;}),"registered shard motion with source queries");
        for(unsigned i=0;i<3;++i){
            const auto* r=world->instance(first+i);auto* s=shards.get(i+1);std::int32_t x,y;
            require(r&&s&&s->x.promoted().roundPixel(x)&&s->y.promoted().roundPixel(y)&&
                    r->x==x&&r->y==y&&r->imageSpeed==0&&r->alarms.get(0)==s->alarm0,
                    "stopped shard still publishes motion, zero animation speed and paused lifetime");
        }
        auto otherRegistration=world->timeShardRegistration<3>();TimeShardEffects<3> otherShards;
        require(otherShards.bindPhases(otherRegistration)&&world->emitTimeShards(burst,otherShards,
                {0x43704000u},{0x43308000u},{},{},false),"second effect pool registers in same world");
        require(shardOwner->instanceNumber(238,population)&&population==6,"population spans effect pools");
        shards.get(3)->shardLimit=5; // Local size is only three; world population is six.
        bool observedDestroy=false;
        require(world->stepTimeShards(registration,shards,shards.beginPhase(),true,
            [&](auto local,const auto& effect,bool& retire){
                retire=false;
                if(local==3){
                    const auto* record=world->instance(first+2);
                    observedDestroy=record&&record->destroyRequested&&effect.destroyRequested&&shards.size()==2;
                    require(shardOwner->instanceNumber(238,population)&&population==5,
                        "source population excludes marked shard inside its continuing Step");
                    retire=true; // Repeated source Destroy must be idempotent.
                }
                return true;
            })&&observedDestroy&&world->instance(first+2)&&world->instance(first+2)->destroyRequested&&shards.size()==2,
            "Destroy excludes immediately and continues outOfView without phase-local storage release");
        require(world->cleanupDestroyed(registration,shards,[](auto,const auto&){return false;})&&!world->instance(first+2)&&
                world->instance(first)&&world->instance(first+1),"selected cleanup releases only the marked world lifetime");
        require(otherShards.clear(),"retire second pool after population verification");
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
        require(!production->resolveInstance({})&&!productionOwner->resolve({}),
            "direct lookup rejects invalid handles");
        for(unsigned i=0;i<runtime->activeInstanceCount();++i){
            const auto& record=runtime->instanceAt(i);const auto found=lifecycle.handles.find(record.instanceId);
            require(found!=lifecycle.handles.end(),"resolve live instance key");
            require(runtime->resolveInstance(found->second)==&record,
                "direct lookup follows live records through growth and compaction");
            const auto* state=storage->owner.resolve(found->second);
            require(state&&storage->owner.objectId(found->second)==record.objectId,"owner identity matches live object");
            if(record.objectId==60||record.objectId==93){
                const auto& sprite=storage->properties[found->second.slot].sprite;
                require(sprite.spriteId==record.spriteId&&record.spriteId==terrainTileForArea(runtime->runStage().area)&&
                    record.imageSpeed==0.0F,"live terrain Create publishes area sprite and stops animation");
                if(record.objectId==93)require(record.imageIndex==23.0F&&sprite.imageIndex.bits==0x41b80000u,
                    "live platform Create publishes frame 23 to collision ownership");
            }
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
        struct WindAudio {
            bool playing=false;unsigned queries=0,plays=0,gains=0;
            bool isPlaying(int id,bool& result){++queries;result=id==900&&playing;return true;}
            bool stop(int){return false;}
            bool play(int sound,int priority,bool loop,int& voice){
                require(sound>=349&&sound<=355&&priority==90&&!loop,"live wind source play arguments");
                ++plays;playing=true;voice=900;return true;
            }
            bool getGain(int id,MotionBinary64& gain){require(id==900,"live wind gain voice");gain=MotionBinary64::fromInteger(1);return true;}
            bool setGain(int id,MotionBinary64,int duration){require(id==900&&duration==0,"live wind immediate gain");++gains;return true;}
            bool setPitch(int id,MotionBinary64 pitch){return id==900&&pitch.high==0x3ff00000u&&pitch.low==0;}
        } audio;
        TimeFieldWind wind;int channel=-1;RunnerCollisionEventContext::Frame lastWindEvent;
        auto dispatchWind=[&](InstanceHandle field){
            auto candidates=std::make_unique<RunnerCollisionPairs<capacity,16384>>();
            require(productionOwner->collectCollisionPairs(*candidates,true),"collect field/player wind candidates");
            for(unsigned i=0;i<candidates->size();++i){
                const auto pair=*candidates->at(i);
                for(unsigned reverse=0;reverse<2;++reverse){
                    const auto a=reverse?pair.other:pair.self,b=reverse?pair.self:pair.other;
                    if(a.slot!=field.slot||a.generation!=field.generation||productionOwner->resolve(b)->objectId!=35)continue;
                    RunnerCollisionEventContext context;unsigned called=0;
                    require(productionOwner->dispatchCollisionEvent(context,a,b,{true,false},[&](const auto& event){
                        ++called;lastWindEvent=event;
                        return production->dispatchTimeFieldWind(event,wind,channel,MotionBinary64::fromInteger(1),audio);
                    })&&called==1,"live field wind resolves inherited event and registered self/other");
                    return;
                }
            }
            require(false,"field/player candidate for live wind dispatch");
        };
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
        const auto beforeWind=production->randomDrawCount();dispatchWind(selected.handle);
        require(production->randomDrawCount()==beforeWind+1&&record->windSound>=349&&record->windSound<=355&&
            channel==record->windSound&&wind.voice==900&&audio.plays==1&&audio.gains==2,
            "live exit-field collision publishes per-instance sound and shared voice with one choice draw");
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
        const auto entryDraws=production->randomDrawCount();const auto queries=audio.queries;
        dispatchWind(selected.handle);
        require(production->randomDrawCount()==entryDraws&&audio.queries==queries+1&&audio.plays==1&&
            productionOwner->resolve(selected.handle)->windSound==-1,
            "inherited entrance wind respects shared playing voice without assigning windsnd or consuming RNG");
        audio.playing=false;
        require(production->dispatchTimeFieldWind(lastWindEvent,wind,channel,MotionBinary64::fromInteger(1),audio)&&
            production->randomDrawCount()==entryDraws+1&&audio.plays==2&&
            productionOwner->resolve(selected.handle)->windSound==channel,
            "expired shared voice permits inherited entrance handler to choose and store its own sound");
        ++lastWindEvent.self.generation;const auto beforeInvalid=audio.queries;
        require(!production->dispatchTimeFieldWind(lastWindEvent,wind,channel,MotionBinary64::fromInteger(1),audio)&&
            !production->effectsSucceeded()&&audio.queries==beforeInvalid&&production->randomDrawCount()==entryDraws+1,
            "stale wind event fails before audio queries or RNG");
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
