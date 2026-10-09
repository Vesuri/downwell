#include "core/StartupRuntime.h"
#include "TimeoutDamageEvents.h"
#include "BulletWallSoundChecks.h"
#include "SourceBulletSchedulingChecks.h"
#include "core/BuilderStringTraversal.h"
#include "RegisteredSkeletonThrowChecks.h"
#include "QuietSkeletonEffects.h"
#include "core/TerrainCreate.h"
#include "core/RoomPack.h"
#include "core/StoredCollisionLifecycle.h"
#include "core/StoredLayerLifecycle.h"
#include "core/StoredWorldCollision.h"
#include "core/StoredSourceBulletStep.h"
#include "core/StoredShootableTargets.h"
#include "core/StoredDitherCollisionPass.h"
#include "SkeletonSoundChecks.h"
#include "core/TimeChangeEffects.h"
#include "core/EventScheduler.h"
#include "core/RunnerObjectFamilies.h"
#include "core/SmokeDirection.h"
#include "core/SmokeTrig.h"
#include "core/GameplayContactProfileInput.h"
#include "core/TimeShardBurst.h"
#include "core/ProjectilePublication.h"
#include "core/EffectSpawnScript.h"
#include "core/ScriptSpawnOperations.h"
#include "core/MachineGunSourceStep.h"
#include "core/BoxSoundVerification.h"
#include "core/CurrencyHomingMath.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iterator>
#include <memory>
#include <unordered_map>
#include <vector>
#include <cstring>
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
    unsigned resultBirths=0;
    unsigned resultWrites=0;
    bool rejectResultBirth=false,rejectResultPosition=false;
    unsigned wipeBirths=0,wipeWrites=0,rejectWipeStage=0;
    unsigned goalWrites=0,rejectGoalStage=0;
    unsigned startWrites=0,rejectStartStage=0,startRetires=0,startMarks=0;
    InstanceHandle markedStart{};
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
        if(r.objectId==68){
            ++wipeBirths;
            require(r.roomId==16&&r.x==0&&r.y==0&&handles.count(102522),"wipe birth uses Result room and source origin");
            if(rejectWipeStage==1)return false;
        }
        if(r.objectId==325){
            ++resultBirths;
            require(r.instanceId==102522&&r.roomId==16&&r.x==80&&r.y==144&&
                !handles.count(100855)&&handles.count(100065),
                "authored Result birth follows player retirement without retiring the persistent controller");
            if(rejectResultBirth)return false;
        }
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
    bool move(InstanceHandle h,const InstanceRecord& r,RunnerImageValue x,RunnerImageValue y){
        if(r.objectId==68){
            ++wipeWrites;
            if(wipeWrites<=2)require(x.bits==0x42a00000u&&r.y==0&&
                (wipeWrites==1?(r.x==0&&y.bits==0):r.x==80),"wipe Create publishes X then Y");
            if(rejectWipeStage==wipeWrites+1)return false;
        }
        if(r.objectId==325){
            ++resultWrites;
            if(!runtime->levelClearActive())
                require(r.y==144&&y.bits==0x430e0000u,"Result Create publishes y=142 before its record write");
            else {
                std::int32_t expected;RunnerImageValue value;
                require(runtime->levelClear().motion.instanceY(expected)&&value.assignInteger(expected)&&value.bits==y.bits,
                    "Result Step publishes rounded new yy to the resource owner");
            }
            if(rejectResultPosition)return false;
        }
        return Lifecycle::move(h,r,x,y);
    }
    bool retire(InstanceHandle h,const InstanceRecord& r){
        if(r.objectId==371){
            require(r.scaleX==2.0F&&r.scaleY==2.0F,"start retirement follows both scale writes");
            if(h.slot==markedStart.slot&&h.generation==markedStart.generation)require(r.destroyRequested&&(owner.resolve(h)->flags&1u),
                "marked entrance lifetime survives until owned room cleanup");
            ++startRetires;
            if(rejectStartStage==3)return false;
        }
        if(!Lifecycle::retire(h,r))return false;
        require(handles.erase(r.instanceId)==1,"retire unknown instance ID");ids[h.slot]=0;++retires;return true;
    }
    bool markDestroyed(InstanceHandle h,const InstanceRecord& r){
        if(r.objectId==371){
            require(r.scaleX==2&&r.scaleY==2&&!r.destroyRequested&&handles.count(r.instanceId),
                "entrance destruction publishes after scales while its lifetime is live");
            ++startMarks;
            if(rejectStartStage==4)return false;
        }
        if(!Lifecycle::markDestroyed(h,r))return false;
        if(r.objectId==371){
            unsigned population=99;
            require(owner.resolve(h)&&(owner.resolve(h)->flags&1u)&&
                owner.instanceNumber(371,graph,population)&&population==0,
                "entrance mark excludes population without retiring spatial storage");
            markedStart=h;
        }
        return true;
    }
};
static bool tracked(int id){
    return id==35||id==60||id==92||id==111||id==112||id==113||id==116||id==117||id==119;
}
struct ProjectileFailureOwner:StoredWorldCollision {
    using StoredWorldCollision::StoredWorldCollision;
    unsigned reject=0,calls=0,rejected=0;
    bool allow(const InstanceRecord& r,unsigned event){
        if(r.objectId!=261)return true;
        ++calls;if(event==reject){rejected=event;return false;}return true;
    }
    bool birth(InstanceHandle h,const InstanceRecord& r){return allow(r,1)&&StoredWorldCollision::birth(h,r);}
    bool sprite(InstanceHandle h,const InstanceRecord& r,int s){return allow(r,2)&&StoredWorldCollision::sprite(h,r,s);}
    bool transform(InstanceHandle h,const InstanceRecord& r,RunnerTransformField f,RunnerImageValue v,RunnerImageValue a,std::uint32_t b){
        return allow(r,3)&&StoredWorldCollision::transform(h,r,f,v,a,b);
    }
    bool move(InstanceHandle h,const InstanceRecord& r,RunnerImageValue x,RunnerImageValue y){return allow(r,4)&&StoredWorldCollision::move(h,r,x,y);}
    bool imageIndex(InstanceHandle h,const InstanceRecord& r,RunnerImageValue v){return (!v.bits||allow(r,5))&&StoredWorldCollision::imageIndex(h,r,v);}
    bool markDestroyed(InstanceHandle h,const InstanceRecord& r){return allow(r,6)&&StoredWorldCollision::markDestroyed(h,r);}
    bool retire(InstanceHandle h,const InstanceRecord& r){return allow(r,7)&&StoredWorldCollision::retire(h,r);}
};
#include "BoxBurstWorldCheck.h"
#include "ScheduledBoxEffectsCheck.h"
#include "core/BoxWorldEffects.h"
int main(){
    const auto rb=read("generated/downwell-rooms.dwrm"),ob=read("generated/downwell-objects.dwob"),
        bb=read("generated/downwell-builder-patterns.dwbp"),sb=read("generated/downwell-sprites.dwsc"),
        mb=read("generated/downwell-sprite-masks.dwsm");
    RoomPack rooms(rb.data(),rb.size());ObjectPack objects(ob.data(),ob.size());
    BuilderPatternPack patterns(bb.data(),bb.size());SpriteCollisionPack sprites(sb.data(),sb.size());
    SpriteMaskPack masks(mb.data(),mb.size());
    require(rooms.valid()&&objects.valid()&&patterns.valid()&&sprites.valid()&&masks.valid(),"real collision resources validate");
    for(unsigned failure=0;failure<=5;++failure){
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach stored corpse contact owner");world->skipToGameForTesting();
        InstanceHandle player;RunnerImageValue x,y;
        for(unsigned i=0;i<world->activeInstanceCount();++i)if(world->instanceAt(i).objectId==35){player=world->instanceHandleAt(i);break;}
        require(player.generation&&owner->collisionPosition(player,x,y),"resolve actual corpse-contact player");
        auto corpses=world->corpseRegistration<1>();DeadBodySource source{x.promoted(),y.promoted(),{},MotionBinary64::fromInteger(-1)};
        require(world->createRegisteredCorpseFromDeath(corpses,source,206,{},102,
            [](auto,int,auto,auto,bool& hit){hit=false;return true;}),"create contact corpse");
        const auto key=corpses.sourceHandle(source.myDeadBody);bool overlaps=false;
        const auto* state=corpses.state(key);
        require(state&&owner->placeMeeting(key,*world->resolveInstance(key),35,state->x.promoted(),state->y.promoted(),overlaps)&&overlaps,
            "corpse mask actually overlaps player");
        struct Globals {
            MotionBinary64 grav{};unsigned writes=0;
            bool gravity(MotionBinary64 value){grav=value;++writes;return true;}
            MotionBinary64 gravity(){return grav;}MotionBinary64 fallSpeed(){return {};}
        } globals;
        auto particles=world->consumptionBloodRegistration<5>();
        auto createChild=[&](MotionBinary64 px,MotionBinary64 py,std::uint32_t& child){
            return world->createRegisteredConsumptionBlood(particles,px,py,globals,child);
        };
        const auto beforeParticles=world->randomState();
        struct Effects {
            decltype(createChild)& createChild;
            unsigned failure=0,eaten=0,sounds=0,children=0,reads=0;
            bool incrementBodiesEaten(){++eaten;return true;}
            void eatSound(int s,int p,int l,int g){require(eaten==1&&s==83&&p==80&&!l&&g==1,"source consumption sound order");++sounds;}
            MotionBinary64 playerY(){require(reads++==children*2,"read player Y first");return MotionBinary64::fromInteger(192+children);}
            MotionBinary64 playerX(){require(reads++==children*2+1,"read player X second");return MotionBinary64::fromInteger(240+children);}
            bool create(int object,MotionBinary64 px,MotionBinary64 py,std::uint32_t& child){
                require(object==223&&eaten==1&&sounds==1&&px.high==MotionBinary64::fromInteger(240+children).high&&
                    py.high==MotionBinary64::fromInteger(192+children).high,"each synchronous child sees fresh player fields");
                ++children;return children!=failure&&createChild(px,py,child);
            }
        } effects{createChild,failure};
        RunnerCollisionEventContext context;
        auto contact=[&](bool upgrade,bool dead){return owner->dispatchCollisionEvent(context,key,player,{true,false},[&](const auto& event){
            return world->dispatchCorpsePlayerCollision(corpses,event,upgrade,dead,effects);
        });};
        require(contact(true,false)&&!effects.eaten,"inactive corpse cannot be consumed");
        require(corpses.dispatchPublished(key,[](auto& state,auto&,bool&){state.active=true;return true;}),"activate contact fixture");
        require(contact(false,false)&&contact(true,true)&&!effects.eaten,"upgrade and death guards");
        require(contact(true,false)==!failure&&world->effectsSucceeded()==!failure&&effects.eaten==1&&effects.sounds==1&&
            effects.children==(failure?failure:5)&&effects.reads==2*effects.children,"stored consumption preserves ordered success/failure prefix");
        require(world->resolveInstance(key)&&world->resolveInstance(key)->destroyRequested==!failure,
            "only complete consumption marks corpse; lifetime remains until cleanup");
        if(failure)require(!contact(true,false)&&effects.children==failure,"failed contact cannot replay children");
        const unsigned born=failure?failure-1:5;require(globals.writes==born,"every real child publishes global gravity");
        unsigned found=0;auto oracleRandom=beforeParticles;
        for(unsigned i=0;i<world->activeInstanceCount();++i){
            const auto& r=world->instanceAt(i);if(r.objectId!=223)continue;
            const auto* particle=particles.state(r);require(particle&&r.alarms.get(0)==120&&r.spriteId==92,"child completes Create before returning");
            ConsumptionBloodEffect oracle;Globals expectedGlobals;
            struct Birth {
                Globals& globals;
                bool gravity(MotionBinary64 v){return globals.gravity(v);}
                bool imageSpeed(RunnerImageValue){return true;}bool imageIndex(RunnerImageValue){return true;}
            } publish{expectedGlobals};
            require(oracle.create(MotionBinary64::fromInteger(240+found),MotionBinary64::fromInteger(192+found),oracleRandom,publish)&&
                particle->imageIndex.bits==oracle.imageIndex.bits&&particle->xsp.high==oracle.xsp.high&&
                particle->xsp.low==oracle.xsp.low&&particle->ysp.high==oracle.ysp.high&&particle->ysp.low==oracle.ysp.low,
                "real child state matches sequential source Create and RNG");
            if(!failure)require(particles.advanceAnimation(r)&&particles.step(r,globals)&&!r.destroyRequested,
                "registered particle publishes animation and three movement writes");
            ++found;
        }
        require(found==born&&world->randomDrawCount()==oracleRandom.drawCount(),"child count and consumed random words match prefix");
        for(unsigned i=0;i<16;++i)require(world->randomState().stateWord(i)==oracleRandom.stateWord(i),"full child RNG state");
        require(corpses.clear(),"clean contact fixture");
        if(!failure)for(unsigned tick=0;tick<120;++tick){
            require(world->withConsumptionBloodEvents(particles,globals,[&]{world->step(false);return world->effectsSucceeded();}),
                "real room dispatches consumption particle animation, Step, alarm and cleanup");
            unsigned live=0;for(unsigned i=0;i<world->activeInstanceCount();++i)live+=world->instanceAt(i).objectId==223;
            require(live==(tick==119?0u:5u),"alarm 120 retires children in the room cleanup phase");
        }
        require(particles.clear(),"clean real consumption children");
    }
    for(int x:{159,160,240,320,321})for(bool reject:{false,true})for(bool field:{false,true}){
        struct CorpseQueryOwner:StoredWorldCollision {
            using StoredWorldCollision::StoredWorldCollision;
            unsigned fields=0,motion=0;bool reject=false;
            bool placeMeeting(InstanceHandle key,const InstanceRecord& r,int object,
                MotionBinary64 x,MotionBinary64 y,bool& hit){
                if(r.objectId==263){
                    require(r.maskId==599,"corpse queries retain the source collision mask");
                    if(object==82){++fields;if(reject)return false;}
                    else if(object==56)++motion;
                    else require(object==57,"only corpse Create queries parentWall");
                }
                return StoredWorldCollision::placeMeeting(key,r,object,x,y,hit);
            }
        };
        auto owner=std::make_unique<CorpseQueryOwner>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
            InstanceLifecycleBinding::bindPositionedBirths(*owner));
        require(owner->attach(*world),"attach corpse to actual collision world");world->skipToGameForTesting();
        if(field)world->registerLevelExit(-1032);
        auto corpses=world->corpseRegistration<1>();DeadBodySource source;
        source.x=MotionBinary64::fromInteger(x);source.y=MotionBinary64::fromInteger(-1000);
        source.imageXscale=MotionBinary64::fromInteger(-1);
        require(world->createRegisteredCorpseFromDeath(corpses,source,206,{},102,
            [](auto,int layer,auto,auto,bool& hit){require(layer==100,"corpse tile layer");hit=false;return true;}),
            "create real registered corpse with mirrored transform");
        const auto key=corpses.sourceHandle(source.myDeadBody);const auto before=*corpses.state(key);
        struct Environment {
            int area(){return 1;}int restInPieces(){return 0;}
            bool create(int,MotionBinary64,MotionBinary64){return false;}
        } environment;
        const bool outside=x<160||x>320;
        bool inField=false;
        const auto* record=world->resolveInstance(key);
        require(record&&owner->StoredWorldCollision::placeMeeting(key,*record,82,source.x,source.y,inField),
            "inspect actual inherited exit-field overlap");
        if(x==240)require(inField==field,"central corpse overlaps the generated exit field");
        const bool allowed=outside||inField;
        owner->reject=reject;
        const bool ok=corpses.dispatchPublished(key,[&](auto& state,auto& publish,bool& good){
            return state.stepInWorld(true,480,publish,environment,good);
        });
        require(ok==!reject&&world->effectsSucceeded()==!reject&&owner->fields==1&&
            owner->motion==unsigned(!reject&&allowed)*2,
            "corpse tests the actual parent field before strict side exemption and solid motion");
        const auto* state=corpses.state(key);require(state,"corpse lifetime remains owned after Step");
        if(reject||!allowed)require(state->xx.high==before.xx.high&&state->xx.low==before.xx.low&&
            state->yy.high==before.yy.high&&state->yy.low==before.yy.low,
            "blocked or rejected time query does not advance corpse accumulators");
        if(!reject&&!allowed)require(state->alarms.get(0)==181&&state->alarms.get(3)==8,
            "frozen corpse refunds expiry but not activation");
        require(corpses.clear(),"retire registered corpse fixture");
    }
    {
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach ordinary dither pass");world->skipToGameForTesting();
        auto pass=std::make_unique<StoredDitherCollisionPass>(*owner);
        unsigned retired=0;
        for(unsigned tick=0;tick<180;++tick){
            std::vector<InstanceHandle> marked;
            require(world->stepWithCollisionPass({},false,[&]{
                if(!pass->run(*world))return false;
                for(unsigned i=0;i<world->activeInstanceCount();++i){
                    const auto& record=world->instanceAt(i);
                    if(record.objectId==60&&record.destroyRequested){
                        const auto key=world->instanceHandleAt(i);
                        require(owner->resolve(key),"collision retains marked wall until frame cleanup");
                        marked.push_back(key);
                    }
                }
                return true;
            }),"ordinary dither collision pass survives natural room progression");
            for(auto key:marked){++retired;require(!owner->resolve(key),"ordinary frame cleanup retires contacted wall");}
            require(world->instance(100854)&&owner->valid(),"dither survives its wall contacts");
        }
        require(retired>0,"ordinary pass actually prunes authored walls");
        world->configureDitherAboveEnvironment(false,true);
        require(world->stepWithCollisionPass({},false,[&]{return pass->run(*world);})&&
            !world->instance(100854),"ending retires authored controller at ordinary cleanup");
        require(world->restartFromResults(DeathMenuSequence::Action::Retry,2),"Retry recreates authored controllers");
        world->configureDitherAboveEnvironment(false,false);
        for(unsigned tick=0;tick<10;++tick)
            require(world->stepWithCollisionPass({},false,[&]{return pass->run(*world);}),
                "reused collision pass resolves new room generations after Retry");
    }
    {
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach dither collision world");world->skipToGameForTesting();
        InstanceHandle dither{},walls[2]{};unsigned count=0;
        for(unsigned i=0;i<world->activeInstanceCount();++i){
            const auto& r=world->instanceAt(i);
            if(r.objectId==61)dither=world->instanceHandleAt(i);
            if(r.objectId==60&&count<2)walls[count++]=world->instanceHandleAt(i);
        }
        require(count==2&&owner->resolve(dither),"authored room supplies dither and walls");
        const auto draws=world->randomDrawCount();RunnerCollisionEventContext context;
        RunnerCollisionEventContext::Frame saved{};
        for(unsigned i=0;i<2;++i){
            require(owner->dispatchCollisionEvent(context,dither,walls[i],{true,false},[&](const auto& event){
                saved=event;return world->dispatchDitherAboveCollision(event);
            }),"resource-selected dither event marks other");
            std::uint32_t flags=0;
            require(owner->resolve(walls[i])&&owner->resolve(walls[i])->destroyRequested&&
                owner->collisionFlags(walls[i],flags)&&(flags&1u)&&
                !owner->resolve(dither)->destroyRequested&&!context.current(),
                "wall mark retains lifetime and leaves self alive");
            if(!i)require(!owner->resolve(walls[1])->destroyRequested,"other wall untouched before its contact");
            require(world->dispatchDitherAboveCollision(saved),"repeated destruction mark is idempotent");
        }
        require(world->randomDrawCount()==draws,"dither wall events consume no RNG");
        for(auto h:walls)require(world->retireMarkedWall(h)&&!owner->resolve(h),"wall retires only at explicit cleanup");
        require(!world->dispatchDitherAboveCollision(saved)&&!world->effectsSucceeded()&&
            owner->resolve(dither)&&!owner->resolve(dither)->destroyRequested,"stale other fails without destroying self");
    }
    {
        struct RejectWallMark:StoredWorldCollision {
            using StoredWorldCollision::StoredWorldCollision;
            unsigned calls=0;
            bool markDestroyed(InstanceHandle h,const InstanceRecord& r){
                if(r.objectId==60){++calls;return false;}
                return StoredWorldCollision::markDestroyed(h,r);
            }
        };
        auto owner=std::make_unique<RejectWallMark>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
            InstanceLifecycleBinding::bindPointQueries(*owner));
        require(owner->attach(*world),"attach rejecting dither collision world");world->skipToGameForTesting();
        InstanceHandle dither{},wall{};
        for(unsigned i=0;i<world->activeInstanceCount();++i){
            if(world->instanceAt(i).objectId==61)dither=world->instanceHandleAt(i);
            if(world->instanceAt(i).objectId==60)wall=world->instanceHandleAt(i);
        }
        RunnerCollisionEventContext::Frame event{dither,wall,4,60,61,991};
        require(!world->dispatchDitherAboveCollision(event)&&owner->calls==1&&
            !owner->resolve(wall)->destroyRequested&&!owner->resolve(dither)->destroyRequested,
            "failed owner mark leaves both records intact");
        require(!world->dispatchDitherAboveCollision(event)&&owner->calls==1,
            "failed dither contact latches failure rather than retrying mutations");
    }
    {
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach scheduled reward lifecycle");world->skipToGameForTesting();
        InstanceHandle box;
        for(unsigned attempt=0;attempt<1024;++attempt){
            require(world->spawnBuilderPrimitive(92,240,512,box),"create scheduled reward lottery box");
            if(owner->resolve(box)->box.haveGem)break;
            require(world->markBoxDestroyed(box),"mark unsuccessful lottery box");
        }
        require(owner->resolve(box)->box.haveGem==2&&world->publishBoxHealth(box,{}),"arm source reward box destruction");
        auto gems=world->currencyRegistration<2>();ScheduledBoxCounts counts;
        BoxSoundVerification::Audio<StartupRuntime> audio{*world,owner->resolve(box)->box.breakSound};
        auto readBox=[&](InstanceHandle h,BoxDestructionScript::Caller& out){return owner->readBoxDestruction(h,out);};
        auto operations=[&](InstanceHandle h){
            auto controls=scheduledBoxEffects(*world,h,counts,[&](InstanceHandle key){return world->markBoxDestroyed(key);});
            controls.timeQuery=[&](InstanceHandle key,bool& allowed){return owner->boxTimeAllowed(*world,key,false,480,allowed);};
            auto rewards=world->boxCurrencyOperations(gems,h,controls,
                [&](InstanceHandle key,MotionBinary64& x,MotionBinary64& y){
                    BoxDestructionScript::Caller out;if(!readBox(key,out))return false;x=out.x;y=out.y;return true;
                },[](auto,int layer,auto,auto,bool& hit){hit=false;return layer==100;});
            return boxWorldEffects(*world,h,rewards,[&](int id,unsigned& frames){
                PackedSpriteCollision art;if(!sprites.sprite(id,art))return false;frames=art.frameCount;return true;
            },audio,MotionBinary64{0x3fd00000u,0},false);
        };
        unsigned steps=0,draws=0;std::uint32_t childId=0;InstanceHandle gem;
        bool birthTick=true,retainedCaller=false;
        auto tick=[&]{return world->withCurrencyEvents(gems,
            [&](auto key,auto&,auto phase,unsigned,auto& random,bool&){
                require(!birthTick&&phase==StartupRuntime::CurrencyPhase::step,"Step birth waits until next ordinary Step phase");
                ++steps;
                struct Frozen {
                    bool freeMotion(CurrencyCreate&){return false;}
                    bool attractedMotion(CurrencyCreate&){return false;}
                } frozen;
                // Explicit source time-stop fixture, not a substitute for the
                // unfinished live player's pTimeStop publication.
                const auto before=random.drawCount();
                const bool ok=gems.step(key,random,true,480,frozen);
                require(ok&&random.drawCount()==before+1,"first reward Step consumes its one-time sound choice before freezing");
                return ok;
            },[&]{return world->withBoxDestruction(readBox,operations,[&]{
                return world->stepWithOwnedDebris({},[](auto,const auto&,bool& frozen){frozen=false;return true;},false,[&]{
                    if(birthTick){
                        const auto* r=world->resolveInstance(box);
                        require(r&&r->destroyRequested&&r->boxReward.moniAmount.zero(),"caller survives through post-Step collision boundary");
                        childId=r->boxReward.myGem;const auto* child=world->instance(childId);
                        require(child&&child->objectId==248,"scheduled reward owns source child ID");gem=gems.handle(*child);
                        retainedCaller=true;
                    }
                    return true;
                });
            });});};
        const bool firstTick=tick();
        if(!firstTick||!retainedCaller||steps||world->resolveInstance(box))
            std::fprintf(stderr,"reward tick ok=%u effects=%u retained=%u steps=%u box=%u marks=%u stop=%u sound=%u valid=%u child=%u\n",
                firstTick,world->effectsSucceeded(),retainedCaller,steps,world->resolveInstance(box)!=nullptr,
                counts.mark,counts.stop,audio.calls,audio.valid,childId);
        require(firstTick&&retainedCaller&&!steps&&!world->resolveInstance(box),"scheduled reward survives caller cleanup without a same-phase Step");
        const auto* state=gems.find(gem);const auto* record=world->instance(childId);
        require(state&&record&&!state->allSet&&record->imageIndex==0&&record->alarms.get(0)==10&&record->alarms.get(3)==15,
            "newborn reward does not retroactively animate or count down alarms");
        require(world->visitRegisteredCurrencyDraw(gems,[&](auto key,const auto& r,const auto& s,int depth){
            require(key.generation==gem.generation&&r.instanceId==childId&&r.spriteId==134&&r.maskId==134&&s.getAmount==10&&depth==10000,
                "newborn medium reward reaches its registered source Draw layer");++draws;return true;
        })&&draws==1,"reward Draw survives box retirement");
        RunnerImageValue expected{};bool wrapped=false;
        require(SequenceTimeline::advance(expected,state->imageSpeed,4,wrapped),"next-frame reward animation oracle");expected.bits&=0x7fffffffu;
        birthTick=false;require(tick()&&steps==1,"reward enters following global animation/alarm/Step phases");
        state=gems.find(gem);record=world->instance(childId);std::uint32_t image;std::memcpy(&image,&record->imageIndex,4);
        require(state->allSet&&state->obtainable&&record->imageSpeed==0&&image==expected.bits&&
            record->alarms.get(0)==10&&record->alarms.get(3)==15,"source frozen Step runs after animation/countdowns and refunds its clocks");
        require(gems.clear(),"retire scheduled reward registry");
        std::puts("Scheduled box reward: birth-phase cutoff, retained caller, Draw, cleanup and next-frame frozen Step passed");
    }
    for(unsigned failure=0;failure<3;++failure){
        const bool rejectCreate=failure==1,rejectPosition=failure==2;
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach box-owned reward fields");world->skipToGameForTesting();
        InstanceHandle box;
        // Keep the real Create lottery; do not force haveGem in the fixture.
        for(unsigned attempt=0;attempt<1024;++attempt){
            require(world->spawnBuilderPrimitive(92,240,512,box),"source box Create for reward fixture");
            if(owner->resolve(box)->box.haveGem)break;
            require(world->markBoxDestroyed(box),"exclude preceding lottery boxes from contacts");
        }
        require(owner->resolve(box)->box.haveGem==2&&world->publishBoxHealth(box,{}),"source-created medium reward box");
        require(world->publishCollisionPosition(box,RunnerImageValue{0x43704000u},RunnerImageValue{0x44002000u}),
            "publish fractional runner position independently of private motion caches");
        require(world->resolveInstance(box)->preciseX==240.0F&&world->resolveInstance(box)->preciseY==512.0F,
            "reward regression deliberately leaves private coordinates stale");
        auto gems=world->currencyRegistration<2>();ScheduledBoxCounts counts;
        auto controls=scheduledBoxEffects(*world,box,counts,[&](InstanceHandle h){return world->markBoxDestroyed(h);});
        controls.timeQuery=[&](InstanceHandle h,bool& allowed){return owner->boxTimeAllowed(*world,h,false,480,allowed);};
        InstanceHandle created;
        auto rewards=world->boxCurrencyOperations(gems,box,controls,
            [&](InstanceHandle h,MotionBinary64& x,MotionBinary64& y){
                BoxDestructionScript::Caller read;
                if(rejectPosition||!owner->readBoxDestruction(h,read))return false;
                x=read.x;y=read.y;return true;
            },
            [&](InstanceHandle h,int layer,MotionBinary64 x,MotionBinary64 y,bool& hit){
                require(x.high==0x406e0800u&&!x.low&&y.high==0x40800400u&&!y.low,
                    "reward uses caller published position, not private motion caches or rounded drawing coordinates");
                created=h;hit=false;return layer==100&&!rejectCreate;
            });
        BoxSoundVerification::Audio<StartupRuntime> audio{*world,owner->resolve(box)->box.breakSound};
        auto ops=boxWorldEffects(*world,box,rewards,[&](int id,unsigned& frames){
            PackedSpriteCollision art;if(!sprites.sprite(id,art))return false;frames=art.frameCount;return true;
        },audio,MotionBinary64{0x3fd00000u,0},false);
        bool good=true;
        const bool completed=BoxDestructionScript::runRetained([&](auto& out){return owner->readBoxDestruction(box,out);},ops,good);
        require(completed==(failure==0)&&good==completed&&world->effectsSucceeded()==completed,
            "box event reaches real registered rewards after ordered effects and destruction");
        const auto* caller=world->resolveInstance(box);
        require(caller&&caller->destroyRequested&&caller->boxReward.moniAmount.zero()==(failure==0)&&!caller->boxReward.atrState,
            "real caller retains reward fields after destruction marking");
        if(rejectPosition){
            require(!created.generation&&!caller->boxReward.myGem&&!counts.stop&&world->debrisEffects().size()==2,
                "missing authoritative position cannot silently create a reward from cached coordinates");
            require(gems.clear(),"clear empty rejected-position registry");continue;
        }
        if(rejectCreate){
            require(!caller->boxReward.myGem&&caller->boxReward.moniAmount.high==MotionBinary64::fromInteger(10).high&&
                !caller->boxReward.moniAmount.low&&gems.find(created)&&gems.find(created)->getAmount==2&&
                gems.find(created)->size==0&&!counts.stop&&world->debrisEffects().size()==2,
                "failed reward Create retains prior effects and caller amount without myGem/setters/tail alarmStop");
            require(gems.clear(),"retire failed reward child");continue;
        }
        const auto* child=world->instance(caller->boxReward.myGem);
        require(child&&child->objectId==248,"box myGem stores globally allocated child ID");
        const auto* gem=gems.find(gems.handle(*child));
        require(gem&&gem->getAmount==10&&gem->size==1&&!gem->attracted,"source medium reward setters reach registry");
        require(world->debrisEffects().size()==2&&world->smokeEffects().size()==1&&world->scriptEffects().size()==1&&
            counts.mark==1&&counts.stop==1&&audio.valid&&audio.calls==7,"reward preserves completed destruction effects and tail alarmStop");
        require(gems.clear(),"retire box reward registry before teardown");
    }
    {
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach mixed smoke/wall collision pass");world->skipToGameForTesting();
        auto pass=std::make_unique<StoredDitherCollisionPass>(*owner);
        require(pass->run(*world),"settle authored dither contacts before smoke fixture");
        const InstanceRecord* wall=nullptr;
        for(unsigned i=0;i<world->activeInstanceCount();++i){
            const auto& r=world->instanceAt(i);if(r.objectId==60&&!r.destroyRequested){wall=&r;break;}
        }
        require(wall,"retained wall for smoke geometry");
        auto smoke=world->smokeRegistration<1>();std::uint32_t id=0;
        require(world->createRegisteredSmoke(smoke,MotionBinary64::fromInteger(wall->x),MotionBinary64::fromInteger(wall->y),id),
            "place smoke on actual retained wall");
        const auto* r=world->instance(id);require(r&&smoke.alarmEvent(*r,0),"enable smoke wall response");
        InstanceHandle self{};
        for(unsigned i=0;i<world->activeInstanceCount();++i)
            if(world->instanceAt(i).instanceId==id)self=world->instanceHandleAt(i);
        auto expected=*smoke.state(*r);unsigned contacts=0;
        auto particles=world->consumptionBloodRegistration<1>();std::uint32_t particleId=0;
        struct Globals {
            MotionBinary64 grav{};
            bool gravity(MotionBinary64 v){grav=v;return true;}
            MotionBinary64 gravity(){return grav;}MotionBinary64 fallSpeed(){return {};}
        } globals;
        require(world->createRegisteredConsumptionBlood(particles,MotionBinary64::fromInteger(wall->x+4),
            MotionBinary64::fromInteger(wall->y+4),globals,particleId),"place particle inside actual retained wall");
        for(unsigned i=0;i<world->activeInstanceCount();++i){
            const auto& other=world->instanceAt(i);if(other.objectId!=60||other.destroyRequested)continue;
            bool hit=false;require(owner->collisionOverlap(self,world->instanceHandleAt(i),hit),"resource smoke/wall overlap");
            if(hit){++contacts;require(expected.collideWall(),"one source event per overlapping wall");}
        }
        require(contacts&&pass->run(*world,smoke,particles),"shared ordered pair stream admits smoke and particles beside dither");
        const auto* particle=world->instance(particleId);
        require(particle&&particle->destroyRequested&&particles.state(*particle)->destroyRequested,
            "actual ordered geometric contact marks particle without immediate retirement");
        const auto* state=smoke.state(*r);
        require(state&&state->speed.high==expected.speed.high&&state->speed.low==expected.speed.low,
            "ordered collection executes all smoke contacts once");
        require(smoke.clear(),"retire mixed smoke fixture");
        require(particles.clear(),"retire mixed particle fixture");
    }
    {
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach limbo shard collision world");world->skipToGameForTesting();
        auto shards=world->limboShardRegistration<2>();InstanceHandle first,second;
        const auto before=world->randomDrawCount();
        require(world->createRegisteredLimboShard(shards,MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(10000),false,first),
            "register source-ID limbo shard before synchronous Create");
        const auto id=world->resolveInstance(first)->instanceId;
        require(world->randomDrawCount()==before+7&&shards.state(first)&&
            world->resolveInstance(first)->spriteId==91&&world->resolveInstance(first)->imageSpeed!=1.f,
            "limbo Create publishes image fields and consumes seven words");
        require(world->createRegisteredLimboShard(shards,MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(10000),true,second)&&
            world->resolveInstance(second)->instanceId==id+1&&shards.state(second)->shardLimit==5,
            "limbo births share the room numeric-ID stream");
        RunnerImageValue x,y;
        require(owner->collisionPosition(first,x,y)&&x.bits==shards.state(first)->x.bits&&y.bits==shards.state(first)->y.bits,
            "fractional-capable engine position is registered");
        require(shards.advanceAnimation(first),"registered sprite-91 sequence advances through lifecycle owner");
        struct Views {
            int top=9000;unsigned reads=0;
            std::vector<unsigned> selected;
            bool prepare(const InstanceRecord& r,bool& stopped,int& width,LimboShard::Player& player){
                selected.push_back(r.instanceId);stopped=true;width=480;player={};return true;
            }
            bool viewTop(MotionBinary64& value){++reads;value=MotionBinary64::fromInteger(top);return true;}
            bool outside(RunnerImageValue,RunnerImageValue,bool&){require(false,"population cap not reached");return false;}
        } views;
        const auto draws=world->randomDrawCount();
        require(world->stepRegisteredLimboShard(shards,first,true,480,{},views)&&
            world->resolveInstance(first)->imageSpeed==0.f&&views.reads==1&&world->randomDrawCount()==draws,
            "real parent-field query freezes rate but preserves inertial Step and final view read");
        require(owner->collisionPosition(first,x,y)&&x.bits==shards.state(first)->x.bits&&y.bits==shards.state(first)->y.bits,
            "both rounded position setters reach collision owner");
        require(world->withLimboShardEvents(shards,views,[&]{world->step(false);return world->effectsSucceeded();})&&
            views.selected.size()==2&&views.reads==3,
            "room scheduler selects both registered limbo Steps and their animation");
        require(shards.activate(first,false),"deactivate limbo shard");
        require(world->stepRegisteredLimboShard(shards,first,false,480,{},views)&&views.reads==3,
            "inactive limbo shard does not run Step");
        require(shards.activate(first,true),"reactivate limbo shard");views.top=11000;
        require(world->stepRegisteredLimboShard(shards,first,false,480,{},views)&&
            world->resolveInstance(first)->destroyRequested&&shards.state(first)->destroyRequested,
            "view retirement marks both states before cleanup");
        require(shards.retire(first)&&!world->resolveInstance(first)&&!shards.state(first)&&shards.state(second),
            "ordered cleanup retires memberships and preserves sibling identity after compaction");
        InstanceHandle reused;
        require(world->createRegisteredLimboShard(shards,MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(10000),false,reused)&&
            reused.generation!=first.generation&&!shards.state(first)&&world->resolveInstance(reused)->instanceId==id+2,
            "slot reuse cannot revive a stale handle or reuse source IDs");
        require(world->withLimboShardEvents(shards,views,[&]{world->step(false);return world->effectsSucceeded();})&&
            !world->resolveInstance(reused)&&!world->resolveInstance(second)&&!shards.state(reused)&&!shards.state(second),
            "selected Steps mark limbo lifetimes and actual room cleanup retires both");
        require(shards.clear(),"limbo owner clears before room teardown");
    }
    {
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach wall-to-limbo birth composition");world->skipToGameForTesting();
        auto shards=world->limboShardRegistration<1>();InstanceHandle child;
        auto create=[&](int object,MotionBinary64 x,MotionBinary64 y){
            return object==379&&world->createRegisteredLimboShard(shards,x,y,false,child);
        };
        struct Effects {
            decltype(create)& spawn;
            bool create(int object,MotionBinary64 x,MotionBinary64 y){return spawn(object,x,y);}
            bool outOfView(InstanceHandle){return false;}
        } effects{create};
        unsigned attempts=0;
        for(unsigned i=0;i<world->activeInstanceCount()&&!child.generation;++i){
            const auto& wall=world->instanceAt(i);if(wall.objectId!=60||wall.wall.pillar)continue;
            const auto h=world->instanceHandleAt(i);RunnerImageValue x,y;
            require(x.assignInteger(1000)&&y.assignInteger(10000+int(attempts)*64)&&
                world->publishCollisionPosition(h,x,y),"isolate wall through collision setters");
            const auto before=world->randomDrawCount();
            require(owner->wallEndStepEvent(*world,h,4,effects),"wall's area-four branch executes registered child Create");
            require(world->randomDrawCount()==before+1+(child.generation?7:0),
                "wall choice and child Create consume the same shared stream synchronously");
            ++attempts;
            if(child.generation){
                const auto* r=world->resolveInstance(child);
                require(r&&r->x==1000&&r->y==10000+int(attempts-1)*64-16&&shards.state(child),
                    "wall child is registered at exact source offset before End Step returns");
            }
        }
        require(child.generation&&shards.clear(),"source wall path produced and retired a registered limbo child");
    }
    for(unsigned reject=0;reject<5;++reject){
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach limbo bullet contact world");world->skipToGameForTesting();
        InstanceHandle shot;
        for(unsigned tick=0;tick<1000&&!shot.generation;++tick){
            world->step(gameplayContactProfileInput(tick,world->mainPlayerX(),world->mainPlayerY()),false);
            if(world->mainBulletCount())shot=world->mainBulletAt(0).instance;
        }
        require(world->effectsSucceeded()&&shot.generation,"obtain a live fired bullet identity");
        // Explicit source-state admission. This does not resume the legacy
        // trajectory or claim that natural source bullet scheduling is wired.
        MachineGunSourceMotion motion;GameMakerRandom spread(2);
        require(motion.reset(MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(512))&&motion.initialize(spread),
            "initialize exact VM velocity for the selected bullet");
        auto shards=world->limboShardRegistration<1>();InstanceHandle shard;
        require(world->createRegisteredLimboShard(shards,motion.x.promoted(),motion.y.promoted(),false,shard),"create contact shard");
        unsigned reads=0;
        auto velocity=[&](InstanceHandle selected,MotionBinary64& x,MotionBinary64& y){
            ++reads;require(selected.slot==shot.slot&&selected.generation==shot.generation,"resolve selected other lifetime");
            if(reject==1||!motion.initialized)return false;
            x=motion.xsp;y=reject==2?MotionBinary64{0x7ff00000u,0}:motion.ysp;return true;
        };
        const auto draws=world->randomDrawCount();auto expectedRandom=world->randomState();
        MotionBinary64 expectedX;
        require(randomBinary64(expectedRandom.nextU32(),MotionBinary64::fromInteger(2),expectedX),"expected contact RNG");
        if(motion.xsp.zero())expectedX={};else expectedX.high|=motion.xsp.high&0x80000000u;
        RunnerCollisionEventContext context;
        auto dispatch=[&]{return owner->dispatchCollisionEvent(context,shard,shot,{true,false},[&](const auto& event){
            require(event.owner==379&&event.code==2771&&event.subtype==world->resolveInstance(shot)->objectId,
                "parentPBullet event inherits with actual other subtype");
            auto selected=event;
            if(reject==3)selected.owner=89;
            if(reject==4)++selected.other.generation;
            return world->dispatchLimboShardBulletCollision(shards,selected,velocity);
        });};
        if(reject){
            require(!dispatch()&&!world->effectsSucceeded()&&!context.current()&&world->randomDrawCount()==draws&&
                !shards.state(shard)->noTouch&&reads==(reject<3?1u:0u),"reject unavailable velocity, malformed binding and stale other before mutation");
            continue;
        }
        require(shards.activate(shard,false)&&dispatch()&&reads==0&&world->randomDrawCount()==draws,
            "inactive shard does not evaluate bullet fields");
        require(shards.activate(shard,true)&&dispatch()&&reads==1&&world->randomDrawCount()==draws+1,"one contact consumes one shared RNG word");
        const auto* state=shards.state(shard);
        require(state&&state->xsp.high==expectedX.high&&state->xsp.low==expectedX.low&&
            state->ysp.high==motion.ysp.high&&state->ysp.low==motion.ysp.low&&state->noTouch&&state->alarm2==5&&
            world->resolveInstance(shard)->alarms.get(2)==5,"contact preserves fractional velocity and publishes alarm 2");
        motion.initialized=false;
        require(dispatch()&&reads==1&&world->randomDrawCount()==draws+1&&!context.current(),
            "noTouch skips even an unavailable bullet velocity owner");
        require(shards.clear(),"retire contact shard");
    }
    for(unsigned reject=0;reject<3;++reject){
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach consumption collision world");world->skipToGameForTesting();
        auto particles=world->consumptionBloodRegistration<1>();std::uint32_t id=0;
        struct Globals {
            MotionBinary64 grav{};
            bool gravity(MotionBinary64 v){grav=v;return true;}
            MotionBinary64 gravity(){return grav;}MotionBinary64 fallSpeed(){return {};}
        } globals;
        require(world->createRegisteredConsumptionBlood(particles,{}, {},globals,id),"register collision particle");
        InstanceHandle self{},wall{};
        require(world->visitDrawChecked([&](InstanceHandle key,const InstanceRecord& record){
            if(record.instanceId==id)self=key;
            if(record.objectId==60&&!wall.generation)wall=key;
            return true;
        })&&self.generation&&wall.generation,"select registered particle and wall lifetimes");
        const auto draws=world->randomDrawCount();RunnerCollisionEventContext context;
        const bool ok=owner->dispatchCollisionEvent(context,self,wall,{true,false},[&](auto event){
            if(reject==1)event.code=2007;
            if(reject==2)++event.other.generation;
            return world->dispatchConsumptionBloodCollision(particles,event);
        });
        require(ok==(reject==0)&&world->effectsSucceeded()==ok&&world->randomDrawCount()==draws,
            "consumption contact validates event/lifetime and consumes no RNG");
        const auto* record=world->instance(id);
        require(record&&record->destroyRequested==ok&&particles.state(*record)->destroyRequested==ok,
            "kill marks both owners but preserves lifetime until cleanup");
        if(ok){
            require(world->withConsumptionBloodEvents(particles,globals,[&]{world->step({});return world->effectsSucceeded();})&&
                !world->instance(id),"room cleanup retires contact-killed particle");
        }
        require(particles.clear(),"teardown collision particle");
    }
    for(unsigned mode=0;mode<4;++mode){
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach retained currency reward world");world->skipToGameForTesting();
        InstanceHandle box;
        require(world->spawnBuilderPrimitive(92,240,512,box)&&world->markBoxDestroyed(box),"retain destroyed reward caller");
        auto gems=world->currencyRegistration<2>();
        CurrencySpawnSource<std::uint32_t> fields;fields.myGem=77;
        unsigned writes=0,births=0;InstanceHandle first;
        const auto before=world->randomState().drawCount();
        auto readCaller=[&](auto& out){
            const auto* r=world->resolveInstance(box);if(!r||r->objectId!=92)return false;
            require(r->destroyRequested,"reward runs after destruction mark, before retirement");
            out=fields;out.objectIndex=r->objectId;
            out.x=MotionBinary64::fromInteger(r->x);out.y=MotionBinary64::fromInteger(r->y);return true;
        };
        const bool ok=world->spawnRegisteredCurrency(gems,readCaller,
            [&](MotionBinary64 n){fields.moniAmount=n;return true;},
            [&](bool n){fields.atrState=n;return true;},
            [&](std::uint32_t id){++writes;if(mode==3)return false;fields.myGem=id;return true;},
            MotionBinary64::fromInteger(mode==1?5:mode==0?10:3),
            [&](InstanceHandle h,int layer,MotionBinary64,MotionBinary64,bool& hit){
                require(layer==100&&owner->resolve(h)&&owner->resolve(h)->objectId==248,"registered reward enters real collision owner during Create");
                if(!first.generation)first=h;++births;hit=false;return mode!=2;
            });
        require(ok==(mode==0)&&world->effectsSucceeded()==ok,"retained reward success/capacity/Create/caller-write boundaries");
        const auto* gem=gems.find(first);require(gem,"failed reward retains its already-created child");
        require(gem->getAmount==(mode==0?10:2)&&gem->size==(mode==0?1:0),"reward setters follow Create and caller publication");
        require(world->randomState().drawCount()==before+(mode==1?16:8),"reward Create shares exact world RNG stream");
        require(writes==(mode==2?0:mode==1?2:1)&&births==(mode==1?2:1),"no replayed or skipped child publication");
        if(mode==0)require(fields.moniAmount.zero()&&fields.myGem==owner->resolve(first)->instanceId,"reward publishes global child ID, not family handle");
        if(mode>=2)require(fields.myGem==77,"failed Create/write preserves previous caller child ID");
        require(gems.clear(),"release registered reward children before world teardown");
    }
    for(unsigned mode=0;mode<5;++mode){
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach currency Create world");world->skipToGameForTesting();
        auto currency=world->currencyRegistration<1>();
        const MotionBinary64 x{0x406e0800u,0},y{mode==4?0xc0800400u:0x40800400u,0}; // 240.25, +/-512.5
        unsigned queries=0;InstanceHandle key,observed;
        auto tiles=[&](InstanceHandle h,int layer,MotionBinary64,MotionBinary64,bool& hit){
            const auto* r=owner->resolve(h);observed=h;++queries;
            require(r&&r->objectId==248&&layer==100&&!key.generation,"currency belongs to collision owner before synchronous Create returns");
            hit=mode==3&&queries==1;return mode!=2;
        };
        auto oracle=world->randomState();CurrencyCreate expected;
        struct Queries {
            unsigned mode,calls=0;
            bool wall(MotionBinary64,MotionBinary64,bool& hit){hit=mode==4;return true;}
            bool tile(MotionBinary64,MotionBinary64,bool& hit){hit=mode==3&&++calls==1;return mode!=2;}
            bool publish(RunnerImageValue,RunnerImageValue){return true;}
        } expectedQueries{mode};
        require(expected.create(x,y,1,false,oracle,expectedQueries)==(mode!=2),"currency Create oracle result");
        require(world->createRegisteredCurrency(currency,x,y,tiles,key)==(mode!=2),"registered currency Create result");
        if(mode==4)observed=key;
        require(world->randomState().drawCount()==oracle.drawCount()&&world->randomState().stateIndex()==oracle.stateIndex(),"registered currency draw order");
        for(unsigned i=0;i<16;++i)require(world->randomState().stateWord(i)==oracle.stateWord(i),"registered currency full WELL state");
        const auto* state=currency.find(observed);const auto* record=owner->resolve(observed);
        require((queries||mode==4)&&state&&record&&state->x.bits==expected.x.bits&&state->y.bits==expected.y.bits&&
            record->alarms.get(0)==10&&record->alarms.get(3)==15,"registered currency retains synchronous Create state");
        std::uint32_t publishedX,publishedY;
        std::memcpy(&publishedX,&record->preciseX,4);std::memcpy(&publishedY,&record->preciseY,4);
        require(publishedX==expected.x.bits&&publishedY==expected.y.bits,"currency relocation publishes final builtin coordinates");
        if(mode==0){
            const auto currencyId=record->instanceId;InstanceHandle next;
            require(world->spawnBuilderPrimitive(92,400,512,next)&&owner->resolve(next)->instanceId==currencyId+1,
                "currency birth participates in global numeric ID stream");
        }
        if(mode!=2){
            require(key.generation&&currency.amount(key,10)&&currency.size(key,1)&&currency.attracted(key,true)&&
                state->getAmount==10&&state->size==1&&state->attracted,"source reward setters reach registered currency");
            if(mode==1){
                const auto before=world->randomDrawCount();InstanceHandle rejected;
                require(!world->createRegisteredCurrency(currency,x,y,tiles,rejected)&&!rejected.generation&&
                    world->randomDrawCount()==before,"currency capacity failure precedes Create RNG");
            }
        }else require(!key.generation&&!world->effectsSucceeded(),"failed currency Create does not publish its return value");
        require(currency.clear()&&!owner->resolve(observed),"retire registered currency including failed Create");
    }
    for(unsigned mode=0;mode<3;++mode){
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach currency scheduler world");world->skipToGameForTesting();
        auto currency=world->currencyRegistration<1>();InstanceHandle gem;
        require(world->createRegisteredCurrency(currency,MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(512),
            [](auto,int,auto,auto,bool& hit){hit=false;return true;},gem),"create scheduled currency");
        require(currency.dispatch(gem,[&](auto& state,bool&){state.obtime=2;state.distime=2;state.attracted=mode==1;return true;}),
            "configure shortened source alarm durations");
        unsigned steps=0,expiry=0;bool retained=false;
        struct Audio {
            bool fail;unsigned calls=0;
            bool isPlaying(int id,bool& yes){++calls;yes=false;return id==166&&!fail;}
            bool stop(int){return false;}
            bool play(int id,int priority,bool loop,int& voice){++calls;voice=700;return id==166&&priority==30&&!loop;}
            bool getGain(int id,MotionBinary64& gain){++calls;gain=MotionBinary64::fromInteger(1);return id==700;}
            bool setGain(int id,MotionBinary64 gain,int duration){++calls;return id==700&&gain.high==0x3ff00000u&&!gain.low&&!duration;}
            bool setPitch(int id,MotionBinary64 pitch){++calls;return id==700&&pitch.high==0x3ff00000u&&!pitch.low;}
        } audio{mode==2};
        RunnerImageValue image{};const auto speed=currency.find(gem)->imageSpeed;
        for(unsigned age=1;age<=16;++age){
            bool wrapped=false;require(SequenceTimeline::advance(image,speed,4,wrapped),"currency animation oracle");image.bits&=0x7fffffffu;
            const bool ok=world->withCurrencyEvents(currency,
                [&](auto key,auto& state,auto phase,unsigned alarm,auto&,bool&){
                    if(phase==StartupRuntime::CurrencyPhase::step){
                        ++steps;require(state.obtainable==(age>=10)&&state.disappearing==(age>=11)&&
                            state.suckable==(age>=15),"currency flags follow ascending world alarm pass");
                        return true; // Motion deliberately supplied by the fixture, not claimed implemented.
                    }
                    require(phase==StartupRuntime::CurrencyPhase::alarm&&alarm==2&&age==12,"expiry alarm reaches required event adapter");
                    ++expiry;return currency.expire(key,MotionBinary64::fromInteger(1),false,audio);
                },[&]{return world->stepWithCollisionPass({},false,[&]{
                    const auto* r=owner->resolve(gem);if(mode==0&&age==12)retained=r&&r->destroyRequested;
                    return true;
                });});
            require(ok==(mode!=2||age!=12),"currency event failure propagates to world");
            if(const auto* r=owner->resolve(gem)){
                std::uint32_t bits;std::memcpy(&bits,&r->imageIndex,4);
                require(bits==image.bits,"registered currency advances once in global animation phase");
            }
            if(age==12&&mode!=1)break;
        }
        require(expiry==1&&steps==(mode==1?16u:11u),"destroyed or failed currency skips ordinary Step");
        require(audio.calls==(mode==0?5u:mode==1?0u:1u),"currency expiry sound precedes destruction and is suppressed for attracted gems");
        if(mode==0)require(retained&&!owner->resolve(gem)&&!currency.find(gem),"currency destruction retains caller until cleanup then retires both owners");
        else require(owner->resolve(gem)&&currency.find(gem),"attracted or failed currency keeps its lifetime");
        require(currency.clear(),"clear borrowed currency scheduler owner");
    }
    for(unsigned mode=0;mode<3;++mode){
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach currency time-stop world");world->skipToGameForTesting();
        auto currency=world->currencyRegistration<1>();InstanceHandle gem;
        require(world->createRegisteredCurrency(currency,MotionBinary64{0x406e0800u,0},MotionBinary64{0x40800400u,0},
            [](auto,int,auto,auto,bool& hit){hit=false;return true;},gem),"create frozen currency");
        require(currency.dispatch(gem,[&](auto& state,bool&){state.disappearing=true;state.attracted=mode==2;return true;}),"configure currency freeze fixture");
        auto oracle=world->randomState();CurrencyStepSound sound;sound.begin(0,oracle);
        struct Motion {
            unsigned free=0,attracted=0;
            bool freeMotion(CurrencyCreate&){++free;return false;}
            bool attractedMotion(CurrencyCreate&){++attracted;return false;}
        } motion;
        for(unsigned tick=0;tick<(mode?1u:3u);++tick){
            require(world->withCurrencyEvents(currency,
                [&](auto key,auto&,auto phase,unsigned,auto& random,bool&){
                    require(phase==StartupRuntime::CurrencyPhase::step,"frozen currency has no secondary alarm event");
                    return currency.step(key,random,mode!=1,480,motion);
                },[&]{return world->stepWithCollisionPass({},false,StartupRuntime::NoCollisionPass{});})==(mode==0),
                "frozen currency executes source envelope; missing motion fails explicitly");
        }
        const auto* state=currency.find(gem);const auto* r=owner->resolve(gem);
        require(state&&r&&state->allSet&&state->bounceSound==sound.bounceSound&&motion.free==(mode==1)&&motion.attracted==(mode==2),
            "currency first-Step sound precedes freeze and correct movement branch");
        require(world->randomState().drawCount()==oracle.drawCount()&&world->randomState().stateIndex()==oracle.stateIndex(),"currency frozen Step consumes sound RNG only once");
        for(unsigned i=0;i<16;++i)require(world->randomState().stateWord(i)==oracle.stateWord(i),"currency frozen Step full WELL state");
        if(mode==0)require(state->obtainable&&state->imageSpeed.bits==0&&r->imageSpeed==0&&state->scriptIndex==4&&
            state->alarms.get(0)==10&&state->alarms.get(3)==15&&state->flash==1&&r->preciseX==240&&r->preciseY==512&&
            state->xx.high==0x406e0800u&&state->yy.high==0x40800400u,"frozen currency refunds alarms, flashes and rounds only published coordinates");
        require(currency.clear(),"clear frozen currency owner");
    }
    {
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach attracted currency world");world->skipToGameForTesting();
        auto currency=world->currencyRegistration<1>();InstanceHandle gem;
        require(world->createRegisteredCurrency(currency,MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(512),
            [](auto,int,auto,auto,bool& hit){hit=false;return true;},gem)&&currency.attracted(gem,true),"create attracted registered currency");
        auto expected=*currency.find(gem);CurrencyHomingMath math{portableAtan};bool good=true;unsigned freeCalls=0;
        for(unsigned tick=0;tick<12;++tick){
            require(CurrencyHomingSource::run(expected,MotionBinary64::fromInteger(250),MotionBinary64::fromInteger(500),math,good),"homing integration oracle");
            require(world->withCurrencyEvents(currency,[&](auto key,auto&,auto phase,unsigned,auto& random,bool&){
                if(phase!=StartupRuntime::CurrencyPhase::step)return false;
                return currency.stepWithHoming(key,random,true,480,MotionBinary64::fromInteger(250),MotionBinary64::fromInteger(500),
                    [&](auto&){++freeCalls;return false;},math);
            },[&]{return world->stepWithCollisionPass({},false,StartupRuntime::NoCollisionPass{});}),"registered homing continues while time is stopped");
            const auto* state=currency.find(gem);const auto* r=owner->resolve(gem);std::int32_t x,y;
            require(expected.xx.roundPixel(x)&&expected.yy.roundPixel(y)&&state&&r&&!freeCalls&&
                state->xx.high==expected.xx.high&&state->xx.low==expected.xx.low&&state->yy.high==expected.yy.high&&state->yy.low==expected.yy.low&&
                r->preciseX==float(x)&&r->preciseY==float(y),"source homing state and separately published rounded coordinates match");
        }
        require(currency.clear(),"clear attracted currency owner");
    }
    {
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach free currency collision world");world->skipToGameForTesting();
        InstanceHandle box;require(world->spawnBuilderPrimitive(92,240,544,box),"create currency bounce floor");
        auto currency=world->currencyRegistration<1>();InstanceHandle gem;
        require(world->createRegisteredCurrency(currency,MotionBinary64::fromInteger(248),MotionBinary64::fromInteger(512),
            [](auto,int,auto,auto,bool& hit){hit=false;return true;},gem),"create freely moving currency");
        require(currency.dispatch(gem,[](auto& s,bool&){s.xsp={};s.ysp=MotionBinary64::fromInteger(2);return true;}),"set source bounce fixture velocity");
        struct Audio {
            unsigned sounds=0;
            bool isPlaying(int,bool& playing){playing=false;return true;}
            bool stop(int){return true;}
            bool play(int id,int priority,bool loop,int& voice){if(id<168||id>170||priority!=30||loop)return false;voice=700+int(++sounds);return true;}
            bool getGain(int,MotionBinary64& gain){gain=MotionBinary64::fromInteger(1);return true;}
            bool setGain(int,MotionBinary64,int){return true;}
            bool setPitch(int,MotionBinary64){return true;}
        } audio;
        CurrencyHomingMath math{portableAtan};bool bounced=false;
        for(unsigned tick=0;tick<32;++tick){
            require(world->withCurrencyEvents(currency,[&](auto key,auto&,auto phase,unsigned,auto& random,bool&){
                if(phase!=StartupRuntime::CurrencyPhase::step)return false;
                return currency.stepSource(key,random,false,480,MotionBinary64::fromInteger(999),MotionBinary64::fromInteger(999),
                    math,[](auto& top){top={};return true;},audio,MotionBinary64::fromInteger(1),false);
            },[&]{return world->stepWithCollisionPass({},false,StartupRuntime::NoCollisionPass{});}),"source currency free motion through registered terrain queries");
            const auto* state=currency.find(gem);const auto* r=owner->resolve(gem);std::int32_t x,y;
            require(state&&r&&state->xx.roundPixel(x)&&state->yy.roundPixel(y)&&r->preciseX==float(x)&&r->preciseY==float(y),"free currency publishes rounded position");
            if(state->yCollision){
                require(state->ysp.zero()||(state->ysp.high==0xbff00000u&&!state->ysp.low),"source fractional integer bounce is zero or minus one");bounced=true;
            }
        }
        require(bounced&&audio.sounds&&!currency.find(gem)->attracted,"registered currency bounced off real box with source audio");
        require(currency.clear(),"clear free currency owner");
    }
    {
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach currency collection world");world->skipToGameForTesting();
        InstanceHandle player;RunnerImageValue x,y;
        for(unsigned i=0;i<world->activeInstanceCount();++i)if(world->instanceAt(i).objectId==35){
            player=world->instanceHandleAt(i);break;
        }
        require(player.generation,"registered collection player exists");
        require(owner->collisionPosition(player,x,y),"read published collection player position");
        auto currency=world->currencyRegistration<1>();InstanceHandle gem;
        require(world->createRegisteredCurrency(currency,x.promoted(),y.promoted(),[](auto,int,auto,auto,bool& hit){hit=false;return true;},gem),"create colliding registered currency");
        require(currency.dispatch(gem,[](auto& s,bool&){s.obtainable=true;return true;}),"admit currency collision fixture");
        struct Controls {
            using Handle=InstanceHandle;unsigned& controllers;unsigned& sounds;
            bool achievement(int){return false;}bool timerStart(std::uint32_t& timer){timer=300;return true;}
            bool lessResist(){return false;}bool resetControllers(){++controllers;return true;}
            bool gemPop(){return false;}bool gemPowered(){return false;}
            bool playerPosition(MotionBinary64&,MotionBinary64&){return false;}
            bool effect(MotionBinary64,MotionBinary64,int,MotionBinary64,int,int){return false;}
            bool create(int,MotionBinary64,MotionBinary64,Handle&){return false;}bool myBullet(Handle){return false;}
            bool bulletDirection(MotionBinary64){return false;}bool bulletSprite(int){return false;}
            bool bulletDamage(int){return false;}bool bulletSpeed(int){return false;}bool sound(int,int,bool,bool){return false;}
            bool soundOverlap(int id,int priority,bool loop,bool cut,const char* channel){
                require(id==167&&priority==60&&!loop&&cut&&std::strcmp(channel,"UI")==0,"source collection UI sound");++sounds;return false;
            }
            bool fxNoLoop(int,MotionBinary64){return false;}bool recharge(){return false;}bool destroy(){return false;}
        };
        unsigned controllers=0,sounds=0;auto ops=world->currencyGainOperations(Controls{controllers,sounds});
        const auto before=world->mainCurrencyTotal();const auto draws=world->randomDrawCount();bool hit=false;
        require(owner->placeMeeting(gem,*owner->resolve(gem),35,currency.find(gem)->x.promoted(),currency.find(gem)->y.promoted(),hit),"query currency-player overlap");
        require(hit,"currency actually overlaps the source player mask");
        RunnerCollisionEventContext context;
        require(!owner->dispatchCollisionEvent(context,gem,player,{true,false},[&](const auto& frame){
            require(frame.owner==248&&frame.self.slot==gem.slot&&frame.other.slot==player.slot,"source currency-player event lookup");
            return world->collectRegisteredCurrency(currency,gem,ops);
        }),"currency collision propagates source sound failure");
        require(!world->effectsSucceeded()&&controllers==1&&sounds==1&&world->mainCurrencyTotal()==before+2&&
            world->mainGameGemTotal()==2&&world->mainGemStreak()==2&&world->mainGemStreakTimer()==300&&world->randomDrawCount()==draws&&
            currency.find(gem)->gemAmount==2&&!owner->resolve(gem)->destroyRequested,"collection failure retains preceding account writes and live caller");
        require(currency.clear(),"clear failed collection caller");
    }
    {
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach currency Draw world");world->skipToGameForTesting();
        auto currency=world->currencyRegistration<1>();InstanceHandle gem;
        require(world->createRegisteredCurrency(currency,MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(512),
            [](auto,int,auto,auto,bool& hit){hit=false;return true;},gem),"create Draw caller");
        unsigned draws=0;unsigned hits[2]{};
        for(int size=0;size<=1;++size){
            require(currency.size(gem,size),"publish reward size");
            for(int flash:{-1,0,1}){
                require(currency.dispatch(gem,[&](auto& s,bool&){s.flash=flash;return true;}),"set source flash gate");
                const auto before=draws;
                require(world->visitRegisteredCurrencyDraw(currency,[&](auto key,const auto& record,const auto& state,int depth){
                    ++draws;require(depth==10000&&key.slot==gem.slot&&record.spriteId==133+size&&record.maskId==133+size&&state.flash<=0,
                        "source sprite and mask writes precede drawFx");return true;
                })&&draws==before+(flash<=0?1:0),"negative dflash draws and positive suppresses");
            }
            // Draw's mask write must affect subsequent identity-bearing queries.
            for(int y=500;y<=524;++y)for(int x=228;x<=252;++x){
                InstancePlaceResult result;
                require(owner->collisionPoint({},248,MotionBinary64::fromInteger(x),MotionBinary64::fromInteger(y),true,false,result),
                    "query Draw-published currency mask");
                if(result.id!=-4){require(result.handle.slot==gem.slot,"query selects currency lifetime");++hits[size];}
            }
        }
        require(hits[0]&&hits[1]>hits[0],"large gem Draw publishes larger precise collision coverage");
        require(currency.dispatch(gem,[](auto& s,bool&){s.flash=-1;return true;}),"enable failing compositor fixture");
        require(!currency.draw(gem,[](auto,const auto&,const auto&){return false;})&&!world->effectsSucceeded(),
            "drawFx failure is terminal after retained appearance writes");
        require(currency.clear(),"clear failed Draw caller");
    }
    for(unsigned failure=0;failure<=7;++failure){
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach currency sound world");world->skipToGameForTesting();
        auto currency=world->currencyRegistration<1>();InstanceHandle gem;
        require(world->createRegisteredCurrency(currency,MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(512),
            [](auto,int,auto,auto,bool& hit){hit=false;return true;},gem),"create sound caller");
        struct Audio {
            StartupRuntime& world;unsigned failure,calls=0;
            bool call(int code){const int expected[]={167,1167,2167,3000,4000,5000,900};
                require(calls<7&&expected[calls]==code,"pickup audio operation order");return ++calls!=failure;}
            bool isPlaying(int id,bool& playing){playing=true;return call(id);}
            bool stop(int id){return call(1000+id);}
            bool play(int id,int priority,bool loop,int& voice){
                require(world.overlapSound(8)==167&&priority==60&&!loop,"UI publication precedes pickup playback");
                voice=900;return call(2000+id);}
            bool getGain(int id,MotionBinary64& gain){require(id==900,"pickup gain voice");gain={0x3fe00000u,0};return call(3000);}
            bool setGain(int id,MotionBinary64 gain,int duration){
                require(id==900&&gain.high==0x3fc00000u&&gain.low==0&&!duration,"pickup source volume product");return call(4000);}
            bool setPitch(int id,MotionBinary64 pitch){
                require(id==900&&pitch.high==0x3ff00000u&&pitch.low==0,"UI pitch remains one");return call(5000);}
        } audio{*world,failure};
        const auto draws=world->randomDrawCount();
        const auto before=world->overlapSound(8);
        require(world->playCurrencyPickupSound(currency,gem,{0x3fd00000u,0},audio)==(failure==0),"pickup audio failure result");
        const auto& sound=owner->resolve(gem)->sound;
        require(audio.calls==(failure?failure:7)&&world->effectsSucceeded()==(failure==0)&&world->randomDrawCount()==draws&&
            sound.sfxIndex==167&&sound.soundPriority==60&&!sound.cutPrv&&sound.chanNum==8&&
            world->overlapSound(8)==((!failure||failure>=3)?167:before),"pickup retains source sound and channel failure prefix");
        if(failure)require(!world->playCurrencyPickupSound(currency,gem,{0x3fd00000u,0},audio)&&audio.calls==failure,
            "failed pickup audio cannot retry later operations");
        require(currency.clear(),"retire pickup sound caller");
    }
    for(bool accept:{true,false}){
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach currency pickup effect world");world->skipToGameForTesting();
        auto currency=world->currencyRegistration<1>();InstanceHandle gem;
        require(world->createRegisteredCurrency(currency,{0x406e0800u,0},{0x40800400u,0},
            [](auto,int,auto,auto,bool& hit){hit=false;return true;},gem),"create pickup caller");
        const auto x=currency.find(gem)->x.bits,y=currency.find(gem)->y.bits;
        const auto id=world->nextRuntimeInstanceId();const auto draws=world->randomDrawCount();unsigned lookups=0;
        auto frames=[&](int sprite,unsigned& count){
            ++lookups;const auto* child=world->instance(id);
            require(currency.find(gem)->myFx==id&&child&&child->objectId==219&&child->spriteId==-1,
                "pickup caller assignment precedes sprite validation");
            require(sprite==105,"pickup uses source non-loop sprite");
            return accept&&sprites.frameCount(sprite,count);
        };
        require(world->spawnCurrencyPickupEffect(currency,gem,frames)==accept,"pickup effect admission result");
        const auto* effect=world->scriptEffects().atNewest(0);
        require(effect&&effect->x.bits==x&&effect->y.bits==y&&effect->angle.bits==0&&effect->depth.bits==0&&
            currency.find(gem)->myFx==id&&world->nextRuntimeInstanceId()==id+1&&world->randomDrawCount()==draws&&lookups==1,
            "pickup retains child coordinates, source ID, defaults and unchanged RNG");
        if(accept){
            require(effect->spriteId==105&&effect->imageSpeed.bits==0x3f4ccccdu,"pickup publishes source sprite and speed");
            require(currency.clear(),"retire pickup caller independently of child");
            for(unsigned tick=0;tick<64&&world->instance(id);++tick)world->step({},false);
            require(world->effectsSucceeded()&&!world->instance(id),"pickup animation end retires child through world scheduler");
        }else{
            require(!world->effectsSucceeded()&&world->instance(id)->spriteId==-1&&effect->imageSpeed.bits==0x3f800000u,
                "failed sprite lookup retains birth and caller assignment without later speed setter");
            require(!world->spawnCurrencyPickupEffect(currency,gem,frames)&&lookups==1,"terminal pickup failure cannot duplicate child");
            require(currency.clear(),"clear failed pickup caller");
        }
    }
    for(unsigned chainCase=0;chainCase<10;++chainCase){
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach scheduled chain world");world->skipToGameForTesting();
        auto bullets=world->droneBulletRegistration<4>();
        InstanceHandle box,neighbors[4];
        require(world->spawnBuilderPrimitive(92,240,512,box)&&world->publishBoxHealth(box,{}),"create chain source box");
        const int xs[]={240,256,240,224},ys[]={496,512,528,512};
        const unsigned neighborCount=chainCase==2?3:4;
        for(unsigned i=0;i<neighborCount;++i)require(world->spawnBuilderPrimitive(92,xs[i],ys[i],neighbors[i]),"create chain neighbor");
        if(chainCase==1)for(unsigned i=0;i<4;++i){
            InstanceHandle child;require(bullets.create(MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(600),child),"fill chain bullet capacity");
        }
        require(!owner->resolve(box)->box.haveGem,"chain fixture has no reward");
        ScheduledBoxCounts counts;unsigned steps=0;bool retained=false;
        BoxSoundVerification::Audio<StartupRuntime> audio{*world,owner->resolve(box)->box.breakSound};
        audio.chain=true;audio.water=chainCase==3;
        if(chainCase>=4)audio.fail=chainCase+4;
        const bool completed=world->withBoxDestruction(
            [&](InstanceHandle h,BoxDestructionScript::Caller& caller){return owner->readBoxDestruction(h,caller);},
            [&](InstanceHandle h){
                auto rest=scheduledBoxEffects(*world,h,counts,[&](InstanceHandle key){return world->markBoxDestroyed(key);});
                rest.verifyDebris=true;rest.sawSound=h.slot==box.slot&&h.generation==box.generation;rest.before=world->randomState();
                rest.timeQuery=[&](InstanceHandle key,bool& allowed){return owner->boxTimeAllowed(*world,key,false,480,allowed);};
                auto chain=world->boxChainPublication(bullets,*owner,h);
                return boxWorldEffects(*world,h,ScheduledBoxChain<decltype(chain),decltype(rest)>(chain,rest),
                    [&](int id,unsigned& frames){PackedSpriteCollision art;if(!sprites.sprite(id,art))return false;
                        frames=art.frameCount;return true;},audio,MotionBinary64{0x3fd00000u,0},audio.water);
            },[&]{return world->withDroneBulletEvents(bullets,
                [&](auto&,auto&,auto,auto&,bool&){++steps;return false;},
                [&]{return world->stepWithOwnedDebris({},[](auto,const auto&,bool& frozen){frozen=false;return true;},false,[&]{
                    const auto* caller=owner->resolve(box);
                    if(!caller||!caller->destroyRequested)return false;
                    const auto* bullet=world->instance(caller->boxMyBullet);
                    const int lastTarget=neighborCount==4?int(owner->resolve(neighbors[3])->instanceId):-4;
                    if(!bullet||bullet->objectId!=266||caller->boxTarget!=lastTarget)return false;
                    const auto local=bullets.handle(*bullet);const auto* shot=bullets.instances().find(local);
                    if(!shot||shot->state.motion.direction.high!=MotionBinary64::fromInteger(90).high||
                        shot->state.damage.high!=MotionBinary64::fromInteger(20).high||shot->state.motion.direction.low||
                        shot->state.damage.low||shot->state.motion.initialized||shot->state.alarm0!=16||shot->spriteId!=459)return false;
                    for(unsigned i=0;i<neighborCount;++i)if(!owner->resolve(neighbors[i])||owner->resolve(neighbors[i])->alarms.get(0)!=5)return false;
                    retained=true;return true;
                });});});
        require(audio.valid&&audio.calls==(audio.fail?audio.fail:chainCase==1?7u:13u),"scheduled chain runs source plain sound after overlap sound");
        require(completed==(chainCase!=1&&!audio.fail),"scheduled chain respects projectile and sound failures");
        if(audio.fail){
            const auto* caller=owner->resolve(box);
            require(!world->effectsSucceeded()&&caller&&!caller->destroyRequested&&caller->boxTarget==-4&&
                caller->boxMyBullet>=0&&caller->sound.sfxIndex==342&&caller->sound.cutPrv&&caller->sound.chanNum==7&&
                world->overlapSound(7)==audio.sound&&!steps&&!counts.mark&&!world->debrisEffects().size(),
                "chain audio failure retains caller fields without activating neighbors or destroying box");
            const auto* shot=world->instance(caller->boxMyBullet);
            require(shot&&shot->objectId==266&&bullets.instances().size()==1,"chain audio failure retains already-created projectile");
            for(unsigned i=0;i<neighborCount;++i)require(owner->resolve(neighbors[i])->alarms.get(0)==-1,"chain audio failure leaves neighbors unarmed");
            require(bullets.clear(),"clear retained projectile after chain sound failure");continue;
        }
        if(chainCase==1){
            const auto* caller=owner->resolve(box);
            require(!world->effectsSucceeded()&&caller&&!caller->destroyRequested&&caller->boxMyBullet==-4&&
                caller->boxTarget==-4&&!steps&&!counts.mark&&bullets.instances().size()==4&&
                world->smokeEffects().size()==1&&world->scriptEffects().size()==1&&!world->debrisEffects().size(),
                "failed drone Create preserves earlier effects without neighbor writes or destruction");
            for(unsigned i=0;i<neighborCount;++i)require(owner->resolve(neighbors[i])->alarms.get(0)==-1,"rejected chain leaves neighbors unarmed");
            require(bullets.clear(),"clear retained failed chain owner");continue;
        }
        require(retained&&!steps&&counts.mark==1&&!owner->resolve(box)&&
            bullets.instances().size()==1&&world->debrisEffects().size()==2,"chain children and phase cutoff survive source cleanup");
        auto retired=world->boxChainPublication(bullets,*owner,box);
        require(!retired.targetAlarm(neighbors[0],0,99)&&owner->resolve(neighbors[0])->alarms.get(0)==5,
            "retired chain caller cannot mutate a surviving neighbor");
        require(bullets.clear(),"clear borrowed chain projectile owner");
    }
    for(unsigned mode=0;mode<12;++mode){
        const bool fullEffects=mode!=0;
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach scheduled box world");world->skipToGameForTesting();
        InstanceHandle box;
        require(world->spawnBuilderPrimitive(92,240,512,box)&&world->publishBoxHealth(box,MotionBinary64::fromInteger(mode>=10?1:0)),"create scheduled box");
        if(mode>=10)require(world->publishBoxAlarm0(box,2),"arm scheduled healthy box");
        ScheduledBoxCounts counts;
        const auto destroyed=world->mainBlocksDestroyed();
        auto readCaller=[&](InstanceHandle key,BoxDestructionScript::Caller& caller){return owner->readBoxDestruction(key,caller);};
        auto controls=[&](InstanceHandle key){return scheduledBoxEffects(*world,key,counts,
            [&](InstanceHandle h){return world->markBoxDestroyed(h);});};
        if(fullEffects){
            require(!owner->resolve(box)->box.haveGem,"scheduled no-reward box fixture");
            const unsigned soundFailure=mode>=3?mode-2:0;
            BoxSoundVerification::Audio<StartupRuntime> audio{*world,owner->resolve(box)->box.breakSound,soundFailure};
            GameMakerRandom soundRandom{0};
            bool boxAllowed=false;unsigned queries=0;
            if(mode==2)for(unsigned i=0;i<debrisEffectCapacity;++i){
                InstanceHandle child;
                require(world->spawnDebris(MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(512),1,child)&&
                    owner->resolve(child)&&owner->resolve(child)->objectId==370,"fill registered debris capacity with real handles");
            }
            const bool completed=world->withBoxDestruction(readCaller,[&](InstanceHandle key){
                auto callbacks=controls(key);callbacks.verifyDebris=true;
                callbacks.timeQuery=[&](InstanceHandle h,bool& allowed){
                    const bool selected=h.slot==box.slot&&h.generation==box.generation;
                    if(selected){++queries;if(mode==11)return false;}
                    const bool ok=owner->boxTimeAllowed(*world,h,mode==10,480,allowed);
                    if(selected&&ok)boxAllowed=allowed;return ok;
                };
                callbacks.sawSound=mode<10&&key.slot==box.slot&&key.generation==box.generation;callbacks.before=world->randomState();
                if(callbacks.sawSound)soundRandom=world->randomState();
                return boxWorldEffects(*world,key,callbacks,[&](int id,unsigned& frames){
                    PackedSpriteCollision art;if(!sprites.sprite(id,art))return false;frames=art.frameCount;return true;},
                    audio,MotionBinary64{0x3fd00000u,0},false);
            },[&]{return world->stepWithOwnedDebris({},[](auto,const auto&,bool& frozen){frozen=false;return true;});});
            require(completed==(mode==1||mode==10),"stored scheduled source effects honor query and child failures");
            if(mode>=10){
                if(mode==10)require(!boxAllowed,"healthy scheduled box is outside time fields and refunds its alarm");
                const auto* retained=owner->resolve(box);
                require(queries==1&&retained&&!retained->destroyRequested&&!counts.mark&&audio.calls==0&&
                    !world->debrisEffects().size(),"healthy source box executes only alarmStop");
                require(retained->alarms.get(0)==(mode==10&&!boxAllowed?2:1)&&
                    retained->box.scriptIndex==(mode==10?1:0),"scheduled alarm refund and failed-query prefix");
                continue;
            }
            if(soundFailure){
                require(!world->effectsSucceeded()&&audio.valid&&audio.calls==soundFailure&&
                    owner->resolve(box)&&!owner->resolve(box)->destroyRequested&&!counts.mark&&
                    !world->smokeEffects().size()&&!world->flashEffects().slot(0).handle&&!world->scriptEffects().size()&&
                    !world->debrisEffects().size(),"audio failure stops before all source particle emissions");
                require(soundRandom.drawCount()==world->randomState().drawCount()&&
                    soundRandom.stateIndex()==world->randomState().stateIndex(),"failed box sound consumes no RNG");
                for(unsigned i=0;i<16;++i)require(soundRandom.stateWord(i)==world->randomState().stateWord(i),"failed sound retains WELL words");
                continue;
            }
            if(mode==2){
                require(!world->effectsSucceeded()&&world->debrisEffects().size()==debrisEffectCapacity&&
                    owner->resolve(box)&&owner->resolve(box)->destroyRequested&&audio.valid&&audio.calls==7&&counts.mark==1&&
                    world->smokeEffects().size()==1&&world->scriptEffects().size()==1,
                    "real debris exhaustion preserves completed destruction prefix");
                continue;
            }
            require(world->effectsSucceeded()&&!owner->resolve(box)&&world->debrisEffects().size()==2&&
                world->smokeEffects().size()==1&&world->flashEffects().slot(0).handle&&world->scriptEffects().size()==1&&
                counts.mark==1&&audio.valid&&audio.calls==7&&counts.stop&&world->mainBlocksDestroyed()==destroyed+1,
                "stored complete effect sequence retires only the box at Draw cleanup");
            require(world->stepWithOwnedDebris({},[](auto,const auto&,bool& frozen){frozen=false;return true;})&&
                world->debrisEffects().size()==2,"registered destruction debris survives next stored frame");
            continue;
        }
        require(!world->withBoxDestruction(readCaller,controls,
            [&]{return world->stepWithCollisionPass({},false,[]{return true;});}),"scheduled debris failure stops stored tick");
        require(!world->effectsSucceeded()&&counts.sound==1&&counts.smoke==1&&counts.flash==1&&
            counts.impact==1&&counts.mark==1&&counts.debris==1&&
            world->smokeEffects().size()==1&&world->flashEffects().slot(0).handle&&world->scriptEffects().size()==1&&
            owner->resolve(box)&&owner->resolve(box)->destroyRequested&&
            world->mainBlocksDestroyed()==destroyed+1,"scheduled source prefix retains child owners and marked caller on failure");
    }
    for(bool rejectPost:{false,true}){
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach synchronous builder creation world");world->skipToGameForTesting();
        auto skeletons=world->skeletonRegistration<2>();
        auto builder=world->builderOwner(skeletons);
        const auto first=world->nextRuntimeInstanceId();const auto draws=world->randomDrawCount();
        require(builder.create(60,160,200,[&](InstanceHandle key){
            const auto* r=owner->resolve(key);
            require(r&&r->instanceId==first&&r->spriteId==51&&r->imageSpeed==0&&r->preciseX==160,
                "terrain callback observes completed Create and its actual global ID");
            return builder.write(key,BuilderChildField::deactivate,1);
        }),"builder creates registered terrain");
        InstanceHandle skeleton;
        require(builder.create(120,100,200,[&](InstanceHandle key){
            skeleton=key;const auto* r=owner->resolve(key);
            return r&&r->instanceId==first+1&&r->objectId==120&&r->x==96&&r->y==192&&r->health==60&&r->imageSpeed==0;
        }),"builder skeleton callback runs after inherited grid snap and source Create");
        require(builder.create(93,176,224,[&](InstanceHandle key){
            const auto* r=owner->resolve(key);return r&&r->instanceId==first+2&&r->imageIndex==23&&r->imageSpeed==0;
        }),"platform joins the same source ID stream");
        require(world->nextRuntimeInstanceId()==first+3&&world->randomDrawCount()==draws&&skeletons.instances().size()==1,
                "mixed terrain/skeleton births retain IDs and RNG");
        require(builder.create(92,208,224,[&](InstanceHandle key){
            const auto* r=owner->resolve(key);
            require(r&&r->objectId==92&&r->spriteId==51&&r->imageSpeed==0&&
                    (r->imageIndex==16||r->imageIndex==17||r->imageIndex==22)&&
                    r->box.materialBreakable&&!r->box.groundRoomResult&&r->box.breakSound>=171&&r->box.breakSound<=174,
                    "box caller observes source Create state rather than a blank generic command");
            return builder.deactivate(key);
        }),"builder creates and then engine-deactivates source box");
        unsigned steps=0;
        require(world->withSkeletonEvents(skeletons,[&](InstanceHandle key,auto phase,auto&,auto&,bool&){
            require(skeletons.worldHandle(key).generation==skeleton.generation&&skeletons.worldHandle(key).slot==skeleton.slot,
                    "builder-created skeleton enters the registered room scheduler");
            if(phase==StartupRuntime::SkeletonPhase::step)++steps;return true;
        },[&]{return world->stepWithCollisionPass(Input{},false,[]{return true;});})&&steps==1,
                "one world tick schedules the builder-created skeleton once");
        const auto before=world->nextRuntimeInstanceId();unsigned callbacks=0;
        require(!builder.create(rejectPost?120:102,240,240,[&](InstanceHandle){++callbacks;return false;}),
                "unsupported family or post-Create failure stops the builder owner");
        require(!world->effectsSucceeded()&&world->nextRuntimeInstanceId()==before+unsigned(rejectPost)&&callbacks==unsigned(rejectPost),
                "failure preserves only the actually allocated source prefix");
        require(skeletons.clear(),"retire borrowed skeleton lifetimes before test-world teardown");
    }
    {
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach builder publication world");world->skipToGameForTesting();
        InstanceHandle wall{};
        for(unsigned i=0;i<world->activeInstanceCount();++i)
            if(world->instanceAt(i).objectId==60){wall=world->instanceHandleAt(i);break;}
        const auto* record=owner->resolve(wall);require(record,"source room has a registered wall");
        const int oldMask=record->maskId;
        auto publication=world->builderPublication();
        require(publication.write(wall,BuilderChildField::deactivate,1)&&record->active&&record->wall.deactivate,
                "builder wall flag does not deactivate its collision lifetime");
        require(publication.write(wall,BuilderChildField::maskIndex,594)&&
                publication.write(wall,BuilderChildField::indexVariable,15)&&
                publication.write(wall,BuilderChildField::imageIndex,15)&&
                publication.write(wall,BuilderChildField::checked,1),"publish boundary wall through actual collision owner");
        require(record->maskId==594&&record->wall.imageIndex==15&&record->imageIndex==15&&record->wall.checked==1,
                "stored boundary fields retain all ordered writes");
        Families families;require(families.bindInstanceFlags(objects),"builder query family metadata");
        unsigned changed=0;
        for(int y=record->y-64;y<=record->y+64;y+=8)for(int x=record->x-192;x<=record->x+192;x+=8){
            bool expected=false,oldExpected=false;
            for(unsigned i=0;i<world->activeInstanceCount();++i){
                const auto& r=world->instanceAt(i);
                if(!r.active||r.destroyRequested||!families.isA(r.objectId,60))continue;
                auto hit=[&](int mask){
                    PackedSpriteCollision s;
                    require(sprites.collisionSprite(r.spriteId,mask,s),"wall has source bounds");
                    const int px=x-r.x+s.originX,py=y-r.y+s.originY;
                    return px>=s.marginLeft&&px<=s.marginRight&&py>=s.marginTop&&py<=s.marginBottom;
                };
                expected|=hit(r.maskId);oldExpected|=hit(&r==record?oldMask:r.maskId);
            }
            InstancePlaceResult selected;
            require(owner->collisionPoint({},60,MotionBinary64::fromInteger(x),MotionBinary64::fromInteger(y),false,false,selected)&&
                    (selected.id!=-4)==expected,"post-builder collision query uses new resource mask, not stale cached bounds");
            changed+=expected!=oldExpected;
        }
        require(changed&&world->effectsSucceeded()&&owner->valid(),"boundary mask has observable collision changes");
    }
    {
        struct CompanionImages:StoredWorldCollision {
            using StoredWorldCollision::StoredWorldCollision;
            unsigned images=0;
            bool imageIndex(InstanceHandle h,const InstanceRecord& r,RunnerImageValue value){
                if(!StoredWorldCollision::imageIndex(h,r,value))return false;
                if(r.objectId==249||r.objectId==256)++images;
                return true;
            }
        };
        auto owner=std::make_unique<CompanionImages>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
            InstanceLifecycleBinding::bindPointQueries(*owner));
        require(owner->attach(*world)&&world->grantUpgrade(9)&&world->grantUpgrade(15),"attach companion creation world");
        const_cast<UpgradeInventory&>(world->upgrades()).levels[9]=2; // Source value-two branch, not a normal offer.
        world->skipToGameForTesting();
        require(world->effectsSucceeded()&&world->roomLoadSucceeded()&&owner->valid(),"real collision owner admits companion births");
        unsigned drones=0,balloons=0;
        require(owner->instanceNumber(249,drones)&&owner->instanceNumber(256,balloons)&&drones==2&&balloons==1,
            "companions occupy actual resource-backed object membership");
        for(std::size_t i=0;i<world->activeInstanceCount();++i){
            const auto& record=world->instanceAt(i);
            if(record.objectId!=249&&record.objectId!=256)continue;
            const auto key=world->instanceHandleAt(i);RunnerImageValue x,y;
            require(owner->resolve(key)==&record&&world->companionCreateState(record.instanceId)&&
                owner->collisionPosition(key,x,y)&&x.bits==0x43700000u&&y.bits==0x43300000u,
                "companion numeric ID, generation, Create state and published birth coordinates agree");
        }
        const auto before=owner->images;
        for(unsigned tick=0;tick<64;++tick){
            world->step(false);
            require(world->effectsSucceeded()&&owner->valid(),"companion animation retains valid real collision owner");
        }
        require(owner->images==before+64*3,"each companion animation publishes once through the resource-backed owner");
    }
    {
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach registered skeleton throw world");world->skipToGameForTesting();
        auto bones=world->boneRegistration<2>();
        checkRegisteredSkeletonThrows(*world,[&](auto& caller,bool separated,int facing,bool& shot,auto sound){
            return world->throwRegisteredBone(bones,caller,240,200,separated,facing,shot,sound);
        },[&](InstanceHandle key){return bones.instances().find(key);},[&]{return bones.clear();});
    }
    {
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach registered skeleton world");world->skipToGameForTesting();
        auto skeletons=world->skeletonRegistration<2>();InstanceHandle key;
        const auto id=world->nextRuntimeInstanceId();const auto draws=world->randomDrawCount();
        require(world->spawnSkeleton(skeletons,100,200,key),"create skeleton through real resource owner");
        const auto* record=world->instance(id);const auto* actor=skeletons.instances().find(key);
        require(record&&actor&&record->objectId==120&&record->spriteId==208&&record->maskId==208&&
            record->x==96&&record->y==192&&record->health==60&&record->reward==10&&record->imageSpeed==0&&
            actor->sourceId==id&&actor->state.x==96&&actor->state.y==192&&world->randomDrawCount()==draws,
            "skeleton inherited grid snap and Create constants consume no RNG");
        require(skeletons.dispatch(key,[](auto& state,auto&,bool&){
            state.imageSpeed=MotionBinary64::fromInteger(1);state.shot=true;state.alarms[2]=2;return true;
        }),"arm skeleton source alarm");
        unsigned steps=0,ends=0;
        auto tick=[&]{return world->withSkeletonEvents(skeletons,[&](InstanceHandle selected,auto phase,auto&,auto&,bool&){
            require(selected.generation==key.generation,"resolve skeleton generation in each room event");
            if(phase==StartupRuntime::SkeletonPhase::step)++steps;else ++ends;return true;
        },[&]{return world->stepWithCollisionPass(Input{},false,[]{return true;});});};
        require(tick()&&steps==1&&ends==0&&record->alarms.get(2)==1&&actor->state.shot,"first shared skeleton tick");
        require(tick()&&steps==2&&ends==0&&!actor->state.shot&&record->alarms.get(2)==0&&record->alarms.get(3)==14&&
            record->imageIndex==0,"skeleton alarm decrements once and later slot sees same-phase write");
        require(skeletons.dispatch(key,[](auto& state,auto&,bool&){state.hitPending=true;return true;}),"hit source skeleton");
        unsigned hurts=0;SkeletonSoundAudio audio;audio.wet=true;
        SoundPlayEnvironment soundEnvironment;soundEnvironment.gameVolume={0x3fd00000u,0};
        soundEnvironment.viewY=MotionBinary64::fromInteger(10000);soundEnvironment.inWater=true;
        TimeStopEffects<4> skeletonStop;skeletonStop.player.stopped=true;
        auto skeletonTick=[&]{return world->withSkeletonTimeStopEvents(skeletons,skeletonStop,
            [&](auto,auto& actor,auto&){return soundingSkeletonEffects(hurts,[&actor,&world,&soundEnvironment,&audio](int id){
                return world->playSkeletonSound(actor,id,soundEnvironment,audio);
            });},[&]{world->step({},false);return world->effectsSucceeded();});};
        require(skeletonTick()&&!hurts&&actor->state.hitPending&&record->imageSpeed==0,
            "source stopped state defers skeleton hit processing");
        skeletonStop.player.stopped=false;
        require(skeletonTick()&&hurts==1&&
            record->spriteId==205&&record->imageSpeed==0&&record->alarms.get(1)==4&&record->alarms.get(3)==30&&
            actor->state.hitStun&&!actor->state.hitPending,"checked hit Step executes in the stored room scheduler");
        require(audio.calls==7&&world->overlapSound(4)==133&&actor->state.sound.sndsnd==1001&&
            soundEnvironment.viewHeight.high==MotionBinary64::fromInteger(284).high&&
            soundEnvironment.viewY.high!=MotionBinary64::fromInteger(10000).high,
            "stored hit Step runs source enemymove sound with retained caller and global channel");
        require(world->withSkeletonPublishedEvents(skeletons,[&](InstanceHandle,auto phase,auto& state,auto&,auto& publish,bool&){
            if(phase!=StartupRuntime::SkeletonPhase::step)return true;
            require(publish.sprite(207)&&publish.imageIndex({0x40800000u})&&record->imageIndex==4,"publish source image index immediately");
            require(publish.sprite(205)&&record->spriteId==205&&record->imageIndex==0&&
                state.sprite==205&&!state.timeline.bits,"hit sprite applies its sequence reset before another callback");
            require(publish.sprite(207)&&publish.facing(-1)&&record->spriteId==207&&record->scaleX==-1&&state.facing==-1,
                "registered sprite and facing are visible before child creation");
            const auto oldX=state.xx,oldY=state.yy;
            require(publish.position({0x42e00000u},{0x43400000u})&&record->x==112&&record->y==192&&
                publish.position({0x42e00000u},{0x43500000u})&&record->y==208&&state.x==112&&state.y==208&&
                state.xx.high==oldX.high&&state.xx.low==oldX.low&&state.yy.high==oldY.high&&state.yy.low==oldY.low,
                "position setters do not rewrite private accumulators");
            return true;
        },[&]{return world->stepWithCollisionPass(Input{},false,[]{return true;});}),"skeleton setters join actual room Step");
        require(skeletons.mark(key)&&tick()&&!world->instance(id)&&!skeletons.instances().find(key),
            "skeleton marked lifetime retires through normal cleanup");
        InstanceHandle next;require(world->spawnSkeleton(skeletons,100,200,next)&&next.generation!=key.generation&&
            skeletons.clear(),"skeleton reuse does not alias stale generations");
    }
    {
        struct SnapOwner:StoredWorldCollision {
            using StoredWorldCollision::StoredWorldCollision;
            unsigned moves=0;
            bool move(InstanceHandle h,const InstanceRecord& r,RunnerImageValue x,RunnerImageValue y){
                if(r.objectId==120&&++moves==2)return false;
                return StoredWorldCollision::move(h,r,x,y);
            }
        };
        auto owner=std::make_unique<SnapOwner>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
            InstanceLifecycleBinding::bindPointQueries(*owner));
        require(owner->attach(*world),"attach failing skeleton snap owner");world->skipToGameForTesting();
        auto skeletons=world->skeletonRegistration<1>();InstanceHandle key;
        const auto id=world->nextRuntimeInstanceId();const auto draws=world->randomDrawCount();
        require(!world->spawnSkeleton(skeletons,100,200,key)&&!world->effectsSucceeded()&&
            world->nextRuntimeInstanceId()==id+1&&world->randomDrawCount()==draws&&owner->moves==2,
            "failed snap retains registered birth without RNG or retry");
        const auto* retained=world->instance(id);const auto* actor=skeletons.instances().find(key);
        require(retained&&actor&&retained->x==96&&retained->y==200&&actor->x.bits==0x42c00000u&&
            actor->y.bits==0x43480000u,"failed second snap publication retains successful X only");
        require(!world->spawnSkeleton(skeletons,100,200,key)&&world->nextRuntimeInstanceId()==id+1&&
            owner->moves==2,"terminal skeleton Create cannot run twice");
    }
    {
        struct SpriteOwner:StoredWorldCollision {
            using StoredWorldCollision::StoredWorldCollision;
            unsigned calls=0;
            bool sprite(InstanceHandle h,const InstanceRecord& r,int id){
                if(r.objectId==120){++calls;if(id==205)return false;}
                return StoredWorldCollision::sprite(h,r,id);
            }
        };
        auto owner=std::make_unique<SpriteOwner>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
            InstanceLifecycleBinding::bindPointQueries(*owner));
        require(owner->attach(*world),"attach failing skeleton sprite owner");world->skipToGameForTesting();
        auto skeletons=world->skeletonRegistration<1>();InstanceHandle key;
        require(world->spawnSkeleton(skeletons,100,200,key),"create skeleton before failed setter");
        const auto* actor=skeletons.instances().find(key);const auto* record=world->instance(actor->sourceId);
        require(!skeletons.dispatchPublished(key,[&](auto&,auto&,auto& publish,bool&){
            return publish.sprite(207)&&publish.facing(-1)&&publish.imageIndex({0x40800000u})&&publish.sprite(205);
        })&&!world->effectsSucceeded()&&owner->calls==2&&record->spriteId==207&&record->scaleX==-1&&record->imageIndex==4&&
            actor->state.sprite==207&&actor->state.facing==-1,"failed sprite keeps prior publications without installing new art");
        require(!skeletons.dispatchPublished(key,[](auto&,auto&,auto&,bool&){return true;})&&owner->calls==2,
            "failed skeleton setter cannot be retried");
    }
    {
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach animation-driven skeleton encounter");world->skipToGameForTesting();
        auto skeletons=world->skeletonRegistration<1>();auto bones=world->boneRegistration<4>();InstanceHandle skeleton;
        require(world->spawnSkeleton(skeletons,240,200,skeleton),"register encounter skeleton");
        TimeStopEffects<4> stop;auto shards=world->timeShardRegistration<4>();auto changes=world->timeChangeRegistration<8>();
        require(stop.shards.bindPhases(shards)&&stop.changes.bind(changes),"bind encounter time state");
        unsigned throws=0,hurts=0;
        for(unsigned frame=0;frame<24;++frame){
            require(world->withSkeletonTimeStopEvents(skeletons,stop,[&](InstanceHandle,auto& actor,auto& throwing){
                auto launch=[&](int){
                    const auto before=world->randomDrawCount();
                    SkeletonSoundAudio audio;SoundPlayEnvironment environment;environment.gameVolume={0x3fd00000u,0};
                    const bool ok=world->throwSkeletonBone(bones,actor,throwing,environment,audio);
                    require(ok&&world->randomDrawCount()==before+4&&audio.calls==7&&world->overlapSound(4)==134&&
                        throwing.myBoneSourceId&&throwing.xarc.high==0x3ff00000u&&!throwing.xarc.low,
                        "coincident live player skips horizontal RNG after registered Create and real sound script");
                    ++throws;return ok;
                };
                return ThrowingSkeletonEffects<decltype(launch)>{hurts,launch};
            },[&]{return world->stepWithTimeStopAndBones(bones,shards,changes,stop,Input{},
                [&](int sprite,unsigned& count){return sprites.frameCount(sprite,count);},StartupRuntime::NoTimeTransition{});}),
                "shared skeleton/bone room tick");
        }
        require(throws==1&&!hurts&&bones.instances().size()==1&&stop.reset()&&bones.clear()&&skeletons.clear(),
            "animation drives one registered source throw without invented death effects");
    }
    for(unsigned failure=0;failure<6;++failure){
        struct BloodOwner:StoredWorldCollision {
            using StoredWorldCollision::StoredWorldCollision;
            unsigned failure=0,moves=0;
            bool sprite(InstanceHandle h,const InstanceRecord& r,int id){
                return !(r.objectId==220&&failure==2)&&StoredWorldCollision::sprite(h,r,id);
            }
            bool transform(InstanceHandle h,const InstanceRecord& r,RunnerTransformField field,
                RunnerImageValue value,RunnerImageValue alpha,std::uint32_t blend){
                return !(r.objectId==220&&failure==3)&&StoredWorldCollision::transform(h,r,field,value,alpha,blend);
            }
            bool move(InstanceHandle h,const InstanceRecord& r,RunnerImageValue x,RunnerImageValue y){
                if(r.objectId==220&&++moves==failure-3&&failure>=4)return false;
                return StoredWorldCollision::move(h,r,x,y);
            }
        };
        auto owner=std::make_unique<BloodOwner>(objects,sprites,masks);owner->failure=failure;
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
            InstanceLifecycleBinding::bindPointQueries(*owner));
        require(owner->attach(*world),"attach registered blood world");world->skipToGameForTesting();
        auto blood=world->bloodRegistration<1>();InstanceHandle key;unsigned calls=0;
        const auto id=world->nextRuntimeInstanceId();const auto rng=world->randomDrawCount();
        const RunnerImageValue x{0x43704000u},y{0x43308000u};const MotionBinary64 angle{0x402b8000u,0};
        const bool created=world->spawnBlood(blood,x.promoted(),y.promoted(),angle,[&](InstanceHandle child){
            ++calls;const auto* record=blood.instances().find(child);const auto* stored=world->instance(id);
            require(record&&stored&&record->sourceId==id&&record->spriteId==-1&&stored->spriteId==-1&&
                record->state.xx.high==x.promoted().high&&record->state.xx.low==x.promoted().low&&
                !record->state.xsp.high&&record->state.imageSpeed.bits==0x3f800000u,
                "myFx observes completed no-RNG Create before sprite/speed/angle/velocity");
            return failure!=1;
        },key);
        require(created==(failure==0||failure>=4)&&calls==1&&world->randomDrawCount()==rng&&world->instance(id),
            "child identity and successful creation prefix survive failed caller/setters");
        if(!created){require(!world->effectsSucceeded()&&!blood.valid(),"failed creation latches world");continue;}
        const auto* record=blood.instances().find(key);const auto* stored=world->instance(id);BloodEffect reference;
        require(reference.create(x.promoted(),y.promoted(),angle)&&record&&stored->spriteId==84&&stored->rotation==13.75f&&
            stored->imageSpeed==0.5f&&record->state.xsp.high==reference.xsp.high&&record->state.xsp.low==reference.xsp.low,
            "emitMovingFx properties match scalar source specialization");
        require(blood.step(key,[](const auto& r,bool& allowed){
            require(r.state.savedSpeed.bits==0x3f000000u,"query observes captured speed");allowed=true;return true;
        })==(failure==0),"registered blood publishes movement with checked failure");
        if(failure){
            require(owner->moves==failure-3&&!world->effectsSucceeded()&&!blood.valid(),"movement stops at rejected setter");
            if(failure==5)require(record->x.bits==x.bits&&record->y.bits!=y.bits,"Y completed before failed X publication");
            continue;
        }
        for(unsigned tick=0;tick<18;++tick)require(blood.advanceAnimation(key),"registered animation event");
        require(record->destroyRequested&&stored->destroyRequested&&blood.retire(key)&&!world->instance(id),
            "animation end marks both owners before deferred retirement");
        InstanceHandle reused;
        require(world->spawnBlood(blood,{}, {},{},[](auto){return true;},reused)&&reused.slot==key.slot&&
            reused.generation!=key.generation&&!blood.instances().find(key)&&blood.clear(),"reuse preserves generation safety");
    }
    for(unsigned failAt:{0u,1u,2u}){
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
            InstanceLifecycleBinding::bindPointQueries(*owner));
        require(owner->attach(*world),"attach registered blood burst world");world->skipToGameForTesting();
        auto blood=world->bloodRegistration<2>();auto random=world->randomState();
        const auto draws=world->randomDrawCount();const auto firstId=world->nextRuntimeInstanceId();
        unsigned calls=0,myFx=0;InstanceHandle children[2]{};
        const bool ok=world->spawnBloodBurst(blood,MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(192),
            [&](InstanceHandle child,unsigned id){
                const auto* record=blood.instances().find(child);children[calls]=child;++calls;myFx=id;
                require(record&&record->spriteId==-1&&id==firstId+calls-1&&
                    world->randomDrawCount()==draws+calls,"burst interleaves each RNG draw, Create and caller publication");
                if(calls==2)require(blood.instances().find(children[0])->spriteId==84,
                    "first child setters finish before the second caller write");
                return calls!=failAt;
            });
        require(ok==(failAt==0)&&calls==(failAt?failAt:2)&&blood.instances().size()==calls&&
            myFx==firstId+calls-1&&world->nextRuntimeInstanceId()==firstId+calls,
            "burst retains identities and caller prefix after rejection");
        for(unsigned i=0;i<calls;++i){
            MotionBinary64 angle;require(randomBinary64(random.nextU32(),MotionBinary64::fromInteger(20),angle),"burst angle oracle");
            if(i){angle.high^=0x80000000u;require(angle.add(MotionBinary64::fromInteger(180)),"second blood angle oracle");}
            if(!failAt||i+1<failAt){RunnerImageValue expected;require(expected.assign(angle)&&
                blood.instances().find(children[i])->state.imageAngle.bits==expected.bits,"burst preserves original random angle");}
        }
        for(unsigned i=0;i<16;++i)require(random.stateWord(i)==world->randomState().stateWord(i),"burst full RNG state");
        if(!ok){const auto before=world->randomDrawCount();require(!world->spawnBloodBurst(blood,{}, {},[](auto,unsigned){return true;})&&
            world->randomDrawCount()==before,"terminal failure forbids burst replay");}
        else require(blood.clear(),"successful burst teardown");
    }
    for(int x:{159,160,240,320,321}){
        struct BloodQueryOwner:StoredWorldCollision {
            using StoredWorldCollision::StoredWorldCollision;
            unsigned queries=0;
            bool placeMeeting(InstanceHandle key,const InstanceRecord& record,int object,
                MotionBinary64 x,MotionBinary64 y,bool& hit){
                if(record.objectId==220){++queries;require(object==82&&record.spriteId==84,
                    "blood uses its registered rotated sprite against parentTimeField");}
                return StoredWorldCollision::placeMeeting(key,record,object,x,y,hit);
            }
        };
        auto owner=std::make_unique<BloodQueryOwner>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
            InstanceLifecycleBinding::bindPointQueries(*owner));
        require(owner->attach(*world),"attach blood scheduled world");world->skipToGameForTesting();
        auto blood=world->bloodRegistration<1>();InstanceHandle key;
        require(world->spawnBlood(blood,MotionBinary64::fromInteger(x),MotionBinary64::fromInteger(-1000),
            MotionBinary64::fromInteger(14),[](auto){return true;},key),"scheduled blood birth");
        unsigned steps=0;bool stopped=true;
        const auto tick=[&]{return world->withBloodEvents(blood,[&](InstanceHandle selected,const auto& record,bool& allowed){
            ++steps;require(selected.slot==key.slot&&selected.generation==key.generation&&record.state.savedSpeed.bits==0x3f000000u,
                "scheduled query follows saved-speed capture");
            return world->bloodTimeAllowed(blood,selected,stopped,allowed);
        },[&]{world->step({},false);return world->effectsSucceeded();});};
        require(tick()&&steps==1&&owner->queries==1,"field query precedes even an outside-side short circuit");
        const auto* child=blood.instances().find(key);
        require(child&&child->state.timeline.bits==0x3f000000u&&
            child->state.imageSpeed.bits==((x<160||x>320)?0x3f000000u:0u),
            "animation runs before stopped Step, with strict source side bounds");
        stopped=false;
        require(tick()&&steps==2&&owner->queries==1,"running Step bypasses field query and restores speed");
        for(unsigned remaining=0;remaining<18&&blood.instances().find(key);++remaining)
            require(tick(),"blood expiry through the stored room scheduler");
        require(!blood.instances().find(key)&&blood.instances().size()==0,
            "stored Animation End and cleanup retire the registered lifetime");
        require(blood.clear(),"scheduled blood teardown");
    }
    for(bool reject:{false,true}){
        struct FieldOwner:StoredWorldCollision {
            using StoredWorldCollision::StoredWorldCollision;
            StartupRuntime* runtime=nullptr;bool reject=false;
            unsigned fieldBirths=0;std::uint64_t before=0;
            bool birth(InstanceHandle key,const InstanceRecord& record){
                if(record.objectId==82&&runtime){
                    ++fieldBirths;
                    require(runtime->randomDrawCount()==before,
                        "Timeout field membership precedes radius RNG");
                    if(reject)return false;
                }
                return StoredWorldCollision::birth(key,record);
            }
        };
        auto owner=std::make_unique<FieldOwner>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
            InstanceLifecycleBinding::bindPointQueries(*owner));
        require(owner->attach(*world),"attach runtime Timeout field world");world->skipToGameForTesting();
        owner->runtime=world.get();owner->reject=reject;owner->before=world->randomDrawCount();
        const auto id=world->nextRuntimeInstanceId();
        const auto count=world->timeFields().size();auto oracle=world->randomState();
        InstanceHandle child;
        require(world->spawnOrdinaryTimeField(240,40000,child)==!reject,"registered Timeout field birth");
        require(owner->fieldBirths==1&&world->randomDrawCount()==owner->before+unsigned(!reject),
            "rejected Timeout membership consumes no Create RNG");
        if(reject){
            require(!world->effectsSucceeded()&&!child.generation&&world->timeFields().size()==count,
                "failed Timeout birth retains terminal state and no radius record");
            require(!world->spawnOrdinaryTimeField(240,40000,child)&&owner->fieldBirths==1,
                "failed Timeout birth cannot retry a partial script");
        }else{
            const auto* field=world->timeFields().at(count);
            require(field&&field->sourceId==id&&field->x==240&&field->y==40000&&
                field->radius.equal(OrdinaryTimeField::nextRadius(oracle)),
                "Timeout render owner retains room source identity and exact Create sample");
            InstancePlaceResult selected;
            RunnerImageValue x,y;x.assignInteger(240);y.assignInteger(40000);
            require(world->playerTimeFieldInstance(x,y,selected)&&selected.id==static_cast<int>(id),
                "new Timeout field participates in source identity queries immediately");
            TimeoutDamageEvents events;
            ContactDamageScripts damage;owner->before=world->randomDrawCount();
            const auto next=world->nextRuntimeInstanceId();
            require(world->typicalPlayerDamage(damage,MotionBinary64::fromInteger(1),
                MotionBinary64::fromInteger(3),MotionBinary64::fromInteger(2),events)&&
                events.players==2&&events.clears==2&&events.spin&&!events.grounded&&events.alarm==90&&world->timeFields().size()==count+2&&
                world->timeFields().at(count+1)->sourceId==next&&
                world->timeFields().at(count+1)->y==40100&&world->randomDrawCount()==owner->before+1,
                "Timeout script uses published player coordinates and rereads stop state for each birth");
        }
    }
    for(unsigned reject:{0u,1u,2u}){
        struct RowOwner:StoredWorldCollision {
            using StoredWorldCollision::StoredWorldCollision;
            unsigned writes=0,reject=0;
            bool move(InstanceHandle h,const InstanceRecord& r,RunnerImageValue x,RunnerImageValue y){
                if(r.objectId==65&&++writes==reject)return false;
                return StoredWorldCollision::move(h,r,x,y);
            }
        };
        auto owner=std::make_unique<RowOwner>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
            InstanceLifecycleBinding::bindPointQueries(*owner));
        require(owner->attach(*world),"attach literal builder row publication");world->skipToGameForTesting();
        auto skeletons=world->skeletonRegistration<2>();auto builder=world->builderOwner(skeletons);
        BuilderStringTraversal cursor;cursor.y=704;cursor.setPillar=false;
        owner->reject=reject;unsigned births=0;
        const std::uint8_t text[]="\r\n\tj\r\n\tj\r\n\t";
        const auto first=world->nextRuntimeInstanceId();
        const bool ok=cursor.run(text,sizeof(text)-1,0,[](char,int,int){return true;},
            [&](char c,int x,int y){
                require(c=='j',"fixture emits only selected skeleton cells");
                const auto* controller=world->instance(100856);
                require(controller&&controller->y==y&&controller->preciseY==float(y),
                    "next synchronous child observes the published builder row");
                ++births;return builder.create(120,x,y,[](InstanceHandle){return true;});
            },[](int,int){return true;},[&](int y){return builder.publishBuilderY(y);});
        require(ok==(reject==0)&&world->effectsSucceeded()==ok&&owner->writes==(reject?reject:2)&&
            births==(reject==1?1u:2u)&&world->nextRuntimeInstanceId()==first+births,
            "row setter failure stops later registered child births without consuming their IDs");
        const auto* controller=world->instance(100856);
        require(controller&&controller->y==cursor.y&&controller->preciseY==float(cursor.y),
            "failed row write retains matching published and traversal prefixes");
        require(skeletons.clear(),"clear literal-builder skeleton lifetimes");
    }
    for(int offset:{-161,-160,3000,3001}){
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
            InstanceLifecycleBinding::bindPointQueries(*owner));
        require(owner->attach(*world),"attach skeleton End Step world");world->skipToGameForTesting();
        auto skeletons=world->skeletonRegistration<1>();InstanceHandle key;
        require(world->spawnSkeleton(skeletons,240,192,key),"register view-retirement skeleton");
        RunnerImageValue x,y;require(x.assignInteger(240)&&y.assignInteger(world->mainCameraRenderViewY()+offset),"published view boundary");
        require(skeletons.dispatchPublished(key,[&](auto&,auto&,auto& publish,bool&){return publish.position(x,y);}),"publish End Step position");
        const auto sourceId=skeletons.instances().find(key)->sourceId;
        unsigned endCallbacks=0;
        require(world->withSkeletonPublishedEvents(skeletons,[&](auto,auto phase,auto&,auto&,auto&,bool&){
            endCallbacks+=phase==StartupRuntime::SkeletonPhase::endStep;return true;
        },[&]{world->step({});return world->effectsSucceeded();}),"real room tick owns skeleton End Step");
        const bool retire=offset==-161||offset==3001;
        require(!endCallbacks&&bool(world->instance(sourceId))==!retire&&skeletons.instances().size()==unsigned(!retire),
            "strict source view boundaries retire through both owners, not external no-op callbacks");
        require(skeletons.clear(),"clear view boundary fixture");
    }
    for(unsigned mode=0;mode<4;++mode){
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
            InstanceLifecycleBinding::bindPointQueries(*owner));
        require(owner->attach(*world),"attach smoke collision world");world->skipToGameForTesting();
        auto smoke=world->smokeRegistration<1>();std::uint32_t id=0;
        require(world->createRegisteredSmoke(smoke,MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(192),id),
            "register collision smoke");
        InstanceHandle self{},walls[2]{};unsigned count=0;
        require(world->visitDrawChecked([&](InstanceHandle key,const InstanceRecord& record){
            if(record.instanceId==id)self=key;
            if(record.objectId==60&&count<2)walls[count++]=key;
            return true;
        })&&self.generation&&count==2,"select two distinct live wall lifetimes");
        const auto* record=world->instance(id);auto expected=*smoke.state(*record);
        if(mode){require(smoke.alarmEvent(*record,0),"source alarm clears smoke contact guard");expected.alarm0();}
        const auto draws=world->randomDrawCount();RunnerCollisionEventContext context;
        for(unsigned i=0;i<2;++i){
            const bool ok=owner->dispatchCollisionEvent(context,self,walls[i],{true,false},[&](auto event){
                if(mode==2)event.code=2016; // Cannot forge water's empty override for a wall.
                if(mode==3)++event.other.generation;
                return world->dispatchSmokeCollision(smoke,event);
            });
            require(ok==(mode<2),"smoke validates selected binding and opposite lifetime");
            if(!ok)break;
            require(expected.collideWall(),"scalar wall event");
            const auto* state=smoke.state(*record);
            require(state&&state->speed.high==expected.speed.high&&state->speed.low==expected.speed.low&&
                state->xsp.high==expected.xsp.high&&state->ysp.high==expected.ysp.high&&
                state->imageSpeed.bits==expected.imageSpeed.bits,
                "each wall independently scales speed without recomputing velocity or animation rate");
        }
        require(world->randomDrawCount()==draws&&smoke.clear(),"contacts consume no RNG and permit explicit teardown");
    }
    for(unsigned mode=0;mode<8;++mode){
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
            InstanceLifecycleBinding::bindPointQueries(*owner));
        require(owner->attach(*world),"attach skeleton collision world");world->skipToGameForTesting();
        if(mode>=6)world->registerLevelExit(192);
        auto skeletons=world->skeletonRegistration<1>();InstanceHandle local,other;
        require(world->spawnSkeleton(skeletons,240,192,local),"spawn collision skeleton");
        require(world->visitDrawChecked([&](InstanceHandle key,const InstanceRecord& record){
            bool found=false;PackedObjectEvent binding{};
            if(!objects.collisionEvent(120,record.objectId,found,binding))return false;
            if(found&&binding.codeId==(mode>=6?1066:1266)&&!other.generation)other=key;return true;
        })&&other.generation,"select live player or inherited time-field binding");
        if(mode==7)require(skeletons.dispatch(local,[](auto& state,auto&,bool&){state.timefieldImmune=true;return true;}),"immune fixture");
        RunnerCollisionEventContext context;unsigned stomps=0,damages=0;const auto self=skeletons.worldHandle(local);
        const bool ok=owner->dispatchCollisionEvent(context,self,other,{true,false},[&](const auto& event){
            return world->dispatchSkeletonCollision(skeletons,event,[&](auto& actor){return CheckedSkeletonContact{actor,mode,stomps,damages};});
        });
        require(ok==(mode!=3&&mode!=5)&&stomps==unsigned(mode>=1&&mode<=3)&&
            damages==unsigned(mode==2||mode==4||mode==5),"skeleton source independent IFs and callback failures");
        if(ok){require(skeletons.instances().find(local)->state.health==(mode==6?0:60),"time-field inheritance and immunity");
            require(skeletons.clear(),"clear skeleton collision fixture");}
    }
    for(unsigned mode:{4u,5u}){
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach collected skeleton-player encounter");world->skipToGameForTesting();
        auto pass=std::make_unique<StoredDitherCollisionPass>(*owner);
        auto skeletons=world->skeletonRegistration<1>();auto bones=world->boneRegistration<1>();
        auto smoke=world->smokeRegistration<1>();auto particles=world->consumptionBloodRegistration<1>();
        int x=0,y=0;bool found=false;
        for(unsigned i=0;i<world->activeInstanceCount();++i){const auto& r=world->instanceAt(i);
            if(r.objectId==35){x=r.x;y=r.y;found=true;break;}}
        require(found,"find actual player collision position");
        PackedObjectEvent reverse{};bool hasReverse=true;
        require(objects.collisionEvent(35,120,hasReverse,reverse)&&!hasReverse,"player has no reverse skeleton handler");
        InstanceHandle skeleton;require(world->spawnSkeleton(skeletons,x,y,skeleton),"register overlapping skeleton");
        struct Audio {
            bool isPlaying(int,bool& playing){playing=false;return true;}
            bool stop(int){return true;}
            bool play(int,int,bool,int&){return false;}
            bool getGain(int,MotionBinary64&){return false;}
            bool setGain(int,MotionBinary64,int){return false;}
            bool setPitch(int,MotionBinary64){return false;}
        } audio;
        SoundPlayEnvironment environment;TimeStopEffects<4> stop;unsigned stomps=0,damages=0;
        const bool okay=pass->runSkeletonContacts(*world,skeletons,bones,smoke,particles,stop,
            [&](int sprite,unsigned& count){return sprites.frameCount(sprite,count);},environment,audio,
            [](auto dy,auto dx,auto& angle){return portableAtan(dy,dx,angle);},
            [](auto&,int,int,int){return false;},
            [&](auto& actor){
                require(&actor==&skeletons.instances().find(skeleton)->state,"factory receives retained skeleton caller");
                return CheckedSkeletonContact{actor,mode,stomps,damages};
            });
        require(okay==(mode==4)&&stomps==0&&damages==1,"collected skeleton-player handler and terminal failure");
        if(okay)require(skeletons.clear(),"retire collected skeleton fixture");
    }
    for(unsigned contact=0;contact<5;++contact){
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
            InstanceLifecycleBinding::bindPointQueries(*owner));
        require(owner->attach(*world),"attach bone collision world");world->skipToGameForTesting();
        auto bones=world->boneRegistration<1>();InstanceHandle local,other;
        require(world->spawnBone(bones,240,200,local),"register colliding bone");
        const auto self=bones.worldHandle(local);
        require(world->visitDrawChecked([&](InstanceHandle key,const InstanceRecord& record){
            bool found=false;PackedObjectEvent binding{};
            if(!objects.collisionEvent(269,record.objectId,found,binding))return false;
            if(found&&binding.codeId==(contact?2292:2291)&&!other.generation)other=key;
            return true;
        })&&other.generation,"select real opposite lifetime with recovered collision binding");
        if(contact==3)require(bones.dispatch(local,[](auto& state,auto&,bool&){state.active=false;return true;}),"inactive bone");
        unsigned damages=0;RunnerCollisionEventContext context;
        const auto rng=world->randomDrawCount();
        const bool ok=owner->dispatchCollisionEvent(context,self,other,{true,false},[&](const auto& event){
            return world->dispatchBoneCollision(bones,event,contact==2,
                [&](int sprite,unsigned& count){return sprites.frameCount(sprite,count);},
                [&](auto& caller,int amount,int horizontal,int vertical){
                    require(amount==1&&horizontal==3&&vertical==2&&!bones.instances().find(local)->destroyRequested&&
                        !bones.instances().find(local)->myFxLocal,"damage precedes impact and destruction");
                    require(&caller==&bones.instances().find(local)->state,"damage borrows the retained bone caller");
                    caller.damageScripts.typical.damage=MotionBinary64::fromInteger(amount);
                    ++damages;return contact!=4;
                });
        });
        require(ok==(contact!=4)&&damages==unsigned(contact==1||contact==4)&&world->randomDrawCount()==rng,
            "source contact guards, checked damage and zero impact RNG");
        const auto* retained=world->resolveInstance(self);
        require(retained&&retained->destroyRequested==(contact<3),"collision preserves lifetime and destruction prefix");
        require(bones.instances().find(local)->state.damageScripts.typical.damage.zero()==(damages==0),
            "caller damage writes survive failed callbacks and destruction marking");
        if(ok){const auto* actor=bones.instances().find(local);
            require(actor&&bool(actor->myFxLocal)==(contact<3),"registered impact assigned only on admitted contact");
            require(bones.clear(),"clear collision fixture");}
    }
    for(unsigned contact=0;contact<6;++contact){
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach collected bone contacts");world->skipToGameForTesting();
        auto pass=std::make_unique<StoredDitherCollisionPass>(*owner);
        auto skeletons=world->skeletonRegistration<1>();auto bones=world->boneRegistration<2>();
        auto smoke=world->smokeRegistration<1>();auto particles=world->consumptionBloodRegistration<1>();
        int x=240,y=40008;
        if(contact){
            bool found=false;
            for(unsigned i=0;i<world->activeInstanceCount();++i){const auto& r=world->instanceAt(i);
                if(r.objectId==35){x=r.x;y=r.y-4;found=true;break;}}
            require(found,"player position for collected bone overlap");
        }else{InstanceHandle wall;require(world->spawnBuilderPrimitive(60,232,40000,wall),"plain wall for bone contact");}
        InstanceHandle local;require(world->spawnBone(bones,x,y,local),"bone positioned for collected collision");
        InstanceHandle second;if(contact==5)require(world->spawnBone(bones,x,y,second),"second bone observes mutable stop state");
        if(contact==3)require(bones.dispatch(local,[](auto& state,auto&,bool&){state.active=false;return true;}),"inactive collected bone");
        struct Audio {
            bool isPlaying(int,bool& playing){playing=false;return true;}
            bool stop(int){return true;}
            bool play(int,int,bool,int& voice){voice=1000;return true;}
            bool getGain(int,MotionBinary64& gain){gain=MotionBinary64::fromInteger(1);return true;}
            bool setGain(int,MotionBinary64,int){return true;}
            bool setPitch(int,MotionBinary64){return true;}
        } audio;
        SoundPlayEnvironment environment;unsigned damage=0;TimeStopEffects<4> stop;stop.player.stopped=contact==2;
        const bool okay=pass->runSkeletonContacts(*world,skeletons,bones,smoke,particles,stop,
            [&](int sprite,unsigned& count){return sprites.frameCount(sprite,count);},environment,audio,
            [](auto dy,auto dx,auto& angle){return portableAtan(dy,dx,angle);},
            [&](auto& caller,int amount,int horizontal,int vertical){
                require(amount==1&&horizontal==3&&vertical==2&&!bones.instances().find(local)->destroyRequested,
                    "collected damage precedes bone destruction");++damage;
                caller.damageScripts.typical.damage=MotionBinary64::fromInteger(amount);
                if(contact==5)stop.player.stopped=true;
                return contact!=4;
            });
        require(okay==(contact!=4)&&damage==unsigned(contact==1||contact==4||contact==5),"collected bone guards and damage failure");
        const auto* record=bones.instances().find(local);
        require(record&&record->destroyRequested==(contact<3||contact==5)&&bool(record->myFxLocal)==(contact<3||contact==5),
            "collected bone preserves source impact and destruction order");
        if(contact==5)require(bones.instances().find(second)->destroyRequested&&bones.instances().find(second)->myFxLocal,
            "time-stop change suppresses later damage without suppressing impact or destruction");
        if(okay)require(bones.clear(),"collected bone teardown");
    }
    for(unsigned rejectedMove=0;rejectedMove<=2;++rejectedMove){
        struct BoneOwner:StoredWorldCollision {
            using StoredWorldCollision::StoredWorldCollision;
            unsigned reject=0,moves=0;
            bool move(InstanceHandle h,const InstanceRecord& r,RunnerImageValue x,RunnerImageValue y){
                if(r.objectId==269&&++moves==reject)return false;
                return StoredWorldCollision::move(h,r,x,y);
            }
        };
        auto owner=std::make_unique<BoneOwner>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
            InstanceLifecycleBinding::bindPointQueries(*owner));
        require(owner->attach(*world),"attach registered bone world");world->skipToGameForTesting();
        auto bones=world->boneRegistration<2>();InstanceHandle local;
        auto expectedRandom=world->randomState();BoneProjectile expected;
        const auto sourceId=world->nextRuntimeInstanceId();const auto draws=world->randomDrawCount();
        require(expected.create(240,200,expectedRandom)&&world->spawnBone(bones,240,200,local),"synchronous registered bone Create");
        const auto key=bones.worldHandle(local);
        require(key.generation&&owner->resolve(key)==world->instance(sourceId)&&
            bones.instances().find(local)->sourceId==sourceId&&world->randomDrawCount()==draws+3,
            "bone has real numeric ID, generation and three shared Create RNG draws");
        for(unsigned word=0;word<16;++word)require(world->randomState().stateWord(word)==expectedRandom.stateWord(word),
            "bone Create retains exact shared RNG state");
        require(bones.dispatch(local,[](auto& state,auto&,bool&){return state.motion.launch(240,200,10,-30);})&&
            expected.motion.launch(240,200,10,-30),"throw override follows registered Create");
        unsigned effects=0,steps=0,effectId=0,effectLocal=0;bool kill=false;
        auto emit=[&](){
            const auto before=world->randomDrawCount();
            const bool ok=world->spawnBoneImpact(bones,local,[&](int id,unsigned& frames){return sprites.frameCount(id,frames);});
            const auto* caller=bones.instances().find(local);
            require(ok&&caller&&world->randomDrawCount()==before,"bone impact creates real child without RNG");
            effectId=caller->myFxSourceId;effectLocal=caller->myFxLocal;
            const auto* child=world->instance(effectId);const auto* effect=world->scriptEffects().get(effectLocal);
            require(child&&child->objectId==219&&effect&&effect->spriteId==85&&effect->frames==6&&
                effect->imageSpeed.bits==0x3e99999au&&!effect->angle.bits&&!effect->depth.bits&&
                effect->x.bits==caller->x.bits&&effect->y.bits==caller->y.bits,
                "bone myFx references registered impact at published position with source defaults");
            return true;
        };
        auto view=[&](bool& visible){return world->boneInView(bones,local,visible);};
        auto execute=[&](auto& state,auto& publisher,auto&,bool& good){
            ++steps;if(kill)state.health={};
            struct Ops {
                decltype(publisher) publish;unsigned& effects;decltype(emit)& spawn;decltype(view)& query;
                bool imageSpeed(RunnerImageValue v){return publish.imageSpeed(v);}
                bool position(RunnerImageValue x,RunnerImageValue y){return publish.position(x,y);}
                bool mask(int id){return publish.mask(id);}
                bool destroy(){return publish.destroy();}
                bool effect(int id,int speed){require(id==85&&speed==3,"bone death requests source effect");++effects;return spawn();}
                bool inView(int,int,bool& visible){return query(visible);}
            } ops{publisher,effects,emit,view};
            return state.stepPublished(false,ops,good);
        };
        owner->reject=rejectedMove;
        const bool ok=world->stepWithBones(bones,Input{},execute);
        require(ok==!rejectedMove,"registered bone movement failure is terminal");
        RunnerImageValue x,y;
        require(owner->collisionPosition(key,x,y),"bone collision lifetime survives Step");
        if(rejectedMove){
            require(x.bits==(rejectedMove==1?0x43700000u:0x43710000u)&&y.bits==0x43480000u,
                "rejected bone setter retains exact preceding coordinate publication");
            const auto calls=owner->moves;
            require(!world->stepWithBones(bones,Input{},execute)&&owner->moves==calls&&steps==1,
                "failed bone event cannot replay movement or effects");
        }else{
            struct NoEffects {void destroy(){} void effect(int,int){}} noEffects;
            for(unsigned tick=0;tick<8;++tick){
                if(tick)require(world->stepWithBones(bones,Input{},execute),"normal room phases advance registered bone");
                bool crossed;require(expected.advanceImage(crossed)&&expected.step(false,[](int,int){return true;},noEffects),
                    "reference bone advances");
                const auto* actual=bones.instances().find(local);
                std::uint32_t imageBits;std::memcpy(&imageBits,&world->instance(sourceId)->imageIndex,4);
                require(actual&&actual->state.motion.x==expected.motion.x&&actual->state.motion.y==expected.motion.y&&
                    actual->state.timeline.bits==expected.timeline.bits&&
                    imageBits==expected.imageIndex.bits,
                    "registered bone state follows scalar event path");
                RunnerImageValue expectedX,expectedY;
                require(expectedX.assignInteger(expected.motion.x)&&expectedY.assignInteger(expected.motion.y)&&
                    owner->collisionPosition(key,x,y)&&x.bits==expectedX.bits&&y.bits==expectedY.bits&&
                    world->instance(sourceId)->x==expected.motion.x&&world->instance(sourceId)->y==expected.motion.y,
                    "bone state, world cache and indexed position stay synchronized");
            }
            const auto beforeInactive=bones.instances().find(local)->state.timeline.bits;
            const auto beforeSteps=steps;
            require(bones.activate(local,false)&&world->stepWithBones(bones,Input{},execute)&&steps==beforeSteps,
                "pending bone deactivation excludes ordinary Step");
            const auto afterInactive=bones.instances().find(local)->state.timeline.bits;
            require(afterInactive!=beforeInactive&&world->stepWithBones(bones,Input{},execute)&&
                bones.instances().find(local)->state.timeline.bits==afterInactive&&steps==beforeSteps,
                "room membership gives deactivated bone one final animation, then none");
            require(bones.activate(local,true)&&world->stepWithBones(bones,Input{},execute)&&steps==beforeSteps+1&&
                bones.instances().find(local)->state.timeline.bits==afterInactive,
                "reactivated bone rejoins Step before the following animation phase");
            kill=true;require(world->stepWithBones(bones,Input{},execute)&&effects==1,
                "bone health death publishes mask and effect before pre-Draw cleanup");
            require(!bones.instances().find(local)&&!world->instance(sourceId)&&!owner->resolve(key)&&bones.valid(),
                "bone cleanup retires typed state, numeric ID, collision and layer ownership together");
            require(effectId&&effectLocal&&world->instance(effectId)&&world->scriptEffects().get(effectLocal),
                "impact outlives its destroyed bone caller");
            const auto replacementId=world->nextRuntimeInstanceId();InstanceHandle replacement;
            require(world->spawnBone(bones,240,990,replacement)&&replacement.generation!=local.generation&&
                bones.instances().find(replacement)->sourceId==replacementId&&!bones.instances().find(local),
                "bone slot reuse cannot alias retired generation or numeric ID");
            InstanceHandle second,overflow;
            require(world->spawnBone(bones,240,990,second),"fill registered bone capacity");
            const auto beforeId=world->nextRuntimeInstanceId();const auto beforeRng=world->randomDrawCount();
            require(!world->spawnBone(bones,240,990,overflow)&&world->nextRuntimeInstanceId()==beforeId&&
                world->randomDrawCount()==beforeRng&&bones.instances().size()==2&&!overflow.generation,
                "bone capacity failure cannot consume a source ID or Create RNG");
            require(bones.clear(),"explicit bone teardown before world teardown");
        }
    }
    {
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach bone impact failure world");world->skipToGameForTesting();
        auto bones=world->boneRegistration<1>();InstanceHandle bone;
        require(world->spawnBone(bones,240,990,bone)&&bones.mark(bone),"retain destroyed bone before impact");
        const auto childId=world->nextRuntimeInstanceId();const auto draws=world->randomDrawCount();unsigned lookups=0;
        auto rejectSprite=[&](int sprite,unsigned&){
            ++lookups;const auto* caller=bones.instances().find(bone);
            require(sprite==85&&caller&&caller->myFxSourceId==childId&&caller->myFxLocal&&world->instance(childId),
                "source myFx assignment precedes failed sprite lookup");return false;
        };
        require(!world->spawnBoneImpact(bones,bone,rejectSprite)&&!world->effectsSucceeded()&&
            world->nextRuntimeInstanceId()==childId+1&&world->randomDrawCount()==draws&&
            world->instance(childId)&&world->instance(childId)->spriteId==-1,
            "failed bone impact keeps created child and caller but no later setters");
        require(!world->spawnBoneImpact(bones,bone,rejectSprite)&&lookups==1&&world->nextRuntimeInstanceId()==childId+1,
            "failed bone impact does not retry creation");
    }
    for(unsigned reject=1;reject<=3;++reject){
        struct CameraWrites:StoredWorldCollision {
            using StoredWorldCollision::StoredWorldCollision;
            unsigned reject=0,writes=0,id=0;InstanceHandle key;
            RunnerImageValue oldX,oldY;float oldPreciseX=0,oldPreciseY=0;int oldPixelX=0,oldPixelY=0;
            bool move(InstanceHandle h,const InstanceRecord& r,RunnerImageValue x,RunnerImageValue y){
                if(r.objectId==64&&r.roomId==11&&++writes==reject){
                    id=r.instanceId;key=h;oldPreciseX=r.preciseX;oldPreciseY=r.preciseY;oldPixelX=r.x;oldPixelY=r.y;
                    require(collisionPosition(h,oldX,oldY),"camera still registered at rejected publication");
                    return false;
                }
                return StoredWorldCollision::move(h,r,x,y);
            }
        };
        auto owner=std::make_unique<CameraWrites>(objects,sprites,masks);owner->reject=reject;
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
            InstanceLifecycleBinding::bindPointQueries(*owner));
        require(owner->attach(*world),"attach camera publication failure fixture");
        world->skipToGameForTesting();
        require(!world->effectsSucceeded()&&owner->writes==reject&&owner->id,
            "rejected camera Create/Step/collision publication latches failure");
        const auto* camera=world->instance(owner->id);RunnerImageValue x,y;
        require(camera&&camera->preciseX==owner->oldPreciseX&&camera->preciseY==owner->oldPreciseY&&
            camera->x==owner->oldPixelX&&camera->y==owner->oldPixelY&&
            owner->collisionPosition(owner->key,x,y)&&x.bits==owner->oldX.bits&&y.bits==owner->oldY.bits,
            "failed publication preserves the accepted camera record and owner prefix");
        world->step(false);require(owner->writes==reject,"failed camera world does not retry writes");
    }
    {
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach world-owned debris fixture");world->skipToGameForTesting();
        unsigned first=0,second=0;
        require(world->spawnDebris(MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(300),1,first),"owned debris birth");
        require(world->debrisEffects().size()==1&&world->instance(first),"owned debris registered");
        const auto next=world->debrisEffects().birthSequence().next;
        require(world->restartFromResults(DeathMenuSequence::Action::Retry,42),"owned debris Retry");
        require(!world->debrisEffects().size()&&!world->instance(first),"owned debris retired before room teardown");
        require(world->spawnDebris(MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(300),1,second),"owned debris rebirth");
        require(second>first&&world->debrisEffects().retained(next),"owned debris monotonic identities");
        require(world->restartFromResults(DeathMenuSequence::Action::Retry,42),"owned debris fixture cleanup");
    }
    {
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach owned debris scheduling fixture");world->skipToGameForTesting();
        unsigned id=0;
        require(world->spawnDebris(MotionBinary64::fromInteger(20000),MotionBinary64::fromInteger(20000),1,id),"owned scheduled birth");
        auto expected=*world->debrisEffects().retained(1);auto random=world->randomState();unsigned stops=0;
        for(unsigned tick=0;tick<40;++tick){
            const bool frozen=tick<5;
            require(expected.advanceImage(),"owned reference animation");
            for(unsigned a=0;a<3;++a)expected.advanceAlarm(a);
            require(expected.step(frozen,random,[](auto,auto,bool& hit){hit=false;return true;}),"owned reference Step");
            require(world->stepWithOwnedDebris({},[&](InstanceHandle,const auto&,bool& stop){++stops;stop=frozen;return true;}),
                "owned effects use normal stored room scheduler");
            const auto* actual=world->debrisEffects().retained(1);const auto* record=world->instance(id);
            if(expected.destroyRequested){require(!actual&&!record,"owned cleanup after Alarm expiry");continue;}
            require(actual&&record&&actual->x.bits==expected.x.bits&&actual->y.bits==expected.y.bits&&
                actual->imageIndex.bits==expected.imageIndex.bits&&actual->imageSpeed.bits==expected.imageSpeed.bits&&
                actual->noTouch==expected.noTouch&&actual->disappearing==expected.disappearing&&actual->flash==expected.flash,
                "owned scheduled motion and presentation match source primitives");
            for(unsigned a=0;a<3;++a)require(actual->alarms.get(a)==expected.alarms.get(a)&&
                record->alarms.get(a)==expected.alarms.get(a),"owned alarms decremented once");
        }
        require(stops==34,"owned freeze extends lifetime and no Step occurs after destruction");
    }
    for(bool failSmoke:{false,true}){
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns);
        checkBoxBurstWorld(*world,failSmoke);
    }
    {
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach effect-depth world");world->skipToGameForTesting();
        auto registration=world->scriptSpawnRegistration<2>();ScriptSpawnEffects<2> effects;unsigned id=0;
        auto frames=[&](int sprite,unsigned& count){PackedSpriteCollision art;
            if(!sprites.sprite(sprite,art))return false;count=art.frameCount;return true;};
        require(world->spawnScriptEffect(registration,effects,MotionBinary64::fromInteger(240),
            MotionBinary64::fromInteger(300),603,{},{},MotionBinary64::fromInteger(-60000),frames,id),
            "spawn box-impact depth through the stored effect registry");
        InstanceHandle key;
        for(std::size_t i=0;i<world->activeInstanceCount();++i)
            if(world->instanceAt(i).instanceId==id)key=world->instanceHandleAt(i);
        const auto& layers=world->sourceRoomLayers();
        const auto* before=layers.lifetimes().resolve(key);require(before,"resolve effect layer lifetime");
        const auto oldLayer=before->depth.layerId;
        require(owner->resolve(key)->depth==-60000&&layers.layers().get(layers.layers().find(oldLayer))->depth==0,
            "stored depth changes before its element leaves the old layer");
        world->step({},false);
        const auto* after=layers.lifetimes().resolve(key);
        require(world->effectsSucceeded()&&after&&after->depth.layerId!=oldLayer&&
            layers.layers().get(layers.layers().find(after->depth.layerId))->depth==-60000,
            "normal phase service migrates the impact to its requested depth");
        require(effects.clear(registration),"retire migrated effect through its live owner");
    }
    {
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach mixed shootable selector world");world->skipToGameForTesting();
        InstanceHandle shot;
        for(unsigned tick=0;tick<1000&&!shot.generation;++tick){
            world->step(gameplayContactProfileInput(tick,world->mainPlayerX(),world->mainPlayerY()),false);
            if(world->mainBulletCount())shot=world->mainBulletAt(0).instance;
        }
        require(shot.generation&&world->effectsSucceeded(),"natural fired lifetime for parent-90 query");
        RunnerImageValue x,y;require(x.assignInteger(240)&&y.assignInteger(512)&&world->publishBulletCollisionPosition(shot,x,y),
            "publish isolated shootable query origin");
        auto bones=world->boneRegistration<12>();auto corpses=world->corpseRegistration<1>();
        std::vector<InstanceHandle> children;InstanceHandle corpse;
        auto query=[&]{InstancePlaceResult result;const auto draws=world->randomDrawCount();
            require(owner->instancePlace(shot,*world->resolveInstance(shot),90,x.promoted(),y.promoted(),result)&&
                draws==world->randomDrawCount(),"parent-90 identity query uses real resource shapes without RNG");return result;
        };
        require(query().id==-4,"empty registered shootable population");
        for(unsigned i=0;i<12;++i){InstanceHandle bone;
            require(world->spawnBone(bones,240,512,bone),"register shootable bone");children.push_back(bone);
            if(!i){
                require(query().id==static_cast<std::int32_t>(world->resolveInstance(bones.worldHandle(bone))->instanceId),"single bone selected by numeric identity");
                DeadBodySource source{x.promoted(),y.promoted(),{},MotionBinary64::fromInteger(-1)};
                require(world->createRegisteredCorpseFromDeath(corpses,source,206,{},102,
                    [](auto,int,auto,auto,bool& hit){hit=false;return true;}),"interleave corpse in the shootable family");
                corpse=corpses.sourceHandle(source.myDeadBody);
            }
            const auto selected=query();const auto* record=world->resolveInstance(selected.handle);
            require(record&&(record->objectId==269||record->objectId==263),"mixed family returns a retained bone or corpse identity");
        }
        bool hit=false;require(owner->placeMeeting(shot,*world->resolveInstance(shot),90,x.promoted(),y.promoted(),hit)&&hit,
            "source boolean shootable query sees overlap");
        for(auto child:children)require(bones.activate(child,false),"deactivate all bones between separate queries");
        auto selected=query();require(selected.handle.slot==corpse.slot&&selected.handle.generation==corpse.generation,
            "fresh identity query sees surviving corpse, not a cached bone");
        auto targets=storedShootableTargets(*owner,bones,corpses);
        MotionBinary64 hp;bool effect=true;const auto beforeWrite=world->randomDrawCount();
        require(targets.readShootable(selected.handle,hp,effect)&&hp.zero()&&!effect,
            "selected ordinary corpse exposes zero health and disabled hit effects");
        require(targets.writeShootableHealth(selected.handle,{0xc0040000u,0})&&targets.writeShootableHit(selected.handle)&&
            targets.readShootable(selected.handle,hp,effect)&&hp.high==0xc0040000u&&!hp.low&&corpses.state(corpse)->hit&&
            !world->resolveInstance(corpse)->destroyRequested&&world->randomDrawCount()==beforeWrite,
            "selected corpse real-field writes preserve membership, lifetime and RNG");
        require(bones.activate(children.front(),true)&&bones.mark(children.front()),"retain marked bone beside corpse");
        selected=query();require(selected.handle.slot==corpse.slot&&selected.handle.generation==corpse.generation,
            "marked bone is not selected");
        require(bones.clear()&&corpses.clear()&&query().id==-4,"retired shootables leave no selectable stale identity");
        require(!targets.readShootable(corpse,hp,effect)&&!targets.writeShootableHit(shot)&&world->effectsSucceeded(),
            "stale and unrelated identities never dispatch into either actor registration");
        require(x.assignInteger(1000)&&query().id==-4,"offscreen temporary query misses");
        RunnerImageValue actualX,actualY;
        require(owner->collisionPosition(shot,actualX,actualY)&&actualX.bits==0x43700000u&&actualY.bits==0x44000000u,
            "place query restores exact published bullet position");
    }
    {
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach bullet End Step world");world->skipToGameForTesting();
        InstanceHandle shot;
        for(unsigned tick=0;tick<1000&&!shot.generation;++tick){
            require(world->stepWithCollisionPass(gameplayContactProfileInput(tick,world->mainPlayerX(),world->mainPlayerY()),false,[&]{
                for(unsigned i=0;i<world->mainBulletCount();++i){
                    const auto key=world->mainBulletAt(i).instance;const auto* r=owner->resolve(key);
                    if(!r||r->destroyRequested)continue;
                    RunnerImageValue x,y;require(x.assignInteger(240)&&y.assignInteger(10000),"bullet test position");
                    shot=key;return world->publishBulletCollisionPosition(key,x,y)&&!owner->resolve(key)->destroyRequested;
                }
                return true;
            }),"live bullet survives collision publication until End Step");
        }
        require(shot.generation&&!owner->resolve(shot),"bullet End Step observes collision-published Y and cleanup retires it");
        for(unsigned i=0;i<world->mainBulletCount();++i)
            require(owner->resolve(world->mainBulletAt(i).instance),"bullet draw cache contains no End Step retirees");
    }
    {
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach scheduled source-bullet world");world->skipToGameForTesting();
        unsigned calls=0,initializations=0;
        auto frames=[&](int id,unsigned& count){return sprites.frameCount(id,count);};
        for(unsigned tick=0;tick<1000&&calls<3;++tick){
            const auto input=calls?Input{}:gameplayContactProfileInput(tick,world->mainPlayerX(),world->mainPlayerY());
            const auto previous=calls;
            require(world->withSourceBulletSteps([&](InstanceHandle key,MachineGunSourceMotion& motion){
                ++calls;const bool first=!motion.initialized;initializations+=first;
                const auto before=world->randomDrawCount();ScheduledEmptyBulletOps<StartupRuntime> ops{*world,key};
                return world->stepSourceBullet(key,motion,ops,frames)&&ops.writes==2&&world->randomDrawCount()==before+unsigned(first);
            },[&]{world->step(input,false);return world->effectsSucceeded();}),"source bullet dispatch occupies ordinary Step");
            require(calls<=previous+1,"one selected source bullet Step per tick");
        }
        require(calls==3&&initializations==1&&world->mainBulletCount()==1&&world->mainBulletAt(0).sourceOwned&&
            !world->mainBulletAt(0).trajectory.initialized,"source motion survives frames without legacy advancement");
        const auto& bullet=world->mainBulletAt(0);const auto& motion=bullet.sourceMotion;
        MotionBinary64 vx,vy;require(world->sourceBulletVelocity(bullet.instance,vx,vy)&&
            vx.high==motion.xsp.high&&vx.low==motion.xsp.low&&vy.high==motion.ysp.high&&vy.low==motion.ysp.low,
            "source velocity reader resolves exact retained shot fields");
        auto shards=world->limboShardRegistration<1>();InstanceHandle shard;
        require(world->createRegisteredLimboShard(shards,motion.x.promoted(),motion.y.promoted(),false,shard),
            "create shard beside scheduled source bullet");
        RunnerCollisionEventContext contact;const auto contactDraws=world->randomDrawCount();
        require(owner->dispatchCollisionEvent(contact,shard,bullet.instance,{true,false},[&](const auto& event){
            return world->dispatchLimboShardBulletCollision(shards,event);
        }),"bound shard collision reads retained source bullet, without external velocity provider");
        require(shards.state(shard)->noTouch&&shards.state(shard)->ysp.high==vy.high&&shards.state(shard)->ysp.low==vy.low&&
            world->randomDrawCount()==contactDraws+1,"source ysp passes unchanged into shard contact");
        auto stale=bullet.instance;++stale.generation;
        require(!world->sourceBulletVelocity(stale,vx,vy)&&vy.high==motion.ysp.high&&vy.low==motion.ysp.low,
            "stale velocity lookup preserves caller outputs");
        require(shards.clear(),"retire source-contact shard");
        const auto before=world->randomDrawCount();world->step({},false);
        require(!world->effectsSucceeded()&&world->randomDrawCount()==before,"missing source dispatch fails before any tick mutation");
    }
    for(unsigned contactCase=0;contactCase<3;++contactCase){
        const bool corpseTarget=contactCase!=0,failSound=contactCase==2;
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach concrete source bullet adapter");world->skipToGameForTesting();
        auto bones=world->boneRegistration<4>();auto corpses=world->corpseRegistration<1>();
        auto targets=storedShootableTargets(*owner,bones,corpses);InstanceHandle bone,shot;unsigned calls=0;
        auto frames=[&](int id,unsigned& count){return sprites.frameCount(id,count);};
        auto corpseEvent=[&](auto,auto phase,unsigned,auto& state,auto& publish,bool& good){
            require(phase==StartupRuntime::CorpsePhase::step,"new corpse cannot fire an alarm in its birth tick");
            struct Environment {
                int areaValue;
                int area(){return areaValue;}int restInPieces(){return 0;}
                bool create(int,MotionBinary64,MotionBinary64){return false;}
            } environment{world->runStage().area};
            return state.stepInWorld(false,480,publish,environment,good);
        };
        for(unsigned tick=0;tick<1000&&!calls;++tick){
            require(world->withSourceBulletSteps([&](InstanceHandle key,MachineGunSourceMotion& motion){
                shot=key;++calls;const int y=int(world->mainPlayerY());RunnerImageValue px,py;
                require(px.assignInteger(240)&&py.assignInteger(y)&&world->spawnBone(bones,corpseTarget?1000:240,y,bone)&&
                    world->publishBulletCollisionPosition(key,px,py),
                    "prepare explicit registered bone at source bullet position");
                if(corpseTarget){
                    DeadBodySource source{px.promoted(),py.promoted(),{},MotionBinary64::fromInteger(-1)};
                    require(world->createRegisteredCorpseFromDeath(corpses,source,206,{},102,
                        [](auto,int,auto,auto,bool& hit){hit=false;return true;}),"create zero-health corpse in bullet shape");
                }
                SoundPlayEnvironment environment;
                BulletWallAudio audio{world->resolveInstance(key)->sound,world->overlapSound(2),29,false};
                const auto before=world->randomDrawCount();
                require(StoredSourceBulletStep::run(*world,*owner,key,motion,targets,frames,environment,audio),
                    "scheduled source event uses concrete stored collision adapter");
                const auto* victim=bones.instances().find(bone);
                require(victim&&victim->state.hit==!corpseTarget&&victim->state.health.high==(corpseTarget?0x3ff00000u:0xc0200000u)&&
                    world->randomDrawCount()==before+1&&audio.calls==0&&world->scriptEffects().size()==1&&
                    owner->resolve(key)->destroyRequested==!corpseTarget&&motion.initialized,
                    "scheduled bone impact retains source damage, muzzle and destruction timing");
                return true;
            },[&]{return world->withCorpseEvents(corpses,corpseEvent,[&]{
                world->step(gameplayContactProfileInput(tick,world->mainPlayerX(),world->mainPlayerY()),false);
                return world->effectsSucceeded();});}),"source adapter completes ordinary world tick");
        }
        require(calls==1&&(corpseTarget?(owner->resolve(shot)&&!owner->resolve(shot)->destroyRequested):!owner->resolve(shot)),
            "bone impact retires the shot, but zero-health corpse leaves it moving");
        if(corpseTarget){
            auto pass=std::make_unique<StoredDitherCollisionPass>(*owner);
            require(pass->run(*world),"settle dither before new contact participants enter the queue");
            auto skeletons=world->skeletonRegistration<1>();InstanceHandle target;
            const auto* bullet=world->retainedSourceBullet(shot);std::int32_t x,y;
            require(bullet&&bullet->sourceMotion.x.promoted().roundPixel(x)&&bullet->sourceMotion.y.promoted().roundPixel(y)&&
                world->spawnSkeleton(skeletons,x,y,target),"register skeleton beside retained scheduled bullet");
            struct HitAudio {
                bool fail;unsigned calls=0;
                bool isPlaying(int id,bool& playing){require(++calls==1&&id==29,"damage sound lookup");playing=false;return true;}
                bool stop(int){return false;}
                bool play(int id,int priority,bool loop,int& voice){require(++calls==2&&id==29&&priority==90&&!loop,"damage sound start");voice=123;return !fail;}
                bool getGain(int id,MotionBinary64& gain){require(++calls==3&&id==123,"damage sound gain read");gain=MotionBinary64::fromInteger(1);return true;}
                bool setGain(int id,MotionBinary64 gain,int duration){require(++calls==4&&id==123&&gain.high==0x3ff00000u&&!gain.low&&!duration,"damage sound gain write");return true;}
                bool setPitch(int id,MotionBinary64 pitch){require(++calls==5&&id==123&&pitch.high==0x3ff00000u&&!pitch.low,"damage sound pitch");return true;}
            } audio{failSound};
            SoundPlayEnvironment environment;
            const auto effects=world->scriptEffects().size();
            auto smoke=world->smokeRegistration<1>();auto particles=world->consumptionBloodRegistration<1>();
            const InstanceRecord* wall=nullptr;
            for(unsigned i=0;i<world->activeInstanceCount();++i){const auto& r=world->instanceAt(i);
                if(r.objectId==60&&!r.destroyRequested){wall=&r;break;}}
            require(wall,"live wall for simultaneous damage/effect contacts");
            const int wx=wall->x,wy=wall->y;std::uint32_t smokeId=0,particleId=0;
            require(world->createRegisteredSmoke(smoke,MotionBinary64::fromInteger(wx),MotionBinary64::fromInteger(wy),smokeId),
                "create smoke beside retained wall during bullet contact fixture");
            const auto* smokeRecord=world->instance(smokeId);
            require(smokeRecord&&smoke.alarmEvent(*smokeRecord,0),"enable registered smoke contact");
            struct ParticleGlobals {
                MotionBinary64 grav{};
                bool gravity(MotionBinary64 value){grav=value;return true;}
                MotionBinary64 gravity(){return grav;}MotionBinary64 fallSpeed(){return {};}
            } globals;
            require(world->createRegisteredConsumptionBlood(particles,MotionBinary64::fromInteger(wx+4),
                MotionBinary64::fromInteger(wy+4),globals,particleId),"create registered particle on retained wall");
            auto expectedSmoke=*smoke.state(*smokeRecord);unsigned smokeContacts=0;
            InstanceHandle smokeKey{};
            for(unsigned i=0;i<world->activeInstanceCount();++i)
                if(world->instanceAt(i).instanceId==smokeId)smokeKey=world->instanceHandleAt(i);
            for(unsigned i=0;i<world->activeInstanceCount();++i){const auto& r=world->instanceAt(i);
                if(r.objectId!=60||r.destroyRequested)continue;
                bool overlap=false;require(owner->collisionOverlap(smokeKey,world->instanceHandleAt(i),overlap),"count actual smoke overlaps");
                if(overlap){++smokeContacts;require(expectedSmoke.collideWall(),"source smoke response per wall");}}
            require(smokeContacts,"mixed pass has overlapping smoke");
            const auto contactDraws=world->randomDrawCount();
            int roomSpeed=60;
            const bool hitOkay=world->withMomentDelayRoomSpeed(roomSpeed,[&]{
                return pass->runSkeletonContacts(*world,skeletons,smoke,particles,frames,environment,audio,
                    [](auto dy,auto dx,auto& angle){return portableAtan(dy,dx,angle);});
            });
            require(hitOkay==!failSound,"stored enemy collision dispatch preserves backend failure");
            unsigned controllers=0;
            for(std::size_t i=0;i<world->activeInstanceCount();++i){const auto& record=world->instanceAt(i);
                if(record.objectId!=37||!record.active||record.destroyRequested)continue;
                ++controllers;require(record.controllerSlow==30&&record.alarms.get(0)==1,"damage delay reaches real controller alarm state");
            }
            require(controllers&&roomSpeed==30,"damage delay uses bound source room speed and persistent controller");
            const auto* victim=skeletons.instances().find(target);const auto* caller=owner->resolve(shot);
            if(failSound){
                require(!world->effectsSucceeded()&&victim&&victim->state.health==51&&victim->state.hitDamage==9&&
                    victim->state.damageStun&&caller&&caller->destroyRequested&&audio.calls==2&&
                    world->scriptEffects().size()==effects&&world->randomDrawCount()==contactDraws,
                    "failed audio retains damage/destruction prefix and prevents effect and second delay");
                continue; // Terminal failed simulation: never retry or resume this event.
            }
            require(victim&&victim->state.health==51&&victim->state.hitDamage==9&&victim->state.hitPending&&
                victim->state.damageStun&&audio.calls==5&&caller&&caller->destroyRequested&&!caller->bulletHitIgnore&&
                caller->bulletMyFx&&world->scriptEffects().size()==effects+1&&world->randomDrawCount()==contactDraws,
                "stored skeleton damage preserves health, delays, sound, effect ownership and RNG");
            require(owner->resolve(skeletons.worldHandle(target))->health==51,"publish damaged skeleton health to stored instance");
            const auto* actualSmoke=smoke.state(*smokeRecord);const auto* particle=world->instance(particleId);
            require(actualSmoke&&actualSmoke->speed.high==expectedSmoke.speed.high&&actualSmoke->speed.low==expectedSmoke.speed.low&&
                particle&&particle->destroyRequested&&particles.state(*particle)->destroyRequested,
                "one combined stream applies every smoke wall response and marks particle beside skeleton damage");
            require(smoke.clear()&&particles.clear(),"clear registered effects after mixed contact verification");
            require(skeletons.clear(),"clear explicitly registered skeleton contact target");
        }
        require(bones.clear()&&corpses.clear(),"clear explicitly registered adapter targets");
    }
    for(bool high:{false,true})for(bool laser:{false,true})for(bool boneTarget:{false,true})for(bool worldRandom:{false,true}){
        if(worldRandom&&(high||laser))continue;
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach source bullet Step world");world->skipToGameForTesting();
        InstanceHandle box{},shot{};
        for(unsigned tick=0;tick<1000;++tick){
            world->step(gameplayContactProfileInput(tick,world->mainPlayerX(),world->mainPlayerY()),false);
            for(std::size_t i=0;i<world->activeInstanceCount();++i){
                const auto& r=world->instanceAt(i);
                if(r.objectId==92&&r.active&&!r.destroyRequested)box=world->instanceHandleAt(i);
            }
            if(box.generation&&world->mainBulletCount()){shot=world->mainBulletAt(0).instance;break;}
        }
        require(world->effectsSucceeded()&&box.generation&&shot.generation,"natural generation provides live box and shot identities");
        // Isolate the event geometrically, without bypassing either owner's
        // setters. Do not resume the normal simulation after this fixture.
        auto real=[](int n){RunnerImageValue value;require(value.assign(MotionBinary64::fromInteger(n)),"fixture coordinate");return value;};
        for(std::size_t i=0;i<world->activeInstanceCount();++i){
            const auto key=world->instanceHandleAt(i);
            require(world->publishCollisionPosition(key,real(10000+int(i)*32),real(10000)),"isolate collision geometry");
        }
        if(!boneTarget)require(world->publishCollisionPosition(box,real(240),real(512)),"position isolated box");
        auto bones=world->boneRegistration<4>();InstanceHandle bone;
        const int shotY=boneTarget?int(world->mainPlayerY()):500;
        if(boneTarget)require(world->spawnBone(bones,240,shotY,bone),"create registered shootable bone");
        MachineGunSourceMotion motion;GameMakerRandom random(2);
        require(motion.reset(MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(shotY),laser)&&
            world->publishBulletCollisionPosition(shot,motion.x,motion.y),"publish source bullet starting position");
        bool succeeded=true;
        auto frames=[&](int id,unsigned& count){PackedSpriteCollision art;
            const auto* caller=owner->resolve(shot);
            require(caller&&caller->bulletMyFx&&world->scriptEffects().size()==1,
                "actual caller stores the source child ID before the sprite setter");
            if(!sprites.sprite(id,art))return false;count=art.frameCount;return true;};
        struct Ops {
            StartupRuntime& world;StoredWorldCollision& owner;MachineGunSourceMotion& motion;InstanceHandle shot;
            StoredBoneRegistration<StartupRuntime::instanceCapacity,4>& bones;
            InstanceHandle selected{};unsigned lines=0,points=0,approachWrites=0,writes=0;
            bool approaching=false,destroyed=false;
            bool publishAngle(RunnerImageValue angle){return world.publishBulletAngle(shot,angle);}
            bool publishPosition(RunnerImageValue x,RunnerImageValue y){
                if(approaching)++approachWrites;else {require(destroyed,"final movement follows destruction request");++writes;}
                RunnerImageValue actualX,actualY;
                return world.publishBulletCollisionPosition(shot,x,y)&&owner.collisionPosition(shot,actualX,actualY)&&
                    actualX.bits==x.bits&&actualY.bits==y.bits;
            }
            bool query(int object,InstancePlaceResult& result){MotionBinary64 x,y;++lines;
                return motion.endpoint(x,y)&&owner.nonPreciseLine(shot,object,motion.x.promoted(),motion.y.promoted(),x,y,false,result);}
            bool line(int object,bool& hit){InstancePlaceResult result;if(!query(object,result))return false;hit=result.id!=-4;return true;}
            bool selectWall(){InstancePlaceResult result;if(!query(59,result)||result.id==-4)return false;selected=result.handle;return true;}
            bool approachSolid(){approaching=true;unsigned count=0;
                const bool ok=motion.approachSolid([&](auto x,auto y,bool& hit){++points;InstancePlaceResult result;
                    if(!owner.collisionPoint(shot,59,x,y,false,false,result))return false;hit=result.id!=-4;return true;
                },[&](auto x,auto y){return publishPosition(x,y);},count);approaching=false;return ok;}
            bool wallParent(int& p){return owner.projectileWallParent(selected,p);}
            bool wallHealth(MotionBinary64& hp){return owner.projectileBoxHealth(selected,hp);}
            bool wallHealthWrite(MotionBinary64 hp){return owner.projectileBoxHealthWrite(world,selected,hp);}
            bool wallHit(){return owner.projectileBoxHit(world,selected);}
            bool destroySelf(){destroyed=world.markBulletDestroyed(shot);return destroyed;}
            bool effect(int,MotionBinary64,MotionBinary64){return false;}
            bool soundWall(){return false;}bool explosion(int){return false;}
            bool placeShootable(int object,bool& hit){const auto* r=owner.resolve(shot);return r&&
                owner.placeMeeting(shot,*r,object,motion.x.promoted(),motion.y.promoted(),hit);}
            bool selectShootable(int object){const auto* r=owner.resolve(shot);InstancePlaceResult result;
                if(!r||!owner.instancePlace(shot,*r,object,motion.x.promoted(),motion.y.promoted(),result)||result.id==-4)return false;
                selected=result.handle;return true;}
            bool thingHealth(MotionBinary64& hp){bool effect;return bones.readShootable(selected,hp,effect);}
            bool thingHealthWrite(MotionBinary64 hp){return bones.writeShootableHealth(selected,hp);}
            bool thingHit(){return bones.writeShootableHit(selected);}
            bool thingHitEffect(bool& effect){MotionBinary64 hp;return bones.readShootable(selected,hp,effect);}
        } ops{*world,*owner,motion,shot,bones};
        const auto drawsBefore=world->randomDrawCount();auto expectedRandom=world->randomState();auto expectedMotion=motion;
        if(worldRandom){
            motion.zeroSpread=true;
            require(expectedMotion.initialize(expectedRandom)&&world->stepSourceBullet(shot,motion,ops,frames),
                "source event uses retained birth profile and world RNG");
            require(!motion.zeroSpread&&world->randomDrawCount()==drawsBefore+1&&world->randomState().stateIndex()==expectedRandom.stateIndex()&&
                motion.direction.high==expectedMotion.direction.high&&motion.direction.low==expectedMotion.direction.low,
                "world source spread matches the exact pre-event RNG state");
            for(unsigned i=0;i<16;++i)require(world->randomState().stateWord(i)==expectedRandom.stateWord(i),"world source RNG words match");
        }else require(world->withBulletEffects(shot,frames,[&](auto& muzzle){
            return MachineGunSourceStep::run(motion,random,high,muzzle,ops,succeeded);
        })&&succeeded,
            "source Step runs through stored collision, transform, effect and damage owners");
        const auto expected=MotionBinary64::fromInteger(1-machineGunProfile(high).damage);
        if(boneTarget){
            const auto* victim=bones.instances().find(bone);
            require(victim&&victim->state.hit&&!victim->destroyRequested&&victim->state.health.high==expected.high&&
                victim->state.health.low==expected.low&&ops.lines==1&&ops.writes==2&&ops.destroyed&&random.drawCount()==(worldRandom?0u:1u),
                "source bullet selects, damages and retains the bone while destroying only itself");
            auto effectFrames=[&](int id,unsigned& count){return sprites.frameCount(id,count);};
            const auto draws=world->randomDrawCount();
            auto step=[&](bool stopped){return bones.dispatch(bone,[&](auto& state,auto& publisher,bool& good){
                return state.stepInWorld(stopped,publisher,[&]{return world->spawnBoneImpact(bones,bone,effectFrames);},
                    [&](bool& visible){return world->boneInView(bones,bone,visible);},good);
            });};
            require(step(true)&&!bones.instances().find(bone)->destroyRequested&&world->scriptEffects().size()==1,
                "time-stop postpones shot bone's death without replacing the muzzle pool");
            require(step(false)&&bones.instances().find(bone)->destroyRequested&&world->scriptEffects().size()==2&&
                world->scriptEffects().atNewest(0)->spriteId==85&&world->randomDrawCount()==draws,
                "resumed bone Step shares the world effect pool and consumes no RNG");
            require(bones.clear(),"retire retained shot bone");continue;
        }
        const auto* target=owner->resolve(box);
        require(ops.selected.slot==box.slot&&ops.selected.generation==box.generation&&target&&target->box.hit&&
            target->box.wallHp.high==expected.high&&target->box.wallHp.low==expected.low&&target->active&&!target->destroyRequested,
            "source bullet damage leaves the selected box alive for its own Step");
        require(ops.lines==3&&ops.points>1&&ops.approachWrites==2*(ops.points-1)&&ops.writes==2&&
            random.drawCount()==(worldRandom?0u:1u)&&world->scriptEffects().size()==1&&
            owner->resolve(shot)->bulletMyFx&&owner->resolve(shot)->destroyRequested,
            "source query/movement counts and synchronous muzzle birth survive destruction marking");
        {
            const auto before=world->randomDrawCount();const int previous=world->overlapSound(2);
            SoundPlayEnvironment env;env.gameVolume={0x3fd00000u,0};env.inWater=true;
            BulletWallAudio audio{world->resolveInstance(shot)->sound,previous,29,true};
            require(world->playBulletWallSound(shot,env,[&](const char*& material){
                return owner->projectileWallMaterial(box,material);
            },audio)&&audio.valid&&audio.calls==7&&world->randomDrawCount()==before+2&&world->overlapSound(2)==29,
                "retained bullet sound fields and shared RNG/overlap channel use the real selected material");
        }
        if(!worldRandom)std::printf("SOURCE_SHOT %u %u %08x %08x %08x %08x %08x %u %u %u %u\n",
            unsigned(high),unsigned(laser),unsigned(motion.x.bits),unsigned(motion.y.bits),unsigned(motion.imageAngle.bits),
            unsigned(target->box.wallHp.high),unsigned(target->box.wallHp.low),ops.lines,ops.points,ops.approachWrites,ops.writes);
        if(worldRandom){const auto before=world->randomDrawCount();
            require(!world->stepSourceBullet({},motion,ops,frames)&&!world->effectsSucceeded()&&world->randomDrawCount()==before,
                "invalid projectile identity fails before source RNG or effects");}
    }
    {
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach natural box alarm world");world->skipToGameForTesting();
        InstanceHandle box;
        for(unsigned tick=0;tick<1000&&!box.generation;++tick){
            for(unsigned i=0;i<world->activeInstanceCount();++i)
                if(world->instanceAt(i).objectId==92&&world->instanceAt(i).active){box=world->instanceHandleAt(i);break;}
            if(!box.generation)world->step(gameplayContactProfileInput(tick,world->mainPlayerX(),world->mainPlayerY()),false);
        }
        require(box.generation&&world->publishBoxAlarm0(box,1),"arm naturally generated box");
        bool retained=false;
        require(world->stepWithCollisionPass({},false,[&]{
            const auto* r=owner->resolve(box);
            retained=r&&r->active&&r->destroyRequested&&r->box.wallHp.zero()&&r->alarms.get(0)==0;
            return retained;
        })&&retained&&!owner->resolve(box),"Alarm expiry reaches box Step, then collision retention and pre-Draw cleanup");
    }
    {
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach box field world");world->skipToGameForTesting();
        InstanceHandle selected{};
        for(unsigned tick=0;tick<600&&!selected.generation;++tick){
            for(unsigned i=0;i<world->activeInstanceCount();++i)
                if(world->instanceAt(i).objectId==92&&world->instanceAt(i).active){selected=world->instanceHandleAt(i);break;}
            if(!selected.generation)world->step(gameplayContactProfileInput(tick,world->mainPlayerX(),world->mainPlayerY()),false);
        }
        require(selected.generation&&world->effectsSucceeded(),"natural generation supplies a box");
        const auto* candidate=owner->resolve(selected);InstancePlaceResult line;
        const auto x=MotionBinary64::fromInteger(candidate->x),y=MotionBinary64::fromInteger(candidate->y);
        require(owner->nonPreciseLine({},92,x,y,x,y,false,line)&&line.id>=100000&&
            line.handle.slot==selected.slot&&line.handle.generation==selected.generation,
            "box field writes use the source line selector's retained identity");
        selected=line.handle;
        int parent=-123;MotionBinary64 hp;
        require(owner->projectileWallParent(selected,parent)&&parent==84&&owner->projectileBoxHealth(selected,hp)&&
            hp.high==0x3ff00000u&&!hp.low,"box target resolves direct parent and Create health");
        RunnerImageValue distantX,distantY;
        require(distantX.assign(MotionBinary64::fromInteger(1000))&&
            distantY.assign(MotionBinary64::fromInteger(world->mainPlayerY()+128)),"encode isolated box position");
        require(world->publishCollisionPosition(selected,distantX,distantY)&&world->publishBoxAlarm0(selected,2),
            "arm retained box outside player contacts");
        world->step({},false);
        require(world->effectsSucceeded()&&owner->resolve(selected)->alarms.get(0)==1&&
            owner->resolve(selected)->box.wallHp.high==0x3ff00000u,"box alarm counts down without early damage");
        // Test an already-fired alarm independently of its destruction Step.
        require(world->publishBoxAlarm0(selected,0)&&world->publishBoxHealth(selected,MotionBinary64::fromInteger(3)),
            "prepare retained box with an already-fired alarm");
        world->step({},false);
        require(world->effectsSucceeded()&&owner->resolve(selected)->alarms.get(0)==-1&&
            owner->resolve(selected)->box.wallHp.high==0x40080000u,"zero alarm retires without re-firing");
        require(world->publishBoxAlarm0(selected,-2),"negative box alarm is inactive");world->step({},false);
        require(world->effectsSucceeded()&&owner->resolve(selected)->alarms.get(0)==-2,"negative box alarm stays untouched");
        RunnerImageValue insideX,insideY;
        require(insideX.assign(MotionBinary64::fromInteger(240))&&
            insideY.assign(MotionBinary64::fromInteger(20000)),"encode time-bound query fixture");
        require(world->publishCollisionPosition(selected,insideX,insideY)&&world->publishBoxAlarm0(selected,1),
            "arm time-bound box away from fields without running retirement");
        const auto stopDraws=world->randomDrawCount();
        require(owner->stopBoxAlarm0(*world,selected,true,480)&&owner->resolve(selected)->alarms.get(0)==2&&
            owner->resolve(selected)->box.scriptIndex==1&&world->randomDrawCount()==stopDraws,
            "frozen box refunds positive alarm without RNG");
        require(owner->stopBoxAlarm0(*world,selected,false,480)&&owner->resolve(selected)->alarms.get(0)==2,
            "running player time leaves box alarm untouched");
        for(int value:{-2,-1,0}){
            require(world->publishBoxAlarm0(selected,value)&&owner->stopBoxAlarm0(*world,selected,true,480)&&
                owner->resolve(selected)->alarms.get(0)==value,"frozen box does not revive fired or inactive alarms");
        }
        for(int quarter:{639,640,1280,1281}){
            auto q=MotionBinary64::fromInteger(quarter);q.high-=2u<<20;RunnerImageValue px;require(px.assign(q),"encode fractional side");
            require(world->publishCollisionPosition(selected,px,insideY)&&world->publishBoxAlarm0(selected,1)&&
                owner->stopBoxAlarm0(*world,selected,true,480)&&
                owner->resolve(selected)->alarms.get(0)==(quarter>=640&&quarter<=1280?2:1),
                "alarmStop uses fractional published coordinates at strict side boundaries");
        }
        const auto draws=world->randomDrawCount();const auto* box=owner->resolve(selected);
        const auto active=box->active,destroyed=box->destroyRequested;
        const MotionBinary64 fractional{0xbfe00000u,0};
        require(owner->projectileBoxHealthWrite(*world,selected,fractional)&&owner->projectileBoxHit(*world,selected)&&
            owner->projectileBoxHealth(selected,hp)&&hp.high==fractional.high&&!hp.low&&
            owner->resolve(selected)->box.hit&&owner->resolve(selected)->active==active&&
            owner->resolve(selected)->destroyRequested==destroyed&&world->randomDrawCount()==draws,
            "health/hit writes retain fractional overkill, membership and RNG");
        const auto selectedId=owner->resolve(selected)->instanceId;
        const auto destructionDraws=world->randomDrawCount();
        require(world->markBoxDestroyed(selected)&&world->markBoxDestroyed(selected)&&
            owner->resolve(selected)&&owner->resolve(selected)->active&&owner->resolve(selected)->destroyRequested&&
            world->randomDrawCount()==destructionDraws,"box destruction marks once and retains the current event lifetime");
        require(world->publishCollisionPosition(selected,insideX,insideY)&&world->publishBoxAlarm0(selected,1)&&
            owner->stopBoxAlarm0(*world,selected,true,480)&&owner->resolve(selected)->alarms.get(0)==2,
            "marked box still executes alarmStop but no longer contacts time fields");
        InstancePlaceResult afterMark;
        require(owner->nonPreciseLine({},92,insideX.promoted(),insideY.promoted(),insideX.promoted(),insideY.promoted(),false,afterMark)&&
            afterMark.id==-4,"marked box immediately leaves collision selection");
        bool retainedAtCollision=false;
        BoxDestructionScript::Caller caller;
        require(owner->readBoxDestruction(selected,caller)&&caller.state.wallHp.high==fractional.high&&
            caller.x.high==insideX.promoted().high&&caller.y.high==insideY.promoted().high,
            "destruction reader retains marked caller fields and published coordinates");
        require(world->stepWithCollisionPass({},false,[&]{
            const auto* retained=owner->resolve(selected);
            retainedAtCollision=retained&&retained->destroyRequested;return retainedAtCollision;
        })&&retainedAtCollision,"marked box storage survives through the collision boundary");
        require(world->effectsSucceeded()&&!world->instance(selectedId)&&!owner->resolve(selected)&&
            !owner->readBoxDestruction(selected,caller)&&caller.state.wallHp.high==fractional.high,
            "stored frame retires marked box at its pre-Draw cleanup boundary");
        auto wrong=selected;++wrong.generation;hp={0x3ff00000u,0};parent=-123;
        require(!owner->projectileBoxHealth(wrong,hp)&&hp.high==0x3ff00000u&&
            !owner->projectileWallParent(wrong,parent)&&parent==-123&&
            !owner->projectileBoxHealthWrite(*world,wrong,{})&&!owner->projectileBoxHit(*world,wrong),
            "stale generation cannot read or write box fields");
        require(world->restartFromResults(DeathMenuSequence::Action::Retry,43)&&!owner->resolve(selected)&&
            !owner->projectileBoxHealthWrite(*world,selected,fractional),"Retry invalidates retained box selection");
    }
    {
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach box effect caller world");world->skipToGameForTesting();
        InstanceHandle box{};
        for(unsigned tick=0;tick<600&&!box.generation;++tick){
            for(unsigned i=0;i<world->activeInstanceCount();++i)
                if(world->instanceAt(i).objectId==92&&world->instanceAt(i).active){box=world->instanceHandleAt(i);break;}
            if(!box.generation)world->step(gameplayContactProfileInput(tick,world->mainPlayerX(),world->mainPlayerY()),false);
        }
        require(box.generation&&world->effectsSucceeded(),"find box effect caller");
        unsigned neighbourMask=0;
        require(world->stepWithCollisionPass({},false,[&]{
            auto* r=const_cast<InstanceRecord*>(owner->resolve(box));
            require(r&&r->active&&!r->destroyRequested,"autotile caller remains live at Collision");
            const int dx[]={0,16,0,-16},dy[]={-16,0,16,0};
            for(unsigned i=0;i<4;++i){
                bool hit=false;
                require(owner->placeMeeting(box,*r,92,MotionBinary64::fromInteger(r->x+dx[i]),
                    MotionBinary64::fromInteger(r->y+dy[i]),hit),"resource-backed autotile neighbour query");
                if(hit)neighbourMask|=1u<<i;
            }
            r->box.autoTile=true;r->box.checked=false;return true;
        }),"dispatch box End Step against maintained collision ownership");
        const auto* tiled=owner->resolve(box);
        require(tiled&&tiled->box.checked&&!tiled->box.materialBreakable&&
            unsigned(tiled->box.scriptIndex)==neighbourMask&&tiled->imageIndex==float(neighbourMask),
            "live box End Step publishes the collision-derived neighbour image");
        InstanceHandle offscreen{};
        require(world->stepWithCollisionPass({},false,[&]{
            for(unsigned i=0;i<world->activeInstanceCount();++i){
                const auto& r=world->instanceAt(i);
                if(r.objectId!=92||r.instanceId==owner->resolve(box)->instanceId||!r.active||r.destroyRequested)continue;
                offscreen=world->instanceHandleAt(i);break;
            }
            require(offscreen.generation,"second box for post-destruction neighbour queries");
            RunnerImageValue x,y;require(x.assignInteger(240)&&y.assignInteger(-10000),"offscreen coordinates");
            require(world->publishCollisionPosition(offscreen,x,y),"publish offscreen box through spatial ownership");
            auto* r=const_cast<InstanceRecord*>(owner->resolve(offscreen));
            r->box.autoTile=true;r->box.checked=false;return true;
        })&&!owner->resolve(offscreen),
            "maintained queries/image publication accept a marked box through the remainder of End Step");
        unsigned lookups=0,observed=0;bool accept=true;
        auto frames=[&](int sprite,unsigned& count){
            ++lookups;observed=owner->resolve(box)->boxMyFx;
            const auto* child=world->instance(observed);
            require(sprite==603&&child&&child->objectId==219&&child->spriteId==-1,
                "source child ID is stored on the caller before sprite validation");
            PackedSpriteCollision art;require(sprites.sprite(sprite,art),"impact sprite exists");count=art.frameCount;return accept;
        };
        auto x=MotionBinary64::fromInteger(240),y=MotionBinary64::fromInteger(524);
        require(x.add({0x3fd00000u,0})&&y.add({0x3fc00000u,0}),"fractional impact position");
        const auto draws=world->randomDrawCount();
        require(world->spawnBoxScriptEffect(box,x,y,603,{0x3fe00000u,0},MotionBinary64::fromInteger(270),
            MotionBinary64::fromInteger(-60000),frames),"publish owned box effect");
        const auto first=observed;const auto* child=world->scriptEffects().atNewest(0);
        require(child&&child->spriteId==603&&child->x.bits==0x43704000u&&child->y.bits==0x44030800u&&
            child->imageSpeed.bits==0x3f000000u&&child->angle.bits==0x43870000u&&world->randomDrawCount()==draws,
            "owned box effect retains source setters without extra RNG");
        accept=false;
        require(!world->spawnBoxScriptEffect(box,x,y,603,{0x3fe00000u,0},MotionBinary64::fromInteger(270),
            MotionBinary64::fromInteger(-60000),frames),"reject later sprite validation");
        const auto second=owner->resolve(box)->boxMyFx;
        require(!world->effectsSucceeded()&&second>first&&world->instance(first)&&world->instance(second)&&
            world->scriptEffects().size()==2&&lookups==2,"retain both child birth and caller assignment on failure");
        require(!world->spawnBoxScriptEffect(box,x,y,603,{},{},{},frames)&&lookups==2&&world->randomDrawCount()==draws,
            "failed box effect is never retried");
    }
    for(bool retire:{false,true}){
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach box alarm callback world");world->skipToGameForTesting();
        InstanceHandle box{};
        for(unsigned tick=0;tick<600&&!box.generation;++tick){
            for(unsigned i=0;i<world->activeInstanceCount();++i)
                if(world->instanceAt(i).objectId==92&&world->instanceAt(i).active){box=world->instanceHandleAt(i);break;}
            if(!box.generation)world->step(gameplayContactProfileInput(tick,world->mainPlayerX(),world->mainPlayerY()),false);
        }
        require(box.generation&&world->publishBoxAlarm0(box,5),"arm callback box");
        unsigned queries=0;
        require(!world->stopBoxAlarm0(box,[&](bool& allowed){
            ++queries;allowed=false;
            if(retire){
                require(world->restartFromResults(DeathMenuSequence::Action::Retry,43)&&!owner->resolve(box),
                    "callback retires old room caller");
            }else{
                require(!world->publishBoxAlarm0({box.slot,box.generation+1},17),"nested stale write fails");
            }
            return true;
        })&&!world->effectsSucceeded()&&queries==1,"alarm callback cannot revive failure or write a retired caller");
        if(!retire)require(owner->resolve(box)->alarms.get(0)==5,"nested failure preserves countdown");
        require(!world->stopBoxAlarm0(box,[&](bool&){++queries;return true;})&&queries==1,"failed callback is not retried");
    }
    for(unsigned failure=0;failure<=7;++failure){
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach box sound world");world->skipToGameForTesting();
        InstanceHandle box{};
        for(unsigned tick=0;tick<600&&!box.generation;++tick){
            for(unsigned i=0;i<world->activeInstanceCount();++i)
                if(world->instanceAt(i).objectId==92&&world->instanceAt(i).active){box=world->instanceHandleAt(i);break;}
            if(!box.generation)world->step(gameplayContactProfileInput(tick,world->mainPlayerX(),world->mainPlayerY()),false);
        }
        require(box.generation&&world->effectsSucceeded(),"find box sound caller");
        require(BoxSoundVerification::run(*world,box,owner->resolve(box)->box.breakSound,
            [&](){return &owner->resolve(box)->sound;},failure),"source box sound ownership and failure prefix");
        if(!failure){
            require(world->restartFromResults(DeathMenuSequence::Action::Retry,43)&&world->overlapSound(7)==0,
                "new run reinitializes overlap channel entries");
        }
    }
    for(unsigned failure=1;failure<=7;++failure){
        auto owner=std::make_unique<ProjectileFailureOwner>(objects,sprites,masks);owner->reject=failure;
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
            InstanceLifecycleBinding::bindPointQueries(*owner));
        require(owner->attach(*world),"attach projectile failure fixture");world->skipToGameForTesting();
        for(unsigned tick=0;tick<1000&&world->effectsSucceeded();++tick)
            world->step(gameplayContactProfileInput(tick,world->mainPlayerX(),world->mainPlayerY()),false);
        require(!world->effectsSucceeded()&&owner->rejected==failure,"publication failure reaches a terminal simulation latch");
        const auto calls=owner->calls;const auto draws=world->randomDrawCount();world->step({},false);
        require(owner->calls==calls&&world->randomDrawCount()==draws,"failed projectile frame cannot replay callbacks or RNG");
    }
    {
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach marked-projectile publication world");world->skipToGameForTesting();
        for(unsigned tick=0;tick<1000&&!world->mainBulletCount();++tick)
            world->step(gameplayContactProfileInput(tick,world->mainPlayerX(),world->mainPlayerY()),false);
        require(world->effectsSucceeded()&&world->mainBulletCount(),"emit a real projectile for destruction publication");
        const auto key=world->mainBulletAt(0).instance;
        require(world->markBulletDestroyed(key)&&world->markBulletDestroyed(key)&&owner->resolve(key),
            "repeated destruction requests retain the executing projectile identity");
        RunnerImageValue angle;auto degrees=MotionBinary64::fromInteger(267);
        require(degrees.add({0x3fd00000u,0})&&angle.assign(degrees)&&world->publishBulletAngle(key,angle),
            "marked projectile accepts the source-narrowed fractional angle");
        std::uint32_t storedAngle=0;std::memcpy(&storedAngle,&owner->resolve(key)->rotation,4);
        require(storedAngle==angle.bits,"stored projectile retains the exact published angle bits");
        RunnerImageValue x,y,actualX,actualY;
        require(x.assign(MotionBinary64::fromInteger(240))&&y.assign(MotionBinary64::fromInteger(500))&&
            world->publishBulletCollisionPosition(key,x,y)&&owner->collisionPosition(key,actualX,actualY)&&
            actualX.bits==x.bits&&actualY.bits==y.bits&&owner->resolve(key)->destroyRequested,
            "Step-tail position setters publish through a real owner after destruction without resurrection");
        auto stale=key;++stale.generation;
        require(!world->markBulletDestroyed(stale)&&!world->effectsSucceeded(),
            "stale projectile destruction latches failure");
    }
    {
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach projectile ownership world");world->skipToGameForTesting();
        std::vector<InstanceHandle> previous;unsigned births=0,retirements=0;
        for(unsigned tick=0;tick<1800;++tick){
            world->step(gameplayContactProfileInput(tick,world->mainPlayerX(),world->mainPlayerY()),false);
            unsigned population=0;
            require(world->effectsSucceeded()&&owner->valid()&&owner->instanceNumber(261,population)&&
                population==world->mainBulletCount(),"stored projectile population equals live gameplay shots");
            for(auto old:previous)if(!owner->resolve(old))++retirements;
            std::vector<InstanceHandle> current;
            for(unsigned i=0;i<world->mainBulletCount();++i){
                const auto& bullet=world->mainBulletAt(i);const auto* record=owner->resolve(bullet.instance);
                RunnerImageValue x,y,actualX,actualY;
                require(record&&record->objectId==261&&!record->destroyRequested&&
                    record->spriteId==machineGunProfile(bullet.gemHigh).sprite&&
                    ProjectilePublication::coordinate(bullet.trajectory.x,0,x)&&
                    ProjectilePublication::coordinate(bullet.trajectory.y,0,y)&&
                    owner->collisionPosition(bullet.instance,actualX,actualY)&&x.bits==actualX.bits&&y.bits==actualY.bits,
                    "live projectile handle resolves current resource and fractional position");
                bool found=false;for(auto old:previous)found|=old.slot==bullet.instance.slot&&old.generation==bullet.instance.generation;
                if(!found)++births;current.push_back(bullet.instance);
            }
            previous=current;
        }
        require(births>20&&retirements>20,"natural firing exercises repeated births, impacts and slot reuse");
        require(world->restartFromResults(DeathMenuSequence::Action::Retry,2),"projectile world Retry");
        for(auto old:previous)require(!owner->resolve(old),"Retry invalidates former stored projectile handles");
    }
    {
        PackedObjectEvent event;
        require(!objects.findEvent(371,1,0,event)&&!objects.findEvent(371,12,0,event),
            "entrance field has no inherited Destroy/Clean Up handlers to run before marking");
    }
    {
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach wall-event world");world->skipToGameForTesting();
        InstanceHandle handles[9];unsigned count=0;
        for(unsigned i=0;i<world->activeInstanceCount()&&count<9;++i){
            const auto& r=world->instanceAt(i);if(r.objectId!=60||r.wall.pillar)continue;
            handles[count]=world->instanceHandleAt(i);
            RunnerImageValue x,y;
            require(x.assign(MotionBinary64::fromInteger(1000+int(count%3)*16))&&
                y.assign(MotionBinary64::fromInteger(10000+int(count/3)*16))&&
                world->publishCollisionPosition(handles[count],x,y),"publish isolated wall fixture via engine setters");
            ++count;
        }
        require(count==9,"room provides nine ordinary walls");
        struct Effects {
            bool outOfView(InstanceHandle){require(false,"unexpected cavern out-of-view callback");return false;}
            bool create(int,MotionBinary64,MotionBinary64){require(false,"unexpected cavern limboShard creation");return false;}
        } effects;
        const auto draws=world->randomDrawCount();
        for(auto h:handles)require(owner->wallEndStepEvent(*world,h,1,effects),"stored wall first event");
        const auto* middle=world->resolveInstance(handles[4]);
        require(middle&&middle->wall.surrounded&&middle->wall.checked==1&&middle->imageIndex==15&&
            !middle->destroyRequested&&world->wallBackgroundTiles().size()==0,"stored wall first event defers replacement");
        const auto id=middle->instanceId;
        require(owner->wallEndStepEvent(*world,handles[4],1,effects)&&middle->destroyRequested&&
            world->wallBackgroundTiles().size()==1&&world->randomDrawCount()==draws,"stored second event retains tile and requests destruction");
        std::int32_t x,y;const auto* tile=world->wallBackgroundTiles().at(0);
        require(tile->x.roundPixel(x)&&tile->y.roundPixel(y)&&x==1008&&y==10008,
            "tile uses live engine coordinates, not stale actor motion caches");
        require(world->deactivateWall(handles[4])&&!middle->active&&world->hasInstance(id),"deactivation after destruction retains stored lifetime");
        require(world->retireMarkedWall(handles[4])&&!world->hasInstance(id)&&!owner->resolve(handles[4])&&
            world->wallBackgroundTiles().size()==1,"explicit stored cleanup retains wall artwork");
    }
    {
        struct PointOwner:StoredWorldCollision {
            using StoredWorldCollision::StoredWorldCollision;
            bool succeed=false,answer=false;int selector=-1;
            MotionBinary64 receivedX{},receivedY{};
            bool positionMeeting(int object,MotionBinary64 x,MotionBinary64 y,bool& hit){
                selector=object;receivedX=x;receivedY=y;hit=answer;return succeed;
            }
        };
        auto owner=std::make_unique<PointOwner>(objects,sprites,masks);
        const auto binding=InstanceLifecycleBinding::bindPointQueries(*owner);
        const MotionBinary64 x{0x3ff80000u,1u},y{0xbff40000u,3u};
        bool hit=true;
        require(!binding.positionMeeting(60,x,y,hit)&&hit,
            "failed point callback cannot overwrite the caller's previous result");
        require(owner->selector==60&&owner->receivedX.high==x.high&&owner->receivedX.low==x.low&&
            owner->receivedY.high==y.high&&owner->receivedY.low==y.low,
            "point binding preserves fractional VM coordinate words and selector");
        owner->succeed=true;
        require(binding.positionMeeting(60,x,y,hit)&&!hit,"successful point miss is published");
        owner->answer=true;
        require(binding.positionMeeting(60,x,y,hit)&&hit,"successful point hit is published");
    }
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
        RunnerLayerInstances<4,4> layerRegistry;RunnerLayerInstances<4,4>::Layers layers;RunnerLayerIds layerIds;
        RunnerLayerLifetimes<4,4,4> layerOwner(layers,layerRegistry,layerIds);
        StoredLayerLifecycle<4,4,4> layerObserver{layerOwner};
        auto composed=InstanceLifecycleBinding::bindPositionedBirths(lifecycle);
        require(composed.observeLayers(layerObserver),"observe scheduled effect lifetimes alongside spatial owner");
        StoredTimeChangeRegistration<4,2> registration(records,&objects,composed,nextId,room,healthy);
        TimeChangeEffects<2> effects;require(effects.bind(registration),"bind scheduler-selected changes");
        require(effects.createAt({},{},speed,[](unsigned){return true;}),"first scheduled change");
        require(records.create(99,36,room,false,0)==CreateInstanceResult::success,"interleaved scheduler actor");
        records.find(99)->alarms.set(1,2);
        require(effects.createAt({},{},speed,[](unsigned){return true;}),"second scheduled change");
        require(layerRegistry.find(100)!=layerRegistry.End&&layerRegistry.find(101)!=layerRegistry.End&&
            layerOwner.resolve(layerRegistry.get(layerRegistry.find(100))->instance)&&
            layerOwner.resolve(layerRegistry.get(layerRegistry.find(101))->instance),
            "positioned effect births publish both spatial and layer ownership");
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
        // Object 36 runs before both object-211 instances. Its 58 becomes 63
        // in the first change; the second observes >=60, restores 60 and dies.
        require(tick()&&speed==60&&order==std::vector<unsigned>({99,100,101})&&
            records.find(100)->alarms.get(0)==0&&records.find(100)->alarms.get(1)==2&&
            records.find(101)->alarms.get(1)==0&&records.find(101)->destroyRequested,
            "selected effects follow ascending object order and observe prior handler speed writes");
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
        require(layerRegistry.find(100)==layerRegistry.End&&layerRegistry.find(101)==layerRegistry.End,
            "global effect cleanup also retires observed layer lifetimes");
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
        unsigned scheduledPasses=0,scheduledContacts=0,scheduledDitherContacts=0;
        // An initial authored wall is not under the above-screen dither.
        // Supply deliberate geometry for this mixed-event fixture rather
        // than relying on zeroed authored precise coordinates.
        bool foundDither=false;
        require(world->visitDrawChecked([&](InstanceHandle h,const InstanceRecord& r){
            if(r.objectId!=61)return true;
            foundDither=true;
            return world->publishCollisionPosition(h,{0x43700000u},{});
        })&&foundDither,"publish dither at the fixture's authored wall height");
        const bool scheduledOkay=world->stepWithCollisionPass(Input{},false,[&]{
            ++scheduledPasses;
            const auto before=world->randomDrawCount();
            return owner->collectCollisionPairs(*pairs,true)&&
                owner->executeCollisionPairs(*world,*pairs,eventContext,motion,
                    []{return RunnerCollisionEventContext::RoomState{true,false};},
                    [&](const RunnerCollisionEventContext::Frame& frame){
                        const auto* self=owner->resolve(frame.self);const auto* other=owner->resolve(frame.other);
                        if(frame.code==991){++scheduledDitherContacts;return world->dispatchDitherAboveCollision(frame);}
                        if(!self||!other||frame.code!=2081||self->objectId!=238||other->objectId!=35)return false;
                        ++scheduledContacts;
                        return world->dispatchTimeShardContact(registration,shards,frame);
                    })&&world->randomDrawCount()==before;
        });
        if(!scheduledOkay||scheduledPasses!=1||!scheduledContacts||!scheduledDitherContacts)
            std::fprintf(stderr,"scheduled contacts: okay=%d passes=%u shards=%u dither=%u\n",scheduledOkay,
                scheduledPasses,scheduledContacts,scheduledDitherContacts);
        require(scheduledOkay&&scheduledPasses==1&&scheduledContacts>0&&scheduledDitherContacts>0,
            "ordinary scheduler executes collected shard and dither contacts once; cooldown consumes no RNG");
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
    {
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach automatic entry world");
        world->skipToGameForTesting();
        std::uint32_t id=0;InstanceHandle key;
        for(unsigned i=0;i<world->activeInstanceCount();++i){
            const auto& r=world->instanceAt(i);
            if(r.objectId!=73)continue;
            require(!id&&i>0&&world->instanceAt(i-1).objectId==64&&
                r.instanceId==world->instanceAt(i-1).instanceId+1&&r.roomId==11&&
                r.preciseY==16&&r.alarms.get(0)==119,"automatic entry follows camera and first room tick");
            id=r.instanceId;key=world->instanceHandleAt(i);
        }
        require(id&&owner->resolve(key),"automatic entry has collision ownership without scoped dispatch");
        for(unsigned tick=2;tick<=120;++tick){
            world->step({},false);
            require(world->effectsSucceeded()&&world->roomLoadSucceeded(),"automatic entry remains healthy");
            const auto* r=world->instance(id);
            if(tick<120)require(r&&r->preciseY==float(tick*16)&&r->alarms.get(0)==120-int(tick),
                "automatic entry advances once per room tick");
            else require(!r&&!owner->resolve(key),"automatic alarm cleanup retires record and owner");
        }
        unsigned drawn=0;
        require(world->visitEntryDitherDraw([&](auto,const auto&,int){++drawn;return true;})&&!drawn,
            "expired automatic entry never draws");
        world->reset(PlatformProfile::nonDesktop);
        world->skipToGameForTesting();
        InstanceHandle beforeRetry;
        for(unsigned i=0;i<world->activeInstanceCount();++i)
            if(world->instanceAt(i).objectId==73)beforeRetry=world->instanceHandleAt(i);
        require(beforeRetry.generation&&owner->resolve(beforeRetry),"reset permits another owned entry lifetime");
        require(world->restartFromResults(DeathMenuSequence::Action::Retry,42)&&!owner->resolve(beforeRetry),
            "Retry retires an entry cover before alarm expiry");
    }
    auto storage=std::make_unique<Storage>();require(storage->families.bindInstanceFlags(objects),"bind full source families");
    RecordingLifecycle lifecycle({storage->owner,storage->properties,objects,sprites,storage->families,{}, {}});
    auto runtime=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
        InstanceLifecycleBinding::bindQueryDestruction(lifecycle));
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
        require(owner->attach(*world),"attach time-change world");
        {
            world->skipToGameForTesting();
            auto drone=world->droneBulletRegistration<2>();InstanceHandle local;
            const RunnerImageValue x{0x43704000u},y{0x43308000u};
            require(drone.create(x.promoted(),y.promoted(),local),"registered shared drone Create");
            const auto key=drone.worldHandle(local);
            const auto* shared=drone.instances().find(local);
            require(shared&&key.generation,"drone has distinct shared and world handles");
            const unsigned id=shared->sourceId;
            const auto* r=world->instance(id);RunnerImageValue publishedX,publishedY;
            require(r&&r->objectId==266&&r->spriteId==459&&r->imageSpeed==0&&r->alarms.get(0)==16&&
                owner->collisionPosition(key,publishedX,publishedY)&&publishedX.bits==x.bits&&publishedY.bits==y.bits,
                "drone Create publishes resource override and precise position to stored owner");
            require(drone.dispatch(local,[](auto& state,auto& publish,bool&){
                state.motion.direction=MotionBinary64::fromInteger(90);
                state.damage=MotionBinary64::fromInteger(20);
                return publish.publishAngle({0x42b40000u})&&publish.publishImageIndex({0x3f000000u});
            }),"drone source caller overrides");
            r=world->instance(id);
            require(r&&r->rotation==90&&r->imageIndex==0.5F,"stored Draw fields reflect shared publications");
            require(drone.activate(local,false)&&!world->instance(id)->active&&drone.activate(local,true),
                "drone activation reaches stored ownership");
            unsigned droneSteps=0,droneEnds=0;
            auto event=[&](auto&,auto&,StartupRuntime::DronePhase phase,auto&,bool&){
                if(phase==StartupRuntime::DronePhase::step)++droneSteps;
                else if(phase==StartupRuntime::DronePhase::endStep)++droneEnds;
                else return false;
                return true; // This tape tests phase routing, not solid collision.
            };
            for(unsigned tick=1;tick<=16;++tick){
                RunnerImageValue centre;require(centre.assign(MotionBinary64::fromInteger(world->mainCameraRenderViewY()+142)),"drone centre");
                require(drone.dispatch(local,[&](auto&,auto& publish,bool&){return publish.publishPosition(x,centre);}),
                    "keep phase-routing fixture within the published view");
                require(world->stepWithDroneBullets(drone,{},event),"drone joins stored room scheduler");
                const auto* state=drone.instances().find(local);
                const auto* stored=world->instance(id);
                require(state&&stored&&state->state.decelerate==(tick==16)&&state->state.alarm0==16-int(tick)&&
                    stored->alarms.get(0)==16-int(tick),
                    "global alarm decrements once and is not restored by Step publication");
            }
            require(droneSteps==16&&droneEnds==0,"drone Step callback and built-in End Step have distinct ownership");
            require(drone.dispatch(local,[](auto&,auto& publish,bool&){
                return publish.destroySelf()&&publish.publishPosition({0x43708000u},{0x4330c000u});
            })&&world->instance(id)->destroyRequested&&
                owner->collisionPosition(key,publishedX,publishedY)&&publishedX.bits==0x43708000u&&
                publishedY.bits==0x4330c000u,"destroyed drone retains post-event position publication");
            require(drone.retire(local)&&!world->instance(id)&&!drone.instances().find(local),
                "drone cleanup retires both identities");
            InstanceHandle replacement;
            require(drone.create(x.promoted(),y.promoted(),replacement)&&replacement.slot==local.slot&&
                replacement.generation>local.generation&&drone.instances().find(replacement)->sourceId>id&&
                !drone.worldHandle(local).generation,
                "drone recycling preserves source allocator and rejects stale handles");
            require(drone.dispatch(replacement,[](auto& state,auto&,bool&){
                state.imageSpeed={0x3f000000u};state.timeline={0x40b00000u};return true;
            })&&world->stepWithDroneBullets(drone,{},event)&&
                drone.instances().find(replacement)->destroyRequested&&droneSteps==16&&droneEnds==0&&drone.clear(),
                "room animation destroys drone before same-tick alarm and Step dispatch");
            InstanceHandle edge;
            require(drone.create(x.promoted(),MotionBinary64::fromInteger(world->mainCameraRenderViewY()+316),edge)&&
                world->stepWithDroneBullets(drone,{},event)&&!drone.instances().find(edge)->destroyRequested,
                "stored End Step keeps exact published bottom boundary");
            RunnerImageValue below;require(below.assign(MotionBinary64::fromInteger(world->mainCameraRenderViewY()+317)),"drone lower edge");
            require(drone.dispatch(edge,[&](auto&,auto& publish,bool&){return publish.publishPosition(x,below);})&&
                world->stepWithDroneBullets(drone,{},event)&&drone.instances().find(edge)->destroyRequested&&drone.clear(),
                "stored End Step retires below old published view despite current camera movement");
        }
    }
    {
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach time-change world");
        {
            auto entry=world->entryDitherRegistration<1>();InstanceHandle key;
            unsigned reads=0;bool movedCamera=false;
            auto camera=[&](MotionBinary64& out){
                ++reads;unsigned cameras=0;
                if(reads==1){
                    const auto* view=world->roomEntryView();
                    require(view&&view->y.bits==0,"Create observes serialized initial view before following");
                }
                for(unsigned i=0;i<world->activeInstanceCount();++i){
                    const auto& r=world->instanceAt(i);
                    if(r.objectId!=64||r.roomId!=11)continue;
                    ++cameras;
                    RunnerImageValue x,y,registeredX,registeredY;
                    std::memcpy(&x.bits,&r.preciseX,4);std::memcpy(&y.bits,&r.preciseY,4);
                    require(owner->collisionPosition(world->instanceHandleAt(i),registeredX,registeredY)&&
                        registeredX.bits==x.bits&&registeredY.bits==y.bits,
                        "entry reader sees the same precise camera fields as the registered owner");
                    require(r.preciseX==float(r.x)&&r.preciseY==float(r.y),
                        "camMain publishes rounded builtins, not fractional easing accumulators");
                    movedCamera|=r.y!=176;
                    out=x.promoted();
                }
                require(cameras==1,"entry reads the unique main-room parentCamera descendant");
                return true;
            };
            require(world->withEntryDitherRoomCreation(entry,camera,[&]{world->skipToGameForTesting();return true;}),
                    "player Create births entry dither synchronously through its registered owner");
            std::uint32_t id=0;
            for(unsigned i=0;i<world->activeInstanceCount();++i){
                const auto& r=world->instanceAt(i);
                if(r.objectId!=73)continue;
                require(!id&&i>=3,"one player-created entry dither");id=r.instanceId;key=entry.handle(r);
                const auto& cameraRecord=world->instanceAt(i-1);
                require(cameraRecord.objectId==64&&id==cameraRecord.instanceId+1&&r.roomId==11&&
                    r.preciseX==240&&r.preciseY==16&&r.alarms.get(0)==119,
                    "entry follows camera Create before next authored instance and joins first room tick");
            }
            // Fast-forward returns after the first new-room update, not at its
            // Create boundary: one Create and one ordinary Step have run.
            require(id&&key.generation&&reads==2,"room entry executed Create and first room Step");
            for(unsigned tick=2;tick<=120;++tick){
                require(world->stepWithEntryDither(entry,camera,{}),"entry dither joins actual room tick");
                const auto* r=world->instance(id);
                require(r&&r->destroyRequested==(tick==120)&&r->alarms.get(0)==120-int(tick),
                        "global alarm phase decrements entry lifetime exactly once");
                require(r->preciseY==float((tick<120?tick:119)*16),
                        "alarm destruction excludes same-tick entry Step");
                unsigned drawn=0;
                require(world->visitEntryDitherDraw([&](InstanceHandle h,const InstanceRecord& record,int depth){
                    ++drawn;
                    require(h.slot==key.slot&&h.generation==key.generation&&record.instanceId==id&&
                        depth==-200000&&record.spriteId==129&&record.preciseY==r->preciseY,
                        "entry Draw resolves original layer depth and published lifetime state");
                    return true;
                })&&drawn==(tick==120?0u:1u),
                    "entry Draw keeps offscreen origins until alarm destruction, without scrInView clipping");
            }
            require(reads==120,"one Create and 119 ordinary-Step camera reads");
            require(movedCamera,"rounded camera publication is exercised during actual camera movement");
            require(world->cleanupEntryDither(entry,[](auto,const auto&){return false;})&&!world->instance(id),
                    "global cleanup retires marked entry controller and collision/layer ownership");
        }
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
    for(bool failAudioQuery:{false,true}){
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach transition world");world->skipToGameForTesting();
        require(world->setOxygen({0x40230000u,0}),"set fractional oxygen below floor");
        world->registerLevelExit(176);
        TimeStopEffects<16> state;
        auto shards=world->timeShardRegistration<16>();auto changes=world->timeChangeRegistration<8>();
        require(state.shards.bindPhases(shards)&&state.changes.bind(changes),"bind both transition families");
        TimeStopEffects<16>::PlayerPose pose{{0x43700000u},{0x43300000u},{},{},
            RunnerImageValue{0x43704000u}.promoted(),RunnerImageValue{0x43308000u}.promoted()};
        const auto first=world->nextRuntimeInstanceId();const auto draws=world->randomDrawCount();
        unsigned pauses=0;
        auto emit=[&](PlayerTimeStop::Effect effect){
            if(effect==PlayerTimeStop::Effect::oxygenFloor)
                require(world->oxygen().high==0x40240000u&&!world->oxygen().low,"shared oxygen floor precedes notification");
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
        auto failPause=[&](auto effect){return failAudioQuery||effect!=PlayerTimeStop::Effect::change;};
        require(!world->beginTimeStopAudioChecked(shards,changes,state,pose,false,
            [&](auto,bool& playing){playing=true;return !failAudioQuery;},failPause)&&
            !world->effectsSucceeded()&&state.shards.size()==3&&state.changes.size()==1&&
            world->instance(failedId+3)&&world->randomDrawCount()==failedDraws+24&&
            world->nextRuntimeInstanceId()==failedId+4&&state.player.bgmOn,
            "failed transition pause or audio query retains first burst and Create but prevents second burst");
        require(!step()&&world->randomDrawCount()==failedDraws+24&&world->nextRuntimeInstanceId()==failedId+4,
            "failed transition cannot restart through a later Begin Step");
    }
    for(bool rejectPause:{false,true}){
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach combined bone/time-stop world");world->skipToGameForTesting();
        world->registerLevelExit(176);
        TimeStopEffects<16> stop;
        auto shards=world->timeShardRegistration<16>();auto changes=world->timeChangeRegistration<8>();
        auto bones=world->boneRegistration<2>();InstanceHandle bone;
        require(stop.shards.bindPhases(shards)&&stop.changes.bind(changes)&&world->spawnBone(bones,240,200,bone),
            "bind all families before the combined tick");
        BoneProjectile expected=bones.instances().find(bone)->state;
        const auto boneId=bones.instances().find(bone)->sourceId;
        auto blood=world->bloodRegistration<1>();InstanceHandle particle;
        require(world->spawnBlood(blood,MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(800),
            MotionBinary64::fromInteger(14),[](auto){return true;},particle),"register blood outside the player's field");
        BloodEffect expectedBlood=blood.instances().find(particle)->state;
        auto currency=world->currencyRegistration<2>();InstanceHandle gem,homingGem;
        require(world->createRegisteredCurrency(currency,MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(800),
            [](auto,int,auto,auto,bool& hit){hit=false;return true;},gem),"register currency outside the player's field");
        require(world->createRegisteredCurrency(currency,MotionBinary64::fromInteger(244),MotionBinary64::fromInteger(800),
            [](auto,int,auto,auto,bool& hit){hit=false;return true;},homingGem)&&currency.attracted(homingGem,true),
            "register attracted currency alongside the frozen free currency");
        CurrencyCreate expectedHoming=*currency.find(homingGem);
        require(currency.dispatch(gem,[](auto& state,bool&){
            state.xsp={};state.ysp=MotionBinary64::fromInteger(1);return true;
        }),"give the currency a nonzero source free-motion velocity");
        struct CurrencyAudio {
            bool isPlaying(int,bool& playing){playing=false;return true;}
            bool stop(int){return true;}
            bool play(int,int,bool,int&){return false;} // No floor in this fixture.
            bool getGain(int,MotionBinary64&){return false;}
            bool setGain(int,MotionBinary64,int){return false;}
            bool setPitch(int,MotionBinary64){return false;}
        } currencyAudio;
        CurrencyHomingMath currencyMath{portableAtan};
        unsigned currencySteps=0;
        TimeStopEffects<16>::PlayerPose pose{{0x43700000u},{0x43300000u},{},{},
            RunnerImageValue{0x43704000u}.promoted(),RunnerImageValue{0x43308000u}.promoted()};
        unsigned begins=0,collisions=0,pauses=0;std::uint32_t transitionId=0;
        auto transition=[&](const auto&,const auto&,int,int){
            ++begins;
            return world->beginTimeStop(shards,changes,stop,pose,false,[](auto){return true;},
                [&](PlayerTimeStop::Effect effect){
                    if(effect==PlayerTimeStop::Effect::change){
                        ++pauses;transitionId=world->nextRuntimeInstanceId()-1;
                        const auto* child=world->instance(transitionId);
                        require(child&&child->objectId==211&&child->alarms.get(1)==2&&stop.roomSpeed==30,
                            "time-change Create is synchronous before pause and bone Step");
                        return !rejectPause;
                    }
                    return true;
                });
        };
        auto frames=[&](int sprite,unsigned& count){return sprites.frameCount(sprite,count);};
        auto step=[&](){return world->withCurrencyEvents(currency,
            [&](auto key,auto&,auto phase,unsigned,auto& random,bool&){
                if(phase!=StartupRuntime::CurrencyPhase::step)return false;
                ++currencySteps;
                return currency.stepSource(key,random,stop.player.stopped,480,pose.x.promoted(),pose.y.promoted(),
                    currencyMath,[](auto& top){top={};return true;},currencyAudio,MotionBinary64::fromInteger(1),false);
            },[&]{return world->stepWithTimeStopBonesAndBlood(blood,bones,shards,changes,stop,Input{},frames,transition,[&]{
            ++collisions;
            const auto* actual=bones.instances().find(bone);
            require(actual&&begins==collisions&&actual->state.imageSpeed.bits==expected.imageSpeed.bits&&
                actual->state.timeline.bits==expected.timeline.bits&&actual->state.motion.x==expected.motion.x&&
                actual->state.motion.y==expected.motion.y&&actual->state.destroyRequested==expected.destroyRequested,
                "collision boundary sees bone Step under this tick's player time-stop state");
            const auto* effect=blood.instances().find(particle);
            require(effect&&effect->state.imageSpeed.bits==expectedBlood.imageSpeed.bits&&
                effect->state.timeline.bits==expectedBlood.timeline.bits&&effect->x.bits==expectedBlood.x.bits&&
                effect->y.bits==expectedBlood.y.bits,"blood consumes that same Begin Step state before collision");
            const auto* money=currency.find(gem);
            require(money&&currencySteps==2*collisions&&money->allSet,
                "each registered currency Step ran once before collision in the combined tick");
            const auto* homing=currency.find(homingGem);
            require(homing&&homing->xx.high==expectedHoming.xx.high&&homing->xx.low==expectedHoming.xx.low&&
                homing->yy.high==expectedHoming.yy.high&&homing->yy.low==expectedHoming.yy.low&&
                homing->alarms.get(0)==10-int(collisions)&&homing->alarms.get(3)==15-int(collisions),
                "attracted currency bypasses the stop gate while its sibling freezes, without refunding alarms");
            if(stop.player.stopped){
                require(money->yy.high==MotionBinary64::fromInteger(800).high&&!money->yy.low&&
                    !money->imageSpeed.bits&&money->obtainable&&money->alarms.get(0)==10&&money->alarms.get(3)==15,
                    "currency freezes immediately on entry and refunds only this tick's alarm decrements");
            }else{
                auto expectedY=MotionBinary64::fromInteger(801);
                require(expectedY.add(money->gravity)&&money->yy.high==expectedY.high&&money->yy.low==expectedY.low&&
                    money->imageSpeed.bits&&money->alarms.get(0)==9&&money->alarms.get(3)==14,
                    "currency resumes source gravity and alarm countdown in the same exit tick");
            }
            return true;
        });});};
        struct Events {void destroy(){}void effect(int,int){}} events;
        const auto advanceReference=[&](bool stopped){bool crossed;
            require(expected.advanceImage(crossed)&&expected.step(stopped,[](int,int){return true;},events),
                "advance the retained source bone event oracle");
            require(expectedBlood.advanceImage()&&expectedBlood.step(!stopped),"advance blood source oracle outside fields");
            bool good=true;
            require(CurrencyHomingSource::run(expectedHoming,pose.x.promoted(),pose.y.promoted(),currencyMath,good),
                "advance attracted source currency even while other actors are frozen");
        };
        advanceReference(true);
        const auto before=world->randomDrawCount();
        const bool entered=step();
        if(rejectPause){
            require(!entered&&!world->effectsSucceeded()&&begins==1&&!collisions&&pauses==1&&
                stop.player.stopped&&stop.shards.size()==3&&world->randomDrawCount()==before+24&&
                world->instance(transitionId)&&bones.instances().find(bone)->state.imageSpeed.bits==0x3f800000u,
                "failed pause retains prior animation and first burst but prevents bone Step and collision");
            const auto draws=world->randomDrawCount();const auto id=world->nextRuntimeInstanceId();
            const auto* untouched=blood.instances().find(particle);
            require(untouched&&untouched->state.savedSpeed.bits==0xbf800000u&&
                untouched->state.timeline.bits==0x3f000000u,"failed transition retains blood animation but prevents its Step");
            require(!currencySteps&&!currency.find(gem)->allSet&&currency.find(gem)->alarms.get(0)==10,
                "failed Begin Step prevents currency initialization, RNG choice and alarm dispatch");
            require(!step()&&begins==1&&world->randomDrawCount()==draws&&world->nextRuntimeInstanceId()==id,
                "failed combined tick cannot replay the transition prefix");
            continue;
        }
        require(entered&&stop.player.stopped&&stop.shards.size()==6&&pauses==1&&begins==1&&collisions==1&&
            world->instance(boneId)&&bones.instances().find(bone)->state.motion.y==200,
            "one tick enters the field and freezes bone motion without an extra world step");
        require(bones.dispatch(bone,[](auto& state,auto&,bool&){state.health={};return true;}),"damage the paused bone");
        expected.health={};advanceReference(true);
        require(step()&&begins==2&&collisions==2&&pauses==1&&world->instance(boneId),
            "held time stop defers bone health destruction");
        pose.y={0x447a0000u};pose.yy=RunnerImageValue{0x447a2000u}.promoted();
        advanceReference(false);
        require(step()&&begins==3&&collisions==3&&pauses==2&&!stop.player.stopped&&!world->instance(boneId)&&
            !bones.instances().find(bone)&&world->scriptEffects().size()==1&&
            world->scriptEffects().atNewest(0)->spriteId==85,
            "exit transition resumes bone Step, publishes the real impact and retires its caller");
        require(stop.reset()&&blood.clear()&&currency.clear(),"retire transition families before their bridges leave scope");
    }
    {
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach bone scope recursion world");world->skipToGameForTesting();
        auto bones=world->boneRegistration<1>();unsigned ticks=0;
        auto execute=[](InstanceHandle,auto&,auto&,auto&,bool&){return true;};
        require(!world->withBoneEvents(bones,execute,[&]{
            require(!world->withBoneEvents(bones,execute,[&]{++ticks;return true;}),"reject nested bone scope");
            return true; // Ignoring the inner error cannot revive the world.
        })&&!world->effectsSucceeded()&&!ticks,"bone scope preserves terminal nested failure");
    }
    {
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach shootable bone field world");world->skipToGameForTesting();
        auto bones=world->boneRegistration<1>();InstanceHandle local;
        require(world->spawnBone(bones,240,200,local),"create retained shootable bone");
        const auto selected=bones.worldHandle(local);const auto draws=world->randomDrawCount();
        MotionBinary64 hp;bool hitEffect=true;
        require(bones.readShootable(selected,hp,hitEffect)&&hp.high==0x3ff00000u&&!hp.low&&!hitEffect,
            "inherited Create exposes objHp and hitEffect");
        require(bones.activate(local,false)&&bones.writeShootableHealth(selected,{0xc0040000u,0})&&
            bones.writeShootableHit(selected)&&bones.readShootable(selected,hp,hitEffect)&&hp.high==0xc0040000u&&!hp.low&&
            bones.instances().find(local)->state.hit&&!bones.instances().find(local)->active,
            "cross-instance fractional health/hit writes do not reactivate a dormant bone");
        require(bones.activate(local,true),"restore bone engine activation");
        auto frames=[&](int sprite,unsigned& count){return sprites.frameCount(sprite,count);};
        auto step=[&](bool stopped){return bones.dispatch(local,[&](auto& actor,auto& publish,bool& good){
            return actor.stepInWorld(stopped,publish,[&]{return world->spawnBoneImpact(bones,local,frames);},
                [&](bool& visible){return world->boneInView(bones,local,visible);},good);
        });};
        require(step(true)&&!world->resolveInstance(selected)->destroyRequested,
            "paused bone retains negative fractional health until its unpaused Step");
        require(step(false)&&world->resolveInstance(selected)->destroyRequested,
            "resumed bone executes its real impact and destruction sequence");
        require(bones.writeShootableHealth(selected,{0x3ff40000u,0})&&bones.readShootable(selected,hp,hitEffect)&&
            hp.high==0x3ff40000u&&!hp.low&&world->resolveInstance(selected)->destroyRequested&&world->randomDrawCount()==draws,
            "marked lifetime remains writable without undoing destruction or consuming RNG");
        require(bones.retire(local)&&!world->resolveInstance(selected),"retire selected shootable lifetime");
        require(!bones.readShootable(selected,hp,hitEffect)&&!world->effectsSucceeded(),"stale shootable target fails loudly");
    }
    for(int inset:{0,1}){
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach bone cached-view world");world->skipToGameForTesting();
        for(unsigned tick=0;tick<60;++tick)world->step(Input{},false);
        const int oldView=world->mainCameraRenderViewY();
        auto bones=world->boneRegistration<2>();InstanceHandle local;
        require(world->spawnBone(bones,240,oldView+inset,local),"create bone on published top edge");
        const auto sourceId=bones.instances().find(local)->sourceId;
        if(inset){
            auto precise=MotionBinary64::fromInteger(oldView);
            RunnerImageValue published;
            require(precise.add({0x3fd00000u,0})&&published.assign(precise)&&
                bones.dispatch(local,[&](auto& state,auto& publish,bool&){
                    // A collision setter need not update private motion. The
                    // paused Step must query the actual builtin position.
                    state.motion.y=oldView-1;
                    return publish.position({0x43700000u},published);
                }),"publish a fractional bone position independently of its private accumulator");
        }
        unsigned queries=0;
        auto view=[&](bool& visible){++queries;return world->boneInView(bones,local,visible);};
        auto execute=[&](auto& state,auto& publisher,auto&,bool& good){
            struct Ops {
                decltype(publisher) publish;decltype(view)& query;
                bool imageSpeed(RunnerImageValue v){return publish.imageSpeed(v);}
                bool position(RunnerImageValue x,RunnerImageValue y){return publish.position(x,y);}
                bool mask(int id){return publish.mask(id);}
                bool destroy(){return publish.destroy();}
                bool effect(int,int){return false;}
                bool inView(int,int,bool& visible){return query(visible);}
            } ops{publisher,view};
            return state.stepPublished(true,ops,good);
        };
        require(world->stepWithBones(bones,Input{},execute)&&queries==1&&
            world->mainCameraRenderViewY()>oldView+1,"bone reads previous published view despite camera movement");
        require(bool(bones.instances().find(local))==bool(inset)&&bool(world->instance(sourceId))==bool(inset),
            "stopped bone retires at equality but survives a quarter pixel inside old view");
        require(bones.clear(),"clear surviving boundary fixture bone");
    }
    for(unsigned reject:{0u,2u}){
        struct CleanupOwner:StoredWorldCollision {
            using StoredWorldCollision::StoredWorldCollision;
            std::vector<unsigned> order,attempted;unsigned reject=0;
            static bool selected(int object){return object==211||object==238;}
            bool capturePrevious(InstanceHandle key,const InstanceRecord& r){
                if(selected(r.objectId))order.push_back(r.instanceId);
                return StoredWorldCollision::capturePrevious(key,r);
            }
            bool retire(InstanceHandle key,const InstanceRecord& r){
                if(selected(r.objectId)){
                    require(r.destroyRequested,"normal time-effect retirement requires a prior destruction mark");
                    attempted.push_back(r.instanceId);
                    if(attempted.size()==reject)return false;
                }
                return StoredWorldCollision::retire(key,r);
            }
        };
        auto owner=std::make_unique<CleanupOwner>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
            InstanceLifecycleBinding::bindPointQueries(*owner));
        require(owner->attach(*world),"attach mixed time-effect cleanup owner");world->skipToGameForTesting();
        TimeStopEffects<16> state;auto shards=world->timeShardRegistration<16>();auto changes=world->timeChangeRegistration<8>();
        require(state.shards.bindPhases(shards)&&state.changes.bind(changes),"bind automatic cleanup families");
        auto change=[&]{return world->createTimeChange(changes,state.changes,MotionBinary64::fromInteger(240),
            MotionBinary64::fromInteger(176),state.roomSpeed,[](unsigned){return true;});};
        require(change()&&world->emitTimeShards(state.burst,state.shards,{0x43700000u},{0x43300000u},{},{},false)&&change(),
            "interleave time-change and shard lifetimes in the same room");
        for(unsigned i=0;i<world->activeInstanceCount();++i){
            auto& r=const_cast<InstanceRecord&>(world->instanceAt(i));
            if(CleanupOwner::selected(r.objectId))r.alarms.set(r.objectId==211?1:0,1);
        }
        state.roomSpeed=60;owner->reject=reject;unsigned collisions=0;
        const bool ok=world->stepWithTimeStopEffects(shards,changes,state,Input{},false,StartupRuntime::NoTimeTransition{},[&]{
            ++collisions;
            require(owner->order.size()==5&&owner->attempted.empty(),"marked effects retain storage through collision");
            for(auto id:owner->order)require(world->instance(id)&&world->instance(id)->destroyRequested,
                "alarm destruction is visible before pre-Draw cleanup");
            return true;
        });
        require(ok==(reject==0)&&world->effectsSucceeded()==ok&&collisions==1&&
            owner->attempted.size()==(reject?reject:5),"ordered cleanup stops at the rejected lifetime");
        for(unsigned i=0;i<owner->attempted.size();++i)
            require(owner->attempted[i]==owner->order[i],"mixed families retire in room order, not separate family sweeps");
        const unsigned retired=reject?reject-1:5;
        for(unsigned i=0;i<owner->order.size();++i)
            require(bool(world->instance(owner->order[i]))==(i>=retired),"cleanup preserves exactly its accepted retirement prefix");
        if(reject){
            require(!world->stepWithTimeStopEffects(shards,changes,state,Input{})&&owner->attempted.size()==reject,
                "failed time-effect retirement is terminal and not retried");
        }else require(state.reset(),"automatic cleanup leaves owners empty and reusable");
    }
    for(int beyond:{0,1}){
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach shard cached-view world");world->skipToGameForTesting();
        for(unsigned tick=0;tick<60;++tick)world->step(Input{},false);
        require(world->effectsSucceeded(),"reach a naturally descending camera for shard boundary test");
        TimeStopEffects<16> state;auto shards=world->timeShardRegistration<16>();auto changes=world->timeChangeRegistration<8>();
        require(state.shards.bindPhases(shards)&&state.changes.bind(changes),"bind cached-view shard families");
        const int oldView=world->mainCameraRenderViewY();
        auto edge=MotionBinary64::fromInteger(oldView);
        if(beyond)require(edge.add(RunnerImageValue{0xbe800000u}.promoted()),"quarter-pixel outside edge");
        RunnerImageValue y;
        require(y.assign(edge)&&
            world->emitTimeShards(state.burst,state.shards,{0x43700000u},y,{},{},false),
            "create real registered shards at the strict outOfView upper edge");
        const auto id=state.shards.sourceId(1);
        bool sawMarked=false;
        require(world->stepWithTimeStopEffects(shards,changes,state,Input{},false,StartupRuntime::NoTimeTransition{},[&]{
            const auto* r=world->instance(id);
            sawMarked=r&&r->destroyRequested;
            return r&&r->destroyRequested==(beyond!=0);
        })&&
            world->mainCameraRenderViewY()>oldView,"camera moves before the shard Step boundary check");
        require(sawMarked==(beyond!=0)&&bool(world->instance(id))==(beyond==0)&&
            state.shards.size()==(beyond?0u:3u),
            "shard Step preserves cached-view equality and destroys only strictly beyond it");
        require(world->cleanupDestroyed(shards,changes,state,[](auto,const auto&){return false;}),
            "cached-view shard cleanup preserves ordinary deferred destruction");
        require(bool(world->instance(id))==(beyond==0)&&state.reset(),
            "cached-view shard ownership remains valid through cleanup and teardown");
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
                    world->instance(shardId)->alarms.get(0)==lifetime-int(tick)&&
                    world->instance(changeId)&&world->instance(changeId)->destroyRequested==(tick==14);
            })&&collisions==1,"time effects and collision pass share one tick after animation and alarms");
            require(state.shards.get(1)->imageIndex.bits==expectedAnimation.imageIndex.bits&&
                state.shards.get(1)->timeline.bits==expectedAnimation.timeline.bits,
                "world animation traversal advances each registered shard exactly once");
            require(state.roomSpeed==int(tick<12?30+5*(tick/2):60)&&
                world->instance(shardId)->alarms.get(0)==lifetime-int(tick)&&
                bool(world->instance(changeId))==(tick<14),
                "live world alarm counters advance once and execute recovered time-change ramp");
        }
        require(world->cleanupDestroyed(shards,changes,state,[](auto,const auto&){return false;})&&!world->instance(changeId),
            "normal pre-Draw cleanup already retired the time-change; explicit cleanup is idempotent");
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
        auto composedOwner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto composed=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,composedOwner->binding());
        require(composedOwner->attach(*composed),"attach composed player-transition world");
        composed->skipToGameForTesting();composed->registerLevelExit(176);
        TimeStopEffects<16> composedState;
        auto composedShards=composed->timeShardRegistration<16>();
        auto composedChanges=composed->timeChangeRegistration<8>();
        require(composedState.shards.bindPhases(composedShards)&&composedState.changes.bind(composedChanges),
            "bind composed player-transition families");
        unsigned composedPauses=0,queries=0;
        auto composedStep=[&]{
            return composed->stepWithPlayerTimeStop(composedShards,composedChanges,composedState,Input{},false,
                [&](PlayerTimeStop::AudioQuery,bool& answer){++queries;answer=false;return true;},
                [&](auto effect){if(effect==PlayerTimeStop::Effect::change)++composedPauses;return true;});
        };
        unsigned pauses=0,hooks=0;std::uint32_t first=0;
        auto transition=[&](const InstanceRecord& player,const PlayerMotion& motion,int ox,int oy){
            ++hooks;if(!first)first=world->nextRuntimeInstanceId();
            require(player.preciseX==static_cast<float>(player.x)&&player.preciseY==static_cast<float>(player.y),
                "player Begin Step publishes rounded builtins before time-field identity queries");
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
        require(composedStep()&&composedPauses==1&&queries>0&&composedState.player.stopped&&
            composedState.shards.size()==6&&composedState.changes.sourceId(1)==first+3&&
            composedState.shards.sourceId(4)==first+4&&composed->stateDigest()==world->stateDigest()&&
            composed->randomDrawCount()==world->randomDrawCount(),
            "runtime player-pose composition matches explicit transition state, identities and RNG");
        auto camera=[&]() -> const InstanceRecord& {
            for(std::size_t i=0;i<world->activeInstanceCount();++i)
                if(world->instanceAt(i).objectId==64)return world->instanceAt(i);
            require(false,"registered transition camera exists");std::abort();
        };
        require(state.shakeAmt==2&&state.shakeDur==2&&camera().alarms.get(0)==1&&
            camera().x==world->mainCameraRenderViewX()+80&&camera().y==world->mainCameraRenderViewY()+142,
            "Begin Step shake arms the real camera before alarms and publishes its shaken Step position");
        require(state.shards.get(1)->timeline.bits==0&&world->instance(first)->alarms.get(2)==29&&
            world->instance(first+3)->alarms.get(1)==1,
            "Begin Step births miss preceding animation but join same-tick alarms and Step");
        require(world->stepWithTimeStopEffects(shards,changes,state,Input{},false,transition)&&hooks==2&&pauses==1,
            "held field does not recreate the transition on the next world tick");
        require(camera().alarms.get(0)==0,"camera Alarm 0 expires on second tick without repeated shake request");
        require(composedStep()&&composedPauses==1&&composed->stateDigest()==world->stateDigest()&&
            composed->randomDrawCount()==world->randomDrawCount(),
            "composed held-field tick preserves mixed phase state without repeating transition");
        require(composedState.reset(),"retire composed transition fixture before teardown");
        require(state.reset(),"retire live transition fixture before teardown");
    }
    {
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach composed transition audio-failure world");
        world->skipToGameForTesting();world->registerLevelExit(176);
        TimeStopEffects<16> state;auto shards=world->timeShardRegistration<16>();auto changes=world->timeChangeRegistration<8>();
        require(state.shards.bindPhases(shards)&&state.changes.bind(changes),"bind audio-failure families");
        unsigned queries=0,pauses=0,enters=0;
        auto tick=[&]{return world->stepWithPlayerTimeStop(shards,changes,state,Input{},false,
            [&](PlayerTimeStop::AudioQuery query,bool&){
                ++queries;require(query==PlayerTimeStop::AudioQuery::musicPlaying,"first transition audio guard");
                return false;
            },[&](auto effect){
                if(effect==PlayerTimeStop::Effect::change)++pauses;
                if(effect==PlayerTimeStop::Effect::enterSound)++enters;
                return true;
            });};
        require(!tick()&&!world->effectsSucceeded()&&queries==1&&pauses==1&&enters==0&&state.shards.size()==3&&
            state.changes.sourceId(1)!=0,"unavailable audio stops after the first burst and time-change birth");
        require(world->instance(state.shards.sourceId(1))->alarms.get(2)==30&&
            world->instance(state.changes.sourceId(1))->alarms.get(1)==2,
            "failed Begin Step does not advance later alarm phases");
        const auto draws=world->randomDrawCount();
        require(!tick()&&queries==1&&world->randomDrawCount()==draws,
            "failed composed transition cannot replay its partial effects");
        require(!state.reset(),"terminal transition failure cannot be cleared into a reusable world");
    }
    {
        auto owner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,owner->binding());
        require(owner->attach(*world),"attach moving player publication world");world->skipToGameForTesting();
        Input walking;walking.right=true;bool moved=false;int firstX=0;
        for(unsigned tick=0;tick<12;++tick){
            require(world->stepWithCollisionPass(walking,false,[&]{
                for(std::size_t i=0;i<world->activeInstanceCount();++i){
                    const auto& player=world->instanceAt(i);if(player.objectId!=35)continue;
                    if(!tick)firstX=player.x;else moved|=player.x!=firstX;
                    require(player.preciseX==static_cast<float>(player.x)&&player.preciseY==static_cast<float>(player.y),
                        "moving player record retains rounded builtin coordinates for identity queries");
                }
                return true;
            }),"step moving player publication world");
        }
        require(moved,"publication regression exercises player movement");
    }
    auto productionOwner=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
    for(unsigned seed=0;seed<128;++seed){
        GameMakerRandom random(seed),oracle(seed);MainCameraMotion camera;
        const int dx=int(oracle.irandom(2))-1,dy=int(oracle.irandom(4))-2;
        camera.requestTimeStopShake();camera.sampleShake(random);
        require(camera.pixelX()==240+dx&&camera.pixelY()==176+dy&&random.drawCount()==4&&
            random.nextU32()==oracle.nextU32(),"camera shake preserves X/Y inclusive ranges and four-word RNG order");
    }
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
    {
        auto collision=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,collision->binding());
        require(collision->attach(*world),"attach debris field fixture");world->skipToGameForTesting();
        world->registerLevelExit(176);
        auto registration=world->debrisRegistration<1>();DebrisEffects<1> effects;unsigned id=0;
        const auto x=MotionBinary64::fromInteger(240),y=MotionBinary64::fromInteger(208);
        require(world->spawnDebris(registration,effects,x,y,1,id),"spawn particle inside goal time field");
        InstancePlaceResult selected;
        require(collision->collisionPoint({},370,x,y,false,false,selected)&&selected.id==int(id),"resolve field fixture particle");
        auto& effect=*effects.get(1);bool frozen=true;
        require(world->debrisFrozen(registration,selected.handle,effect,true,frozen)&&!frozen,
            "SDL debris remains running inside a parentTimeField descendant");
        RunnerImageValue px,py;
        require(px.assign(x)&&py.assign(MotionBinary64::fromInteger(10000))&&registration.move(1,effect,px,py),"move outside field");
        effect.x=px;effect.y=py;
        require(world->debrisFrozen(registration,selected.handle,effect,true,frozen)&&frozen,
            "SDL debris freezes outside fields while pTimeStop is true");
        require(effects.clear(registration),"clear field fixture");
    }
    {
        auto collision=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,collision->binding());
        require(collision->attach(*world),"attach script-effect registration fixture");
        world->skipToGameForTesting();
        {
            auto registration=world->debrisRegistration<1>();DebrisEffects<1> effects;unsigned id=0;
            const auto x=MotionBinary64::fromInteger(240),y=MotionBinary64::fromInteger(10000);
            require(world->spawnDebris(registration,effects,x,y,1,id),"spawn debris time-stop fixture");
            InstancePlaceResult selected;
            require(collision->collisionPoint({},370,x,y,false,false,selected)&&selected.id==int(id),"resolve time-stop particle identity");
            const auto key=selected.handle;auto& particle=*effects.get(1);
            for(int qx:{639,640,960,1280,1281}){
                auto q=MotionBinary64::fromInteger(qx);q.high-=2u<<20;RunnerImageValue px,py;
                require(px.assign(q)&&py.assign(y)&&registration.move(1,particle,px,py),"publish fractional time-stop boundary position");
                particle.x=px;particle.y=py;bool frozen=true;
                require(world->debrisFrozen(registration,key,particle,false,frozen)&&!frozen,"pTimeStop false leaves debris running");
                require(world->debrisFrozen(registration,key,particle,true,frozen)&&frozen==(qx>=640&&qx<=1280),
                    "SDL debris uses strict room-width boundaries and published coordinates");
            }
            require(effects.clear(registration),"clear time-stop boundary fixture");
        }
        {
            auto registration=world->debrisRegistration<1>();DebrisEffects<1> effects;unsigned id=0;
            require(world->spawnDebris(registration,effects,MotionBinary64::fromInteger(20000),
                MotionBinary64::fromInteger(20000),1,id),"spawn scheduled debris in empty terrain");
            DebrisAnimation expected=*effects.get(1);auto random=world->randomState();unsigned stops=0;
            for(unsigned tick=0;tick<40;++tick){
                const bool frozen=tick<5;
                require(expected.advanceImage(),"scheduled reference animation");
                for(unsigned a=0;a<3;++a)expected.advanceAlarm(a);
                require(expected.step(frozen,random,[](auto,auto,bool& hit){hit=false;return true;}),"scheduled reference Step");
                require(world->stepWithDebris(registration,effects,{},
                    [&](InstanceHandle,const auto&,bool& stop){++stops;stop=frozen;return true;}),"SDL scheduler dispatches debris events");
                const auto* record=world->instance(id);const auto* actual=effects.retained(1);
                if(expected.destroyRequested){require(!record&&!actual,"scheduled Alarm 0 retires at room cleanup");continue;}
                require(record&&actual,"scheduled debris stays alive until source expiration");
                require(actual->x.bits==expected.x.bits&&actual->y.bits==expected.y.bits&&
                    actual->imageIndex.bits==expected.imageIndex.bits&&actual->imageSpeed.bits==expected.imageSpeed.bits&&
                    actual->noTouch==expected.noTouch&&actual->disappearing==expected.disappearing&&actual->flash==expected.flash,
                    "scheduled debris state matches animation, alarms, freeze and motion");
                for(unsigned a=0;a<3;++a)require(record->alarms.get(a)==expected.alarms.get(a)&&
                    actual->alarms.get(a)==expected.alarms.get(a),"scheduler does not double-decrement debris alarms");
            }
            require(stops==34,"five frozen frames extend the thirty-frame lifetime without dispatch after death");
            require(world->spawnDebris(registration,effects,{},{},1,id),"spawn debris before Retry");
            require(world->restartFromResults(DeathMenuSequence::Action::Retry,42)&&!effects.size()&&!world->instance(id),
                "Retry clears borrowed debris before room transition");
            require(world->spawnDebris(registration,effects,{},{},1,id)&&effects.retained(3),"reuse monotonic debris owner after Retry");
            require(effects.clear(registration),"clear scheduled debris fixture");
        }
        for(int area=1;area<=5;++area){
            auto debrisRegistration=world->debrisRegistration<1>();DebrisEffects<1> debris;
            const auto cutoff=debris.beginPhase();auto expectedRandom=world->randomState();
            DebrisAnimation expected;const MotionBinary64 x{0x406e0800u,0},y{0x40afff00u,0};
            require(expected.create(x,y,area,expectedRandom),"reference debris Create");
            unsigned id=0;const auto next=world->nextRuntimeInstanceId();
            require(world->spawnDebris(debrisRegistration,debris,x,y,area,id)&&id==next,
                "stored debris uses world IDs and RNG");
            const auto* record=world->instance(id);
            require(record&&record->objectId==370&&record->depth==-1000&&record->spriteId==expected.spriteId&&
                record->preciseX==240.25F&&record->preciseY==4095.5F&&record->alarms.get(0)==30,
                "stored debris retains source coordinates, area sprite, depth and alarms");
            require(world->randomState().stateIndex()==expectedRandom.stateIndex()&&world->randomDrawCount()==expectedRandom.drawCount(),
                "stored debris birth RNG position");
            for(unsigned i=0;i<16;++i)require(world->randomState().stateWord(i)==expectedRandom.stateWord(i),"stored debris birth RNG words");
            InstancePlaceResult selected;
            require(collision->collisionPoint({},370,x,y,false,false,selected)&&selected.id==int(id),
                "stored debris has real collision membership");
            const auto key=selected.handle;
            require(debrisRegistration.dispatchAnimation(key,*record,debris,cutoff),"debris animation excludes newer births");
            const auto phase=debris.beginPhase();require(expected.advanceImage(),"reference debris animation");
            require(debrisRegistration.dispatchAnimation(key,*record,debris,phase)&&
                debris.get(1)->imageIndex.bits==expected.imageIndex.bits,"stored debris publishes shared animation");
            for(bool frozen:{true,false}){
                require(expected.step(frozen,expectedRandom,[&](auto px,auto py,bool& hit){
                    return collision->placeMeeting(key,*record,56,px,py,hit);
                }),"reference motion uses stored collision owner");
                require(world->stepDebris(debrisRegistration,debris,key,phase,
                    [&](InstanceHandle h,const auto&,bool& stop){
                        require(h.slot==key.slot&&h.generation==key.generation,"stored Step keeps lifetime identity");
                        stop=frozen;return true;
                    }),"stored debris Step uses shared motion and world RNG");
                const auto* effect=debris.retained(1);RunnerImageValue px,py,speed;
                std::memcpy(&px.bits,&record->preciseX,4);std::memcpy(&py.bits,&record->preciseY,4);
                std::memcpy(&speed.bits,&record->imageSpeed,4);
                require(effect&&px.bits==expected.x.bits&&py.bits==expected.y.bits&&speed.bits==expected.imageSpeed.bits&&
                    effect->xsp.high==expected.xsp.high&&effect->xsp.low==expected.xsp.low&&
                    effect->ysp.high==expected.ysp.high&&effect->ysp.low==expected.ysp.low,"stored debris publishes exact shared motion");
                for(unsigned a=0;a<3;++a)require(record->alarms.get(a)==expected.alarms.get(a),"stored freeze refunds mirrored alarms");
                for(unsigned i=0;i<16;++i)require(world->randomState().stateWord(i)==expectedRandom.stateWord(i),"stored Step RNG words");
            }
            require(debris.get(1)!=nullptr,"fixture debris remains alive before explicit expiry");
            debris.get(1)->alarms.set(0,1);
            require(debrisRegistration.advanceAlarm(key,*record,debris,phase,0)&&record->destroyRequested&&
                collision->resolve(key)==record,"stored alarm marks without early erase");
            require(collision->collisionPoint({},370,x,y,false,false,selected)&&selected.id==-4,
                "marked debris is excluded from stored collisions");
            require(world->cleanupDestroyed(debrisRegistration,debris,[](auto,const auto&){return false;})&&
                !world->instance(id)&&!collision->resolve(key)&&!debris.retained(1),"stored room cleanup retires debris through its owner");
            require(world->spawnDebris(debrisRegistration,debris,x,y,area,id)&&id>next&&debris.retained(2)&&
                !collision->resolve(key),"stored debris slot reuse cannot revive a stale key");
            require(debris.clear(debrisRegistration),"clear borrowed debris before registry teardown");
        }
        auto registration=world->scriptSpawnRegistration<2>();ScriptSpawnEffects<2> effects;
        unsigned callerId=0;
        auto frames=[&](int id,unsigned& count){
            PackedSpriteCollision resource;if(!sprites.sprite(id,resource))return false;
            count=resource.frameCount;return true;
        };
        const auto before=world->nextRuntimeInstanceId();const auto random=world->randomDrawCount();
        const auto phase=effects.beginPhase();
        require(world->spawnScriptEffect(registration,effects,{0x406e0800u,0},{0x40afff00u,0},603,
            {0x3fe00000u,0},MotionBinary64::fromInteger(270),{},frames,callerId),"source effect script publishes registered child and setters");
        const auto* r=world->instance(before);
        require(r&&callerId==before&&world->nextRuntimeInstanceId()==before+1&&world->randomDrawCount()==random&&
            r->objectId==219&&r->preciseX==240.25F&&r->preciseY==4095.5F&&r->spriteId==603&&
            r->imageSpeed==0.5F&&r->rotation==270&&r->depth==0,"child uses real source ID, fractional position and caller overrides without RNG");
        InstancePlaceResult result;
        require(collision->collisionPoint({},219,{0x406e0800u,0},{0x40afff00u,0},false,false,result)&&result.id==int(before),
            "script child participates in resource-backed collision membership");
        const auto stale=result.handle;
        require(world->advanceScriptEffectAnimation(registration,effects,phase)&&world->instance(before)->imageIndex==0,
            "captured animation cutoff excludes synchronous child birth");
        require(world->advanceScriptEffectAnimation(registration,effects,effects.beginPhase()),
            "publish first child animation through source registry");
        require(world->instance(before)->imageIndex==0.5F,"source record retains fractional image index");
        require(world->advanceScriptEffectAnimation(registration,effects,effects.beginPhase())&&world->instance(before)&&
            world->instance(before)->destroyRequested&&world->instance(before)->imageIndex==0&&collision->resolve(stale),
            "Animation End marks the wrapped record without unlinking room traversal");
        require(collision->collisionPoint({},219,{0x406e0800u,0},{0x40afff00u,0},false,false,result)&&result.id==-4,
            "marked effect is excluded from queries before cleanup");
        require(world->cleanupDestroyed(registration,effects,[](InstanceHandle,const InstanceRecord&){return false;})&&!world->instance(before)&&
            !collision->resolve(stale)&&callerId==before&&world->effectsSucceeded(),
            "cleanup removes generation-checked membership without clearing caller ID");
        unsigned lookups=0;
        auto rejectFrames=[&](int,unsigned&){++lookups;return false;};
        const auto next=world->nextRuntimeInstanceId();
        require(!world->spawnScriptEffect(registration,effects,{},{},603,{0x3fe00000u,0},{},{},rejectFrames,callerId)&&
            !world->effectsSucceeded()&&callerId==next&&world->instance(next)&&effects.size()==1&&lookups==1,
            "failed script setter preserves birth and caller ID while latching world failure");
        require(!world->spawnScriptEffect(registration,effects,{},{},603,{},{},{},rejectFrames,callerId)&&
            lookups==1&&world->nextRuntimeInstanceId()==next+1&&world->randomDrawCount()==random,
            "failed script creation cannot replay earlier side effects or consume RNG");
        require(!world->advanceScriptEffectAnimation(registration,effects,effects.beginPhase())&&world->instance(next),
            "failed world cannot advance or retire the partially created effect");
        require(!world->restartFromResultsWithScriptEffects(DeathMenuSequence::Action::Retry,43,registration,effects)&&
            world->instance(next)&&world->randomDrawCount()==random,"failed effect world cannot reset itself through Retry");
    }
    {
        auto collision=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,collision->binding());
        require(collision->attach(*world),"attach scheduled script effects");world->skipToGameForTesting();
        auto registration=world->scriptSpawnRegistration<2>();ScriptSpawnEffects<2> effects;
        auto frames=[&](int id,unsigned& count){PackedSpriteCollision r;
            if(!sprites.sprite(id,r))return false;count=r.frameCount;return true;};
        auto spawn=[&](unsigned& caller){return world->spawnScriptEffect(registration,effects,
            MotionBinary64::fromInteger(int(world->mainPlayerX())),MotionBinary64::fromInteger(int(world->mainPlayerY())),
            603,{0x3fe00000u,0},{},{},frames,caller);};
        unsigned first=0,second=0;require(spawn(first),"create pre-animation effect");
        require(world->stepWithScriptEffects(registration,effects,{},false,[&]{return spawn(second);}),
            "schedule effect animation and later collision-phase birth in one tick");
        require(world->instance(first)->imageIndex==0.5F&&world->instance(second)->imageIndex==0,
            "room animation advances existing effect but not later child");
        require(world->stepWithScriptEffects(registration,effects,{})&&world->instance(first)->destroyRequested&&
            !world->instance(second)->destroyRequested&&world->instance(second)->imageIndex==0.5F,
            "Animation End leaves later room entries traversable");
        require(world->stepWithScriptEffects(registration,effects,{})&&world->instance(second)->destroyRequested,
            "marked effect does not prevent subsequent room animation");
        require(world->cleanupDestroyed(registration,effects,[](InstanceHandle,const InstanceRecord&){return false;})&&
            !world->instance(first)&&!world->instance(second)&&effects.size()==0,
            "scheduled effects retire through explicit typed cleanup");
        world->step({},false);
        require(world->effectsSucceeded(),"scoped animation callback does not survive its tick");
        const auto birthSequence=effects.birthSequence().next;
        require(spawn(first),"create still-live effect before Retry");
        require(!world->restartFromResultsWithScriptEffects(DeathMenuSequence::Action::None,43,registration,effects)&&
            world->instance(first)&&effects.size()==1&&world->effectsSucceeded(),"no results action preserves live effects");
        InstancePlaceResult old;
        require(collision->collisionPoint({},219,MotionBinary64::fromInteger(int(world->mainPlayerX())),
            MotionBinary64::fromInteger(int(world->mainPlayerY())),false,false,old)&&old.id==int(first),
            "capture old effect collision identity before Retry");
        require(world->restartFromResultsWithScriptEffects(DeathMenuSequence::Action::Retry,43,registration,effects)&&
            effects.size()==0&&!world->instance(first)&&!collision->resolve(old.handle),
            "Retry retires unmarked effects through typed ownership before replacing the room");
        require(spawn(second)&&second!=first&&effects.birthSequence().next>birthSequence&&!collision->resolve(old.handle),
            "new room cannot revive old script-effect identity");
        world->step({},false);
        require(world->effectsSucceeded()&&world->instance(second)->imageIndex==0.5F,
            "ordinary Step automatically animates runtime-created script effects");
        world->step({},false);
        require(world->effectsSucceeded()&&!world->instance(second)&&effects.size()==0,
            "ordinary pre-Draw cleanup retires finished script effects");
        require(spawn(first)&&world->restartFromResults(DeathMenuSequence::Action::Retry,44)&&
            effects.size()==0&&!world->instance(first),"ordinary Retry clears bound still-live script effects");
    }
    {
        auto collision=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
        auto world=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,collision->binding());
        require(collision->attach(*world),"attach world-owned script effects");world->skipToGameForTesting();
        unsigned previous=0;
        auto frames=[&](int id,unsigned& count){PackedSpriteCollision r;if(!sprites.sprite(id,r))return false;
            count=r.frameCount;return true;};
        auto spawn=[&](unsigned& caller){return world->spawnScriptEffect(
            MotionBinary64::fromInteger(int(world->mainPlayerX())),MotionBinary64::fromInteger(int(world->mainPlayerY())),
            603,{0x3fe00000u,0},{},{},frames,caller);};
        for(unsigned cycle=0;cycle<3;++cycle){
            unsigned caller=0;
            require(spawn(caller)&&caller>previous,"world-owned effects allocate fresh source IDs");
            world->step({},false);
            require(world->effectsSucceeded()&&world->scriptEffects().size()==1&&world->instance(caller)->imageIndex==0.5F,
                "ordinary tick animates world-owned effect");
            world->step({},false);
            require(world->effectsSucceeded()&&world->scriptEffects().size()==0&&!world->instance(caller),
                "ordinary cleanup retires world-owned effect");
            require(spawn(caller),"world-owned live effect before Retry");previous=caller;
            require(world->restartFromResults(DeathMenuSequence::Action::Retry,43+cycle)&&
                world->scriptEffects().size()==0&&!world->instance(previous),"Retry clears internal script ownership");
        }
    }
    require(productionOwner->valid(),"production owner validates resource packs");
    auto production=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
        productionOwner->binding());
    require(productionOwner->attach(*production)&&!productionOwner->attach(*plain),
        "production owner attaches exactly one runtime without sharing its identities");
    production->skipToGameForTesting();
    runtime->skipToGameForTesting();plain->skipToGameForTesting();
    {
        unsigned pillars=0;
        for(unsigned i=0;i<production->activeInstanceCount();++i){
            const auto& r=production->instanceAt(i);
            if(r.objectId!=60)continue;
            const int expected=r.creationCodeId==677?1:r.creationCodeId==676||r.creationCodeId==678?-1:0;
            require(r.wall.pillar==expected&&!r.wall.checked&&!r.wall.surrounded&&!r.wall.lights,
                "wall Create defaults precede authored pillar room scripts");
            pillars+=expected!=0;
        }
        require(pillars==3,"all three authored room pillar assignments execute");
    }
    {
        const auto digest=production->stateDigest();
        const InstanceRecord* wall=nullptr;
        for(unsigned i=0;i<production->activeInstanceCount();++i)
            if(production->instanceAt(i).objectId==60){wall=&production->instanceAt(i);break;}
        require(wall!=nullptr,"room supplies terrain for live point queries");
        const auto binding=productionOwner->binding();
        require(binding.hasPositionMeeting(),"production lifecycle publishes point queries");
        bool unchanged=true;
        require(!InstanceLifecycleBinding{}.hasPositionMeeting()&&
            !InstanceLifecycleBinding{}.positionMeeting(60,{},{},unchanged)&&unchanged,
            "missing point ownership fails without publishing a miss");
        for(int dy=-16;dy<=16;dy+=4)for(int dx=-16;dx<=16;dx+=4){
            const int x=wall->x+dx,y=wall->y+dy;bool expected=false;
            for(unsigned i=0;i<production->activeInstanceCount();++i){
                const auto& r=production->instanceAt(i);
                if(!r.active||r.destroyRequested||!storage->families.isA(r.objectId,60))continue;
                require(r.scaleX==1&&r.scaleY==1&&r.rotation==0,"point fixture uses authored unit terrain");
                PackedSpriteCollision resource;
                if(!sprites.collisionSprite(r.spriteId,r.maskId,resource)||!resource.frameCount)continue;
                const int px=x-r.x+resource.originX,py=y-r.y+resource.originY;
                if(px<resource.marginLeft||px>resource.marginRight||py<resource.marginTop||py>resource.marginBottom)continue;
                if(!resource.collisionMode||!resource.collisionMaskCount){expected=true;break;}
                SpriteMaskPack::Mask mask;int frame=int(r.imageIndex)%int(resource.collisionMaskCount);
                if(frame<0)frame+=int(resource.collisionMaskCount);
                require(masks.mask(unsigned(r.maskId<0?r.spriteId:r.maskId),unsigned(frame),mask),"point fixture resolves original mask");
                if(mask.contains(px,py)){expected=true;break;}
            }
            bool hit=false;
            require(binding.positionMeeting(60,MotionBinary64::fromInteger(x),MotionBinary64::fromInteger(y),hit)&&hit==expected,
                "live indexed point query matches authored unit terrain bounds and original masks");
            const auto px=MotionBinary64::fromInteger(x),py=MotionBinary64::fromInteger(y);
            InstancePlaceResult selected;
            require(productionOwner->collisionPoint({},60,px,py,true,false,selected)&&
                (selected.id!=-4)==expected,"resource-backed precise collision_point matches original mask pixels");
            bool boxHit=false;
            for(unsigned i=0;i<production->activeInstanceCount();++i){
                const auto& r=production->instanceAt(i);PackedSpriteCollision resource;
                if(!r.active||r.destroyRequested||!storage->families.isA(r.objectId,60)||
                   !sprites.collisionSprite(r.spriteId,r.maskId,resource)||!resource.frameCount)continue;
                const int lx=x-r.x+resource.originX,ly=y-r.y+resource.originY;
                boxHit|=lx>=resource.marginLeft&&lx<=resource.marginRight&&
                        ly>=resource.marginTop&&ly<=resource.marginBottom;
            }
            require(productionOwner->collisionPoint({},60,px,py,false,false,selected)&&
                (selected.id!=-4)==boxHit,"non-precise point uses resource bounds rather than opaque pixels");
            require(productionOwner->nonPreciseLine({},60,px,py,px,py,false,selected)&&
                (selected.id!=-4)==boxHit,"resource-backed zero-length integer line selects terrain bounds");
            if(boxHit){
                const auto* r=productionOwner->resolve(selected.handle);
                RunnerImageValue sx,sy;
                require(r&&r->instanceId==unsigned(selected.id)&&
                    productionOwner->collisionPosition(selected.handle,sx,sy)&&
                    selected.x.high==sx.promoted().high&&selected.x.low==sx.promoted().low&&
                    selected.y.high==sy.promoted().high&&selected.y.low==sy.promoted().low,
                    "line result publishes generation-checked identity and exact source position");
            }
        }
        require(production->stateDigest()==digest,"point queries do not move instances or consume game RNG");
    }
    require(!runtime->instance(101097)&&!runtime->startTimeField()&&runtime->randomDrawCount()==331,
        "first-level start field consumes Create RNG then retires its placed lifetime");
    require(lifecycle.startWrites==2&&lifecycle.startMarks==1&&lifecycle.startRetires==1&&!lifecycle.handles.count(101097),
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
        RunnerCollisionEventContext::Frame lastWindEvent;
        auto windPass=std::make_unique<StoredDitherCollisionPass>(*productionOwner);
        auto fieldSkeletons=production->skeletonRegistration<3>();
        auto dispatchWind=[&](InstanceHandle field){
            // Preserve a resource-resolved frame only for the later explicit
            // expired-voice/stale-generation checks. The actual contact below
            // goes through pair collection, overlap and paired dispatch.
            bool saved=false;
            for(unsigned i=0;i<production->activeInstanceCount();++i){
                const auto& r=production->instanceAt(i);if(r.objectId!=35)continue;
                const auto player=production->instanceHandleAt(i);PackedObjectEvent binding;bool found=false;
                require(productionOwner->collisionEvent(field,player,found,binding)&&found,"field/player inherited binding");
                lastWindEvent={field,player,4,35,binding.ownerObjectId,binding.codeId};saved=true;break;
            }
            const auto* fieldRecord=productionOwner->resolve(field);require(fieldRecord,"field remains live");
            InstanceHandle vulnerable,immune,outside;
            require(production->spawnSkeleton(fieldSkeletons,int(fieldRecord->x),int(fieldRecord->y),vulnerable)&&
                production->spawnSkeleton(fieldSkeletons,int(fieldRecord->x),int(fieldRecord->y),immune),"birth mixed field-contact skeletons");
            require(production->spawnSkeleton(fieldSkeletons,-10000,int(fieldRecord->y),outside),"birth nonoverlapping field control");
            require(fieldSkeletons.dispatch(immune,[](auto& actor,auto&,bool&){actor.timefieldImmune=true;return true;}),"set source field immunity");
            SoundPlayEnvironment environment;
            require(saved&&windPass->runSkeletonContacts(*production,fieldSkeletons,
                [&](int sprite,unsigned& count){return sprites.frameCount(sprite,count);},environment,audio,
                [](auto dy,auto dx,auto& angle){return portableAtan(dy,dx,angle);}),
                "wind contacts join collected dither/wall stream without a family pass");
            require(fieldSkeletons.instances().find(vulnerable)->state.health==0&&
                fieldSkeletons.instances().find(immune)->state.health==60&&
                fieldSkeletons.instances().find(outside)->state.health==60,
                "same mixed pass applies inherited field death and preserves immune skeleton");
            require(!fieldSkeletons.instances().find(vulnerable)->destroyRequested&&
                !productionOwner->resolve(fieldSkeletons.worldHandle(vulnerable))->destroyRequested,
                "field Collision changes health without prematurely running death or retirement");
            require(fieldSkeletons.clear(),"retire field-contact fixtures before next room");
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
            production->overlapSound(8)==record->windSound&&production->timeFieldWindVoice()==900&&audio.plays==1&&audio.gains==2&&
            record->sound.sfxIndex==production->overlapSound(8)&&record->sound.sndsnd==900&&record->sound.chanNum==8,
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
        require(production->timeFieldWindVoice()==-1&&production->overlapSound(8)==0,
            "full reset clears global wind and UI channel rather than retaining an external voice");
        production->registerLevelExit(int(production->mainPlayerY()));
        require(production->playerTimeFieldInstance(x,y,selected)&&selected.handle.generation,
            "new run supplies an exit field to establish its own wind");
        dispatchWind(selected.handle);
        require(production->timeFieldWindVoice()==900&&audio.plays==2,
            "new run wind contact uses newly initialized global voice");
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
        require(production->randomDrawCount()==entryDraws&&audio.queries==queries+1&&audio.plays==2&&
            productionOwner->resolve(selected.handle)->windSound==-1&&productionOwner->resolve(selected.handle)->sound.sndsnd==-1,
            "inherited entrance wind respects shared playing voice without assigning windsnd or consuming RNG");
        audio.playing=false;
        require(production->dispatchTimeFieldWind(lastWindEvent,MotionBinary64::fromInteger(1),audio)&&
            production->randomDrawCount()==entryDraws+1&&audio.plays==3&&
            productionOwner->resolve(selected.handle)->windSound==production->overlapSound(8)&&
            productionOwner->resolve(selected.handle)->sound.sfxIndex==production->overlapSound(8)&&
            productionOwner->resolve(selected.handle)->sound.sndsnd==900&&production->timeFieldWindVoice()==900,
            "expired shared voice permits inherited entrance handler to choose and store its own sound");
        ++lastWindEvent.self.generation;const auto beforeInvalid=audio.queries;
        require(!production->dispatchTimeFieldWind(lastWindEvent,MotionBinary64::fromInteger(1),audio)&&
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
            InstanceLifecycleBinding::bindQueryDestruction(clockLife));
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
                    if(clock->levelResultRequested()){
                        const auto controller=clockLife.handles.at(100065);
                        require(clock->enterLevelClear(0,heartGoal),"enter collision-owned level clear");
                        require(clockLife.resultBirths==1&&clockLife.handles.count(102522)&&
                            !clockLife.handles.count(100855)&&
                            clockLife.handles.at(100065).slot==controller.slot&&
                            clockLife.handles.at(100065).generation==controller.generation,
                            "Result registers in the live owner while the original controller handle survives");
                    }
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
    for(unsigned failure=1;failure<=4;++failure){
        auto wipeStorage=std::make_unique<Storage>();
        require(wipeStorage->families.bindInstanceFlags(objects),"wipe failure family fixture");
        RecordingLifecycle wipeLife({wipeStorage->owner,wipeStorage->properties,objects,sprites,wipeStorage->families,{}, {}});
        auto wipe=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
            InstanceLifecycleBinding::bindQueryDestruction(wipeLife));
        wipeLife.runtime=wipe.get();wipeLife.masks=&masks;
        wipe->skipToGameForTesting();wipe->registerLevelExit(static_cast<int>(wipe->mainPlayerY()));
        for(unsigned tick=0;tick<110&&!wipe->levelResultRequested();++tick)wipe->step({},false);
        require(wipe->enterLevelClear(0,false),"wipe failure fixture enters Result");
        bool heart=false;
        for(unsigned tick=0;tick<120;++tick){bool fire=true;
            require(wipe->stepLevelClear(fire,false,false,heart,[](auto,unsigned){}),"wipe fixture approaches Alarm 0");}
        const auto id=wipe->nextRuntimeInstanceId();const auto draws=wipe->randomDrawCount();
        wipeLife.rejectWipeStage=failure<4?failure:0;bool fire=true;unsigned events=0;
        if(failure==4)const_cast<InstanceRecord*>(wipe->instance(100065))->alarms.set(1,1);
        require(!wipe->stepLevelClear(fire,false,false,heart,[&](auto,unsigned){++events;})&&
            !wipe->effectsSucceeded()&&wipeLife.wipeBirths==1&&wipeLife.wipeWrites==(failure<4?failure-1:2)&&
            wipe->nextRuntimeInstanceId()==id+1&&wipe->randomDrawCount()==draws,
            "wipe failure retains numeric allocation and stops before feedback or RNG");
        const auto* partial=wipe->instance(id);
        if(failure<4){
            require(events==0&&partial&&partial->objectId==68&&partial->x==(failure==3?80:0)&&partial->y==0&&
                partial->alarms.get(0)==-1&&!wipe->exitDither().active&&wipe->levelClear().selection.alarm1==-1,
                "wipe failure retains only the accepted Create prefix and does not arm either timer");
        }else{
            require(events==2&&partial&&partial->x==80&&partial->y==wipe->exitDither().y&&
                partial->alarms.get(0)==15&&wipe->exitDither().active&&
                wipe->levelClear().selection.alarm1==90&&wipe->instance(100065)->alarms.get(1)==0,
                "Result Alarm 0 precedes controller Alarm 1; unsupported event stops before Result Alarm 1 and Step");
        }
        const auto tick=wipe->tick();
        require(!wipe->stepLevelClear(fire,false,false,heart,[](auto,unsigned){std::abort();})&&
            wipe->tick()==tick&&wipeLife.wipeBirths==1,"failed wipe update cannot be retried");
    }
    for(bool rejectPosition:{false,true}){
        auto resultStorage=std::make_unique<Storage>();
        require(resultStorage->families.bindInstanceFlags(objects),"Result failure family fixture");
        RecordingLifecycle resultLife({resultStorage->owner,resultStorage->properties,objects,sprites,resultStorage->families,{}, {}});
        auto result=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
            InstanceLifecycleBinding::bindQueryDestruction(resultLife));
        resultLife.runtime=result.get();resultLife.masks=&masks;
        result->skipToGameForTesting();
        result->registerLevelExit(static_cast<int>(result->mainPlayerY()));
        for(unsigned tick=0;tick<110&&!result->levelResultRequested();++tick)result->step({},false);
        require(result->effectsSucceeded()&&result->levelResultRequested(),"Result failure fixture reaches handoff");
        const auto draws=result->randomDrawCount();
        resultLife.rejectResultBirth=!rejectPosition;
        resultLife.rejectResultPosition=rejectPosition;
        require(!result->enterLevelClear(0,false)&&!result->effectsSucceeded()&&!result->roomLoadSucceeded()&&
            !result->levelClearActive()&&resultLife.resultBirths==1&&result->randomDrawCount()==draws,
            "failed Result birth/position prevents Create RNG and latches the partial room transition");
        require(!result->instance(100855)&&result->instance(100065)&&result->instance(102522),
            "failed Result birth retains the committed retirement prefix and partial new record");
        require(result->instance(102522)->y==144&&resultLife.resultWrites==unsigned(rejectPosition)&&
            resultLife.handles.count(102522)==unsigned(rejectPosition),
            "Create publication failure retains birth but not the rejected position write");
        require(!result->enterLevelClear(0,false)&&resultLife.resultBirths==1&&result->randomDrawCount()==draws,
            "terminal Result entry cannot replay retirement or Create");
    }
    for(unsigned failure=1;failure<=4;++failure){
        auto failedStorage=std::make_unique<Storage>();
        require(failedStorage->families.bindInstanceFlags(objects),"start failure family fixture");
        RecordingLifecycle failedLifecycle({failedStorage->owner,failedStorage->properties,objects,sprites,failedStorage->families,{}, {}});
        failedLifecycle.rejectStartStage=failure;
        auto failed=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
            InstanceLifecycleBinding::bindMaskDestruction(failedLifecycle));
        failed->skipToGameForTesting();
        const auto* partial=failed->instance(101097);
        require(!failed->effectsSucceeded()&&partial&&partial->scaleX==(failure==1?1.0F:2.0F)&&
            partial->scaleY==(failure>=3?2.0F:1.0F)&&failed->randomDrawCount()==(failure>=3?331u:330u),
            "start failure preserves accepted scales and samples radius only before retirement");
        require(failedLifecycle.startWrites==(failure==1?1u:2u)&&failedLifecycle.startRetires==(failure==3?1u:0u),
            "start failure stops subsequent publication");
        require(failedLifecycle.startMarks==(failure>=3?1u:0u)&&partial->destroyRequested==(failure==3),
            "failed mark never publishes a record flag; failed cleanup retains the accepted mark");
        const auto digest=failed->stateDigest();
        failed->step({},false);
        require(failed->stateDigest()==digest,"failed start Create remains terminal");
    }
    for(unsigned failure=1;failure<=4;++failure){
        auto failedStorage=std::make_unique<Storage>();
        require(failedStorage->families.bindInstanceFlags(objects),"failed-birth family fixture");
        RecordingLifecycle failedLifecycle({failedStorage->owner,failedStorage->properties,objects,sprites,failedStorage->families,{}, {}});
        auto failed=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
            InstanceLifecycleBinding::bindMaskDestruction(failedLifecycle));
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
