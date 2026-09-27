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
    explicit RecordingLifecycle(Lifecycle base):Lifecycle(base){}
    bool birth(InstanceHandle h,const InstanceRecord& r){
        if(!Lifecycle::birth(h,r)){std::fprintf(stderr,"birth failed: object=%d id=%u\n",r.objectId,r.instanceId);return false;}
        require(handles.emplace(r.instanceId,h).second,"duplicate live instance ID");ids[h.slot]=r.instanceId;++births;return true;
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
    runtime->reset(PlatformProfile::nonDesktop);runtime->skipToGameForTesting();
    require(runtime->effectsSucceeded()&&runtime->roomLoadSucceeded()&&runtime->deferredCommandsSucceeded()&&
            !storage->owner.resolve(player)&&lifecycle.handles.size()==runtime->activeInstanceCount(),
            "room reset retires queried identities before new births reuse slots");
    std::printf("Live resource owner: %u births, %u retirements, %u position/activation checks\n",
        lifecycle.births,lifecycle.retires,checks);
    std::printf("Live precise terrain queries: %u hits and one empty query\n",queries);
}
