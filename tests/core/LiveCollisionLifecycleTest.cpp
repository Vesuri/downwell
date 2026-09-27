#include "core/StartupRuntime.h"
#include "core/RoomPack.h"
#include "core/StoredCollisionLifecycle.h"
#include "core/RunnerObjectFamilies.h"
#include "core/SmokeDirection.h"
#include "core/SmokeTrig.h"
#include "core/GameplayContactProfileInput.h"
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
    explicit RecordingLifecycle(Lifecycle base):Lifecycle(base){}
    bool birth(InstanceHandle h,const InstanceRecord& r){
        if((rejectGoalStage==1&&r.objectId==74)||(rejectGoalStage==2&&r.objectId==255))return false;
        if(r.objectId==255)require(r.scaleX==1.0F&&r.scaleY==1.0F,"goal birth precedes Create scales");
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
        return Lifecycle::transform(h,r,field,value,alpha,blend);
    }
    bool retire(InstanceHandle h,const InstanceRecord& r){
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
    auto storage=std::make_unique<Storage>();require(storage->families.bindInstanceFlags(objects),"bind full source families");
    RecordingLifecycle lifecycle({storage->owner,storage->properties,objects,sprites,storage->families,{}, {}});
    auto runtime=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
        InstanceLifecycleBinding::bindMasks(lifecycle));
    auto plain=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns);
    runtime->skipToGameForTesting();plain->skipToGameForTesting();
    unsigned checks=0;
    for(unsigned tick=0;tick<1800;++tick){
        const auto input=gameplayContactProfileInput(tick,runtime->mainPlayerX(),runtime->mainPlayerY());
        runtime->step(input,false);plain->step(input,false);
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
