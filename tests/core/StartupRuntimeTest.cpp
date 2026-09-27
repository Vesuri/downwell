#include "core/StartupRuntime.h"
#include <memory>
#include "core/BuilderPatternPack.h"
#include "core/ObjectPack.h"
#include "core/RoomPack.h"
#include "core/GameplayContactProfileInput.h"
#include "core/EnemyPresentation.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

void stepMany(downwell::StartupRuntime& runtime, int count) {
    for (int i = 0; i < count; ++i)
        runtime.step(false);
}

struct LifecycleAudit {
    struct Entry {std::uint32_t generation=0,id=0;bool live=false;};
    std::array<Entry,downwell::StartupRuntime::instanceCapacity> entries{};
    unsigned births=0,retires=0,activations=0,inactivations=0,memberships=0,moves=0,sprites=0,spriteChanges=0,indices=0,transforms=0,calls=0,fail=0;
    unsigned walkingTransforms=0;
    unsigned frogTransforms=0;
    unsigned batTransforms=0;
    unsigned boxRemovals=0;
    bool rejectBoxRemoval=false;
    bool rejectBatScale=false;
    bool rejectBatMask=false;
    unsigned masks=0;
    bool mask(downwell::InstanceHandle h,const downwell::InstanceRecord& r,int next){
        ++calls;require(matches(h,r)&&r.active&&r.objectId==116&&r.batActivity.flying&&next==r.spriteId,
                "host bat mask uses already-published flying sprite");
        if(rejectBatMask)return false;
        ++masks;return true;
    }
    bool transform(downwell::InstanceHandle h,const downwell::InstanceRecord& r,downwell::RunnerTransformField field,
                   downwell::RunnerImageValue value,downwell::RunnerImageValue alpha,std::uint32_t blend){
        ++calls;require(matches(h,r)&&r.active&&value.promoted().valid()&&alpha.bits==0x3f800000u&&blend==0xffffffu,
                        "host transform resolves current opaque-white actor");
        if(r.objectId==111||r.objectId==112){
            require(field==downwell::RunnerTransformField::scaleX&&r.spriteId==downwell::enemyPresentationSprite(
                        r.objectId,r.walker.grounded,r.auxiliaryState)&&r.x==r.walker.publishedX&&r.y==r.walker.publishedY,
                    "host walker scale sees selected sprite before final position publication");
            ++walkingTransforms;
        }
        if(r.objectId==116){
            require(field==downwell::RunnerTransformField::scaleX,"pursuit publishes only X scale");
            ++batTransforms;
            if(rejectBatScale)return false;
        }
        if(r.objectId==113){
            require(field==downwell::RunnerTransformField::scaleX,"frog publishes horizontal scale");
            ++frogTransforms;
        }
        if(fail==9)return false;
        ++transforms;return true;
    }
    bool imageIndex(downwell::InstanceHandle h,const downwell::InstanceRecord& r,downwell::RunnerImageValue value){
        ++calls;require(matches(h,r)&&r.active&&value.promoted().valid(),"animation publication resolves a live actor");
        if(fail==8)return false;
        ++indices;return true;
    }
    bool sprite(downwell::InstanceHandle h,const downwell::InstanceRecord& r,int next){
        ++calls;require(matches(h,r)&&r.active,"sprite setter resolves an active published lifetime");
        if(fail==7)return false;
        ++sprites;if(next!=r.spriteId)++spriteChanges;return true;
    }
    bool move(downwell::InstanceHandle h,const downwell::InstanceRecord& r,
              downwell::RunnerImageValue x,downwell::RunnerImageValue y){
        ++calls;require(matches(h,r)&&r.active,"movement resolves an active published lifetime");
        std::int32_t px,py;
        require(x.promoted().roundPixel(px)&&y.promoted().roundPixel(py),"published enemy coordinates are representable");
        if(fail==6)return false;
        ++moves;return true;
    }
    bool matches(downwell::InstanceHandle h,const downwell::InstanceRecord& r)const{
        return h.slot<entries.size()&&entries[h.slot].live&&entries[h.slot].generation==h.generation&&entries[h.slot].id==r.instanceId;
    }
    bool birth(downwell::InstanceHandle h,const downwell::InstanceRecord& r){
        ++calls;if(fail==1)return false;
        require(h.slot<entries.size()&&!entries[h.slot].live&&h.generation>entries[h.slot].generation,
                "live runtime publishes births before reusing generation-checked slots");
        entries[h.slot]={h.generation,r.instanceId,true};++births;return true;
    }
    bool retire(downwell::InstanceHandle h,const downwell::InstanceRecord& r){
        ++calls;require(matches(h,r),"retirement still resolves the published lifetime");
        if(fail==2)return false;
        entries[h.slot].live=false;++retires;return true;
    }
    bool activation(downwell::InstanceHandle h,const downwell::InstanceRecord& r,bool active){
        ++calls;require(matches(h,r)&&active!=r.active,"activation publication sees the old live flag");
        if(!active&&r.objectId==92){
            ++boxRemovals;
            if(rejectBoxRemoval)return false;
        }
        if(fail==(active?3u:5u))return false;
        if(active)++activations;else {
            require(!r.activationPending,"dead actor removal must not queue dormant reactivation");
            ++inactivations;
        }
        return true;
    }
    bool membership(downwell::InstanceHandle h,const downwell::InstanceRecord& r,bool){
        ++calls;require(matches(h,r),"reconciliation publishes a live generation");
        if(fail==4)return false;
        ++memberships;return true;
    }
    void verify(const downwell::StartupRuntime& runtime)const{
        unsigned count=0;
        for(const auto& e:entries)if(e.live){
            ++count;require(runtime.instance(e.id)!=nullptr,"external lifetime still exists in runtime");
        }
        require(count==runtime.activeInstanceCount(),"all stored runtime lifetimes have been published");
    }
};

void testLiveLifecycle(const downwell::RoomPack& rooms,const downwell::ObjectPack& objects,
                       const downwell::BuilderPatternPack& patterns){
    using Runtime=downwell::StartupRuntime;
    const auto healthy=[](const Runtime& r){return r.effectsSucceeded()&&r.roomLoadSucceeded()&&r.deferredCommandsSucceeded();};
    LifecycleAudit audit;
    auto indexed=std::make_unique<Runtime>(downwell::PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
        downwell::InstanceLifecycleBinding::bindTransforms(audit));
    auto plain=std::make_unique<Runtime>(downwell::PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns);
    indexed->skipToGameForTesting();plain->skipToGameForTesting();audit.verify(*indexed);
    for(unsigned tick=0;tick<1800;++tick){
        const auto input=downwell::gameplayContactProfileInput(tick,indexed->mainPlayerX(),indexed->mainPlayerY());
        indexed->step(input,false);plain->step(input,false);
        require(healthy(*indexed)&&indexed->stateDigest()==plain->stateDigest(),
                "lifecycle publication preserves complete live gameplay state and RNG");
        audit.verify(*indexed);
    }
    std::cout << "Live lifecycle births=" << audit.births << " retires=" << audit.retires
              << " activations=" << audit.activations << " inactivations=" << audit.inactivations
              << " memberships=" << audit.memberships << " moves=" << audit.moves
              << " sprites=" << audit.sprites << " spriteChanges=" << audit.spriteChanges << " indices=" << audit.indices
              << " transforms=" << audit.transforms << '\n';
    require(audit.births>100&&audit.retires>2&&audit.memberships>0&&audit.inactivations>0&&audit.moves>0&&audit.sprites==audit.moves&&audit.indices>audit.sprites,
            "natural gameplay covers dynamic births, contact inactivation, retirement and reconciliation");
    indexed->reset(downwell::PlatformProfile::nonDesktop);audit.verify(*indexed);
    require(healthy(*indexed),"reset retires old lifetimes before creating a fresh room");
    for(bool reject:{false,true}){
        LifecycleAudit boxes;boxes.rejectBoxRemoval=reject;
        auto runtime=std::make_unique<Runtime>(downwell::PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
            downwell::InstanceLifecycleBinding::bind(boxes));
        runtime->skipToGameForTesting();
        for(unsigned tick=0;tick<1800&&healthy(*runtime);++tick)
            runtime->step(downwell::gameplayContactProfileInput(tick,runtime->mainPlayerX(),runtime->mainPlayerY()),false);
        require(boxes.boxRemovals>0,"natural contact tape reaches checked box removal");
        require(healthy(*runtime)!=reject,"box publication rejection is terminal");
        if(reject){
            require(runtime->mainBlocksDestroyed()==0,"rejected first box removal does not count a destroyed block");
            const auto calls=boxes.calls;const auto tick=runtime->tick();
            runtime->step(false);
            require(boxes.calls==calls&&runtime->tick()==tick,"box failure remains latched");
        }
    }
    for(unsigned reject:{0u,1u,2u}){
        LifecycleAudit checked;
        auto runtime=std::make_unique<Runtime>(downwell::PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
            downwell::InstanceLifecycleBinding::bindMasks(checked));
        runtime->skipToGameForTesting();
        std::uint32_t id=0;
        int startX=0,startY=0;
        for(unsigned tick=0;tick<600&&!id;++tick){
            runtime->step(downwell::gameplayContactProfileInput(tick,runtime->mainPlayerX(),runtime->mainPlayerY()),false);
            for(unsigned i=0;i<runtime->activeInstanceCount();++i){
                auto& r=const_cast<downwell::InstanceRecord&>(runtime->instanceAt(i));
                if(!r.active||r.health<=0||
                   (r.objectId!=111&&r.objectId!=112&&r.objectId!=113&&r.objectId!=116&&r.objectId!=117&&r.objectId!=119))continue;
                // Controlled pursuit above the player, not a natural encounter.
                startX=static_cast<int>(runtime->mainPlayerX());
                startY=static_cast<int>(runtime->mainPlayerY())-32;
                r.objectId=116;r.spriteId=153;r.maskId=-1;r.health=15;r.x=startX;r.y=startY;
                r.preciseX=startX;r.preciseY=startY;r.motionInitialized=true;
                r.velocityX=r.velocityY=0;r.scaleX=-1;r.direction=0;r.moveSpeed=1.25f;
                r.batDirectionFraction=0;r.batActivity={true,true,0};r.batHit={};r.alarms={};
                id=r.instanceId;break;
            }
        }
        require(id!=0,"host pursuit fixture has an owned enemy");
        checked.rejectBatScale=reject==1;checked.rejectBatMask=reject==2;
        const auto before=checked.batTransforms;
        runtime->step(false);
        const auto* bat=runtime->instance(id);
        if(!(bat&&healthy(*runtime)==!reject&&checked.batTransforms==before+1))
            std::cerr << "Bat fixture reject=" << reject << " healthy=" << healthy(*runtime)
                      << " transforms=" << checked.batTransforms-before << " active=" << (bat?bat->active:false)
                      << " eligible=" << (bat?bat->enemyStepEligible:false) << " vx=" << (bat?bat->velocityX:0)
                      << " direction=" << (bat?bat->direction:0) << '\n';
        require(bat&&healthy(*runtime)==!reject&&checked.batTransforms==before+1,
                "host pursuit reaches checked scale publication once");
        require(bat->maskId==(reject?-1:153),"host mask is stored only after accepted publication");
        if(reject==1)require(bat->preciseX==startX&&bat->preciseY==startY&&bat->velocityX!=0&&bat->scaleX==-1,
                "host scale failure retains calculated motion but skips integration and facing change");
    }
    for(int object:{111,112,113})for(int velocity:{0,640}){
        LifecycleAudit walking;
        auto runtime=std::make_unique<Runtime>(downwell::PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
            downwell::InstanceLifecycleBinding::bindTransforms(walking));
        runtime->skipToGameForTesting();
        std::uint32_t id=0;
        for(unsigned tick=0;tick<600&&!id;++tick){
            runtime->step(downwell::gameplayContactProfileInput(tick,runtime->mainPlayerX(),runtime->mainPlayerY()),false);
            for(unsigned i=0;i<runtime->activeInstanceCount();++i){
                auto& r=const_cast<downwell::InstanceRecord&>(runtime->instanceAt(i));
                if(!r.active||r.health<=0||
                   (r.objectId!=111&&r.objectId!=112&&r.objectId!=113&&r.objectId!=116&&r.objectId!=117&&r.objectId!=119))continue;
                // Controlled adapter fixture, not a naturally generated walker.
                r.objectId=object;r.health=5;r.auxiliaryState=1;r.scaleX=-1;
                r.alarms={};r.motionInitialized=true;
                r.frogMotionInitialized=false;r.grounded=false;
                r.preciseX=r.x;r.preciseY=r.y;r.velocityX=velocity/1280.0f;r.velocityY=0;
                r.walker.initialize(r.x,r.y,velocity,0,false);r.walker.facing=-1;
                id=r.instanceId;break;
            }
        }
        require(id!=0,"host walker fixture has a published enemy lifetime");
        const auto before=object==113?walking.frogTransforms:walking.walkingTransforms;
        runtime->step(false);
        const auto after=object==113?walking.frogTransforms:walking.walkingTransforms;
        require(healthy(*runtime)&&after==before+(velocity!=0?1u:0u),
                "host walker/frog publishes nonzero facing once and skips the zero-speed setter");
        require(velocity!=0||runtime->instance(id)->scaleX==-1,
                "host zero-speed walker preserves its existing mirrored artwork");
    }
    {
        LifecycleAudit changed;
        auto runtime=std::make_unique<Runtime>(downwell::PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
            downwell::InstanceLifecycleBinding::bindAnimation(changed));
        runtime->skipToGameForTesting();
        bool prepared=false;
        for(unsigned tick=0;tick<600&&!prepared;++tick){
            runtime->step(downwell::gameplayContactProfileInput(tick,runtime->mainPlayerX(),runtime->mainPlayerY()),false);
            for(unsigned i=0;i<runtime->activeInstanceCount();++i){
                auto& record=const_cast<downwell::InstanceRecord&>(runtime->instanceAt(i));
                if(!record.active||record.health<=0||
                   (record.objectId!=111&&record.objectId!=112&&record.objectId!=113&&
                    record.objectId!=116&&record.objectId!=117&&record.objectId!=119))continue;
                // Controlled setter fixture, not original-game pose evidence.
                record.spriteId=-1;prepared=true;break;
            }
        }
        require(prepared,"sprite-change fixture contains a published enemy");
        for(unsigned i=0;i<120&&!changed.spriteChanges;++i)runtime->step(false);
        require(healthy(*runtime)&&changed.spriteChanges>0,"live setter reports old and proposed sprite before publication");
        // Controlled wall-less snail fixture: an inactivated actor must not
        // issue later geometry/resource publications in this same Step.
        std::uint32_t snailId=0;
        for(unsigned i=0;i<runtime->activeInstanceCount();++i){
            auto& record=const_cast<downwell::InstanceRecord&>(runtime->instanceAt(i));
            if(!record.active||record.health<=0||
               (record.objectId!=111&&record.objectId!=112&&record.objectId!=113&&
                record.objectId!=116&&record.objectId!=117&&record.objectId!=119))continue;
            record.objectId=117;record.spriteId=177;record.x=240;record.y=5000;
            record.motionInitialized=false;record.snail={};snailId=record.instanceId;break;
        }
        require(snailId!=0,"snail retirement fixture has an existing lifetime");
        runtime->step(false);
        const auto* snail=runtime->instance(snailId);
        require(healthy(*runtime)&&snail&&!snail->active&&!snail->activationPending,
                "wall-less snail stops before post-retirement movement and sprite publication");
    }
    for(unsigned failure=0;failure<=9;++failure){
        LifecycleAudit failing;
        auto runtime=std::make_unique<Runtime>(downwell::PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
            downwell::InstanceLifecycleBinding::bindTransforms(failing));
        runtime->skipToGameForTesting();
        if(failure==0||failure==3||failure==9){
            // Controlled lifecycle fixture, not a natural combat encounter:
            // put an existing enemy into the activation frontier's dormant set,
            // or schedule a crawler alarm before rejected Step angle publication.
            bool prepared=false;
            for(unsigned warmup=0;warmup<600&&!prepared;++warmup){
            for(unsigned i=0;i<runtime->activeInstanceCount();++i){
                auto& record=const_cast<downwell::InstanceRecord&>(runtime->instanceAt(i));
                if(record.objectId!=111&&record.objectId!=112&&record.objectId!=113&&record.objectId!=116&&
                   record.objectId!=117&&record.objectId!=119)continue;
                record.active=false;record.activationPending=true;record.x=240;record.y=1200;
                if(failure==9){
                    record.objectId=119;record.active=true;record.activationPending=false;record.health=5;
                    record.crawler={};record.alarms.set(0,1);
                }
                prepared=true;break;
            }
            if(!prepared)runtime->step(downwell::gameplayContactProfileInput(warmup,runtime->mainPlayerX(),runtime->mainPlayerY()),false);
            }
            require(prepared,"activation fixture has a real published enemy lifetime");
        }
        failing.fail=failure;
        for(unsigned tick=0;tick<1800&&healthy(*runtime);++tick)
            runtime->step(downwell::gameplayContactProfileInput(tick,runtime->mainPlayerX(),runtime->mainPlayerY()),false);
        if(failure==0){
            require(healthy(*runtime)&&failing.activations>0,"accepted activation reaches the live callback");
            continue;
        }
        require(!healthy(*runtime),"every rejected live publication stops the runtime");
        const auto tick=runtime->tick();const auto calls=failing.calls;
        runtime->step(false);
        require(runtime->tick()==tick&&failing.calls==calls,"failed publication stays latched on subsequent updates");
    }
    LifecycleAudit failedReset;
    auto runtime=std::make_unique<Runtime>(downwell::PlatformProfile::desktop,nullptr,nullptr,2,nullptr,
        downwell::InstanceLifecycleBinding::bind(failedReset));
    failedReset.fail=2;const auto births=failedReset.births;
    runtime->reset(downwell::PlatformProfile::desktop);
    require(!healthy(*runtime)&&failedReset.births==births,"failed reset retirement cannot publish replacement births");
    LifecycleAudit failedBirth;failedBirth.fail=1;
    auto rejected=std::make_unique<Runtime>(downwell::PlatformProfile::desktop,&rooms,&objects,2,&patterns,
        downwell::InstanceLifecycleBinding::bind(failedBirth));
    require(!healthy(*rejected)&&failedBirth.calls==1&&rejected->implementedCodeCalls()==0,
            "initial birth rejection precedes all game callbacks and later room births");
}

void testDesktopBootState() {
    downwell::StartupRuntime runtime;
    require(runtime.roomId() == 7, "rmInit is the entry room");
    require(runtime.controllerPersistent(), "controller is persistent");
    require(runtime.activeInstanceCount() == 2, "rmInit creates two instances");
    require(runtime.hasInstance(100065), "controller uses recovered instance ID");
    require(runtime.hasInstance(100066), "transition object uses recovered instance ID");
    require(runtime.initTimes() == 1, "scrInitialize increments initTimes");
    require(runtime.showSplash() == 1, "desktop enables splash sequence");
    require(!runtime.roomDestinationSet(), "destination starts unset");
    require(runtime.cameraViewWidth() == 160 && runtime.cameraViewHeight() == 284,
            "rmInit creation code configures recovered camera size");
}

void testFailedRoomLoadStopsUpdates() {
    downwell::RoomPack invalidPack(nullptr, 0);
    downwell::StartupRuntime runtime(downwell::PlatformProfile::desktop, &invalidPack);
    require(!runtime.roomLoadSucceeded(), "invalid room pack reports failed entry");
    const auto tick = runtime.tick();
    const auto instances = runtime.activeInstanceCount();
    const auto calls = runtime.implementedCodeCalls();
    for (unsigned i = 0; i < 4; ++i) runtime.step(i % 2 != 0);
    require(runtime.tick() == tick && runtime.activeInstanceCount() == instances &&
                runtime.implementedCodeCalls() == calls && !runtime.roomDestinationSet(),
            "failed room entry stops subsequent ticks and dispatch without retry");
}

void testInitialDeferredFailureStopsRoomStart(const downwell::RoomPack* rooms = nullptr) {
    // Artificial bindings exercise the real runtime failure boundary, not the
    // original game's event mapping. Create requests an unavailable child;
    // Room Start would initialize the controller if dispatch wrongly continued.
    using Pack = downwell::ObjectPack;
    std::vector<std::uint8_t> bytes(Pack::headerSize + 38 * Pack::objectRecordSize +
                                    2 * Pack::bindingRecordSize, 0);
    const auto put = [&](std::size_t offset, std::uint64_t value, unsigned size = 4) {
        for (unsigned i=0;i<size;++i) bytes[offset+i]=std::uint8_t(value>>(8*i));
    };
    bytes[0]='D'; bytes[1]='W'; bytes[2]='O'; bytes[3]='B';
    put(4,Pack::formatVersion,2); put(6,Pack::headerSize,2);
    put(8,Pack::manifestSchema); put(12,38); put(16,2);
    put(20,Pack::sourceHashPrefix,8);
    for(unsigned i=0;i<38;++i) {
        const auto offset=Pack::headerSize+i*Pack::objectRecordSize;
        put(offset,i); put(offset+4,0xffffffff); put(offset+8,0xffffffff);
        put(offset+12,0xffffffff);
        if(i==37) put(offset+28,2);
    }
    const auto binding=Pack::headerSize+38*Pack::objectRecordSize;
    put(binding,0); put(binding+8,714); // Create: enqueue objDitherFade (382).
    put(binding+Pack::bindingRecordSize,7);
    put(binding+Pack::bindingRecordSize+4,4); // Other / Room Start.
    put(binding+Pack::bindingRecordSize+8,869);
    const Pack objects(bytes.data(),bytes.size());
    require(objects.valid(), "failure-injection object pack is structurally valid");
    auto runtime=std::make_unique<downwell::StartupRuntime>(
        downwell::PlatformProfile::desktop,rooms,&objects);
    for(unsigned reset=0;reset<2;++reset) {
        require(!runtime->roomLoadSucceeded() && !runtime->deferredCommandsSucceeded(),
                "initial deferred birth failure marks room entry failed");
        require(runtime->initTimes()==0 && runtime->ditherFadeSpawnRequests()==1 &&
                runtime->activeInstanceCount()==2,
                "failed entry retains prior work but never dispatches Room Start");
        const auto digest=runtime->stateDigest();
        runtime->step(true);
        require(runtime->stateDigest()==digest, "failed initial entry cannot advance or retry");
        runtime->reset(downwell::PlatformProfile::desktop);
    }
}

void testNonDesktopSkipsSplash() {
    downwell::StartupRuntime runtime(downwell::PlatformProfile::nonDesktop);
    runtime.step(false);
    require(runtime.roomId() == 20, "non-desktop reaches rmMenu immediately");
    require(runtime.roomDestination() == 20, "rmMenu is selected deterministically");
    require(runtime.achievementLoginCalls() == 1, "login is requested once");
    require(runtime.activeInstanceCount() == 1, "room transition removes room-owned instance");
    require(runtime.hasInstance(100065), "persistent controller survives room transition");
    require(!runtime.hasInstance(100066), "rmInit transition object is removed");
    require(runtime.backgroundMusicId() == 193,
            "rmMenu selects recovered bgmOverworld resource");
    require(runtime.controlsLocked(), "rmMenu locks player control");
    require(runtime.soundGainRequests() == 1 && runtime.soundPlayRequests() == 1,
            "rmMenu emits deterministic audio requests");
    require(runtime.ditherFadeSpawnRequests() == 1,
            "rmMenu records its deferred ditherFade spawn");
}

void testRecoveredDesktopTiming() {
    downwell::StartupRuntime runtime;
    runtime.step(false);
    require(runtime.alarm0() == 10, "first End Step arms ten-tick splash alarm");

    stepMany(runtime, 9);
    require(runtime.showSplash() == 1 && runtime.alarm0() == 1,
            "alarm has not fired a tick early");
    runtime.step(false);
    require(runtime.showSplash() == 2 && runtime.alarm0() == 180,
            "first splash alarm advances to stage two");

    stepMany(runtime, 180);
    require(runtime.showSplash() == 3 && runtime.alarm0() == 180,
            "stage three uses normal splash speed");
    stepMany(runtime, 180);
    require(runtime.showSplash() == 4 && runtime.alarm0() == 240,
            "stage four uses extended delay");
    stepMany(runtime, 240);
    require(runtime.showSplash() == 5 && runtime.alarm0() == 60,
            "later splash stages use short delay");
    require(runtime.splashDitherFrame() == 11,
            "controls fade begins on the transparent dither frame");
    stepMany(runtime, 1);
    require(runtime.splashDitherFrame() == 10,
            "controls fade follows the recovered 0.3-frame decrement");
    stepMany(runtime, 59);
    require(runtime.showSplash() == 6 && runtime.roomId() == 7,
            "stage six remains in rmInit");
    require(runtime.splashDitherFrame() == 0,
            "fade-out reaches the opaque dither endpoint");
    stepMany(runtime, 60);
    require(runtime.showSplash() == 0 && runtime.roomId() == 20,
            "stage seven transitions to rmMenu");
    require(runtime.tick() == 731, "desktop automatic transition tick is stable");
}

void testAdvanceInputUsesPressEdges() {
    downwell::StartupRuntime runtime;
    runtime.step(false);
    for (int expected = 2; expected <= 6; ++expected) {
        runtime.step(true);
        require(runtime.showSplash() == expected, "advance press increments splash stage");
        runtime.step(false);
    }
    runtime.step(true);
    require(runtime.inMenu(), "six advance presses reach rmMenu");
}

void testGeneratedPackEntersRecoveredMenu(const char* roomPath,
                                          const char* objectPath,
                                          const char* builderPatternPath) {
    std::ifstream input(roomPath, std::ios::binary);
    const std::vector<std::uint8_t> bytes(
        (std::istreambuf_iterator<char>(input)),
        std::istreambuf_iterator<char>());
    const downwell::RoomPack pack(bytes.data(), bytes.size());
    require(pack.valid(), "generated room pack is valid");
    testInitialDeferredFailureStopsRoomStart(&pack);

    std::vector<std::uint8_t> objectBytes;
    if (objectPath != nullptr) {
        std::ifstream objectInput(objectPath, std::ios::binary);
        objectBytes.assign(std::istreambuf_iterator<char>(objectInput),
                           std::istreambuf_iterator<char>());
    }
    const downwell::ObjectPack objectPack(objectBytes.data(), objectBytes.size());
    const downwell::ObjectPack* objects = objectPack.valid() ? &objectPack : nullptr;
    if (objectPath != nullptr)
        require(objects != nullptr, "generated object pack is valid");
    if (objects) {
        for (int id : {60,92,93,111,112,113,116,117,119,289}) {
            downwell::PackedObjectEvent event;
            require(!objects->findEvent(id,1,0,event) && !objects->findEvent(id,12,0,event),
                    "supported offscreen destruction has no inherited Destroy or Clean Up callback");
        }
    }

    std::vector<std::uint8_t> builderPatternBytes;
    if (builderPatternPath != nullptr) {
        std::ifstream patternInput(builderPatternPath, std::ios::binary);
        builderPatternBytes.assign(std::istreambuf_iterator<char>(patternInput),
                                   std::istreambuf_iterator<char>());
    }
    const downwell::BuilderPatternPack builderPatterns(
        builderPatternBytes.data(), builderPatternBytes.size());
    const downwell::BuilderPatternPack* patterns =
        builderPatterns.valid() ? &builderPatterns : nullptr;
    if (builderPatternPath != nullptr)
        require(patterns != nullptr, "generated builder-pattern pack is valid");

    if(objects&&patterns)testLiveLifecycle(pack,*objects,*patterns);

    if (objects != nullptr) {
        auto cached = std::make_unique<downwell::StartupRuntime>(
            downwell::PlatformProfile::nonDesktop, &pack, objects,
            downwell::StartupRuntime::defaultRandomSeed, patterns);
        std::array<std::uint64_t,180> checkpoints{};
        for (unsigned i=0;i<checkpoints.size();++i) {
            cached->step(i%32==0);
            checkpoints[i]=cached->stateDigest();
        }
        cached->reset(downwell::PlatformProfile::nonDesktop);
        for (unsigned i=0;i<checkpoints.size();++i) {
            cached->step(i%32==0);
            require(cached->stateDigest()==checkpoints[i],
                    "alarm binding cache reset preserves complete deterministic startup state");
        }
    }

    downwell::StartupRuntime runtime(
        downwell::PlatformProfile::nonDesktop, &pack, objects,
        downwell::StartupRuntime::defaultRandomSeed, patterns);
    require(runtime.roomLoadSucceeded(), "packed rmInit loads");
    require(runtime.activeInstanceCount() == 2,
            "packed rmInit creates two instances");
    if (objects != nullptr) {
        require(runtime.recoveredEventBindingsActive(),
                "recovered event bindings are active");
        require(runtime.implementedCodeCalls() == 3,
                "rmInit room code and both Create handlers route by code ID");
        require(runtime.unimplementedCodeCalls() > 0,
                "unsupported recovered startup code is measured explicitly");
    }

    runtime.step(false);
    require(runtime.inMenu(), "packed startup reaches rmMenu");
    require(runtime.roomLoadSucceeded(), "packed rmMenu loads");
    require(runtime.activeInstanceCount() == (objects != nullptr ? 201U : 193U),
            "rmMenu contains recovered, persistent, and translated instances");
    require(runtime.hasInstance(100065), "controller persists into packed rmMenu");
    require(!runtime.hasInstance(100066), "rmInit object leaves packed rmMenu");

    const downwell::InstanceRecord* player = runtime.instance(104218);
    require(player != nullptr, "first recovered rmMenu player exists");
    require(player->objectId == 35, "rmMenu player has recovered object ID");
    require(player->roomId == 20, "rmMenu player belongs to recovered room");
    require(runtime.randomSeed() == downwell::StartupRuntime::defaultRandomSeed &&
                runtime.randomDrawCount() == 77,
            "menu creation preserves all recovered shared-stream RNG draws");
    if (objects != nullptr)
        require(runtime.implementedCodeCalls() == 191,
                "room transition and recovered wall Create events route by code ID");
    if (objects != nullptr) {
        downwell::StartupRuntime edgeEntry(
            downwell::PlatformProfile::nonDesktop, &pack, objects,
            downwell::StartupRuntime::defaultRandomSeed, patterns);
        edgeEntry.step(false);
        stepMany(edgeEntry, 30);
        downwell::Input enterAtLeftEdge;
        enterAtLeftEdge.right = true;
        for (int tick = 0;
             tick < 160 && edgeEntry.menuPlayerX() < 300.0F;
             ++tick)
            edgeEntry.step(enterAtLeftEdge, false);
        downwell::Input noInput;
        for (int tick = 0;
             tick < 100 && !edgeEntry.menuTeleportActive();
             ++tick)
            edgeEntry.step(noInput, false);
        require(edgeEntry.menuTeleportActive() &&
                    edgeEntry.menuPlayerX() < 324.0F,
                "left-edge well entry reproduces the missed player-X title trigger");
        require(!edgeEntry.menuPlayerVisible(),
                "playerDraw hides the goal-stopped player during well entry");
        for (int tick = 0;
             tick < 180 && !edgeEntry.menuTitleRevealStarted();
             ++tick)
            edgeEntry.step(noInput, false);
        require(edgeEntry.menuTitleRevealStarted(),
                "camera centering starts the title after edge-entry goalStop");
        for (int tick = 0; tick < 300 && !edgeEntry.inPlayMenu(); ++tick)
            edgeEntry.step(noInput, false);
        require(edgeEntry.inPlayMenu(),
                "edge entry completes the title and well transition without input");

        require(runtime.instance(105186) != nullptr &&
                    runtime.instance(105186)->objectId == 66,
                "player Create spawns the depth meter first");
        require(runtime.instance(105187) != nullptr &&
                    runtime.instance(105187)->objectId == 64,
                "player Create spawns camMain second");
        const downwell::InstanceRecord* title = runtime.instance(105192);
        require(title != nullptr && title->objectId == 317 &&
                    title->x == 328 && title->y == 432 &&
                    title->imageSpeed == 0.0F,
                "well Create spawns the stationary first-boot title");
        const downwell::InstanceRecord* fade = runtime.instance(105193);
        require(fade != nullptr && fade->objectId == 382 && fade->roomId == 20 &&
                    fade->x == 0 && fade->y == 0,
                "rmMenu creates ditherFade with the authoritative runtime ID");
        require(fade->imageSpeed == 0.0F && fade->alarms.get(0) == 30,
                "ditherFade Create freezes animation and arms Alarm 0");
        require(runtime.nextRuntimeInstanceId() == 105194,
                "runtime allocator advances over all recovered Create spawns");
        require(runtime.deferredCommandsSucceeded(),
                "deferred room-creation commands apply successfully");

        stepMany(runtime, 30);
        fade = runtime.instance(105193);
        require(fade != nullptr && fade->imageSpeed == 0.2F,
                "ditherFade Alarm 0 starts recovered animation speed");
        require(!runtime.controlsLocked(),
                "ditherFade Alarm 0 restores player control");

        require(runtime.menuPlayerNapping(),
                "groundPlayerSet starts the menu player asleep");
        require(runtime.menuPlayerX() == 112.0F &&
                    runtime.menuPlayerY() == 512.0F &&
                    runtime.menuPlayerSpriteId() == 12,
                "the reference seed selects its recovered sleeping pose");
        downwell::Input wake;
        wake.right = true;
        runtime.step(wake, false);
        require(!runtime.menuPlayerNapping(),
                "the first enabled direction input wakes the player");
        require(runtime.menuPlayerSpriteId() == 31,
                "waking selects the default idle sprite");
        for (int tick = 0; tick < 10; ++tick)
            runtime.step(wake, false);
        require(runtime.menuPlayerX() > 122.9F && runtime.menuPlayerX() < 123.1F,
                "recovered ground acceleration reaches max speed over ten ticks");
        require(runtime.menuPlayerSpriteId() == 27,
                "horizontal movement selects the default run sprite");
        require(runtime.menuCameraViewX() > 32.0F,
                "free cam begins following the moving player");

        for (int tick = 0; tick < 105; ++tick)
            runtime.step(wake, false);
        title = runtime.instance(105192);
        require(title != nullptr && title->alarms.get(2) >= 0,
                "crossing beneath the title arms its recovered reveal delay");
        for (int tick = 0; tick < 50; ++tick)
            runtime.step(wake, false);
        title = runtime.instance(105192);
        require(title != nullptr && title->imageIndex == 9.0F &&
                    title->imageSpeed == 0.0F,
                "title reveal reaches and holds its final recovered frame");
        require(!runtime.menuPlayerGrounded() && runtime.menuPlayerY() >= 625.0F,
                "player falls through the recovered 64-pixel well opening");
        require(runtime.menuTeleportActive(),
                "touching the shaft trigger activates objTeleportMain");
        require(!runtime.menuPlayerVisible(),
                "teleporter goalStop suppresses the menu player sprite");
        for (int tick = 0; tick < 76; ++tick)
            runtime.step(wake, false);
        require(runtime.menuTitleReady(),
                "title completion eventually enables the teleporter sequence");

        for (int tick = 0; tick < 140 && !runtime.inPlayMenu(); ++tick)
            runtime.step(wake, false);
        require(runtime.inPlayMenu(),
                "EnterSequence acceleration reaches rmPlayMenu");
        require(runtime.hasInstance(100067),
                "rmPlayMenu creates its recovered PlayMenu instance");
        require(runtime.activeInstanceCount() == 2,
                "only the persistent controller and PlayMenu survive transition");
        downwell::Input released;
        for (int tick = 0; tick < 100; ++tick)
            runtime.step(released, false);
        require(runtime.playMenuYOffset() == 0.0F,
                "style menu entrance easing settles exactly");
        runtime.step(wake, false);
        require(runtime.playMenuCursor() == 1,
                "a right press advances the recovered style carousel");

        downwell::Input fire;
        fire.fire = true;
        runtime.step(fire, true);
        require(!runtime.playMenuSelected(),
                "confirming a locked style has no effect");
        downwell::Input left;
        left.left = true;
        runtime.step(left, false);
        require(runtime.playMenuCursor() == 0,
                "a left press returns to the unlocked default style");
        runtime.step(released, false);
        runtime.step(fire, true);
        require(runtime.playMenuSelected() && runtime.playStyle() == 0,
                "fire confirms and stores the unlocked style");
        require(runtime.playMenuPlayerDescent() > 0.0F &&
                    runtime.playMenuVerticalDitherY() == -310.0F,
                "confirmation starts the recovered falling dither transition");
        for (int tick = 0; tick < 63; ++tick)
            runtime.step(released, false);
        require(runtime.inPlayMenu() &&
                    runtime.playMenuVerticalDitherY() == 320.0F,
                "the vertical dither takes exactly 64 frames to cross");
        runtime.step(released, false);
        require(runtime.inMainRoom(),
                "the next frame enters the recovered rmMain room");
        require(runtime.roomLoadSucceeded(),
                "rmMain fits and loads from the recovered room pack");
        require(runtime.hasInstance(100855),
                "rmMain creates its authoritative player instance");
        require(runtime.mainPlayerX() == 240.0F &&
                    runtime.mainPlayerY() == 176.0F,
                "rmMain initializes player motion from the recovered spawn");
        require(runtime.mainBuilderInitialized() &&
                    runtime.mainBuilderY() == 704 &&
                    runtime.mainBuilderGenerationCount() == -30,
                "rmMain initializes the recovered first-boot builder prelude");
        require(runtime.mainBuilderRightRoom(0) >= 3 &&
                    runtime.mainBuilderRightRoom(0) <= 15 &&
                    runtime.mainBuilderRightRoom(1) >= 3 &&
                    runtime.mainBuilderRightRoom(1) <= 15 &&
                    std::abs(runtime.mainBuilderRightRoom(0) -
                             runtime.mainBuilderRightRoom(1)) > 2,
                "builder creation consumes the stream for valid side-room rows");
        downwell::Input fallRight;
        fallRight.right = true;
        for (int tick = 0; tick < 10; ++tick)
            runtime.step(fallRight, false);
        require(runtime.mainPlayerX() > 250.9F &&
                    runtime.mainPlayerX() < 251.1F,
                "air control uses recovered acceleration over ten ticks");
        require(runtime.mainPlayerY() > 186.9F &&
                    runtime.mainPlayerY() < 187.1F,
                "rmMain gravity advances the initial fall deterministically");
        require(runtime.mainPlayerSpeedX() > 1.9F &&
                    runtime.mainPlayerSpeedX() < 2.1F &&
                    runtime.mainPlayerSpeedY() > 1.9F &&
                    runtime.mainPlayerSpeedY() < 2.1F,
                "air and gravity speeds retain recovered subpixel values");
        require(runtime.mainPlayerSpriteId() == 37 &&
                    runtime.mainPlayerImageIndex() == 4.0F,
                "fall speed selects the recovered airborne animation frame");
        require(runtime.mainCameraViewY() > 50.0F &&
                    runtime.mainCameraViewY() < 70.0F,
                "rmMain camera follows the fall with recovered smoothing and bias");
        for (int tick = 0; tick < 20; ++tick)
            runtime.step(fallRight, false);
        // The shared Q8 acceleration accumulates a different subpixel
        // remainder from the removed float implementation (292.8). Assert
        // the last clear mask pixel, not that obsolete remainder. The current
        // axis adapter permits subpixel motion within this pixel.
        require(static_cast<int>(runtime.mainPlayerX()) == 292,
                "the shared player mask remains at the shaft wall's last clear pixel");
        downwell::Input fireMain;
        fireMain.fire = true;
        runtime.step(fireMain, true);
        require(runtime.mainAmmo() == 7 && runtime.mainShotsFired() == 1,
                "fire consumes one default machine-gun charge");
        require(runtime.mainPlayerSpeedY() == 0.0F &&
                    runtime.mainPlayerSpriteId() == 3,
                "default recoil arrests the fall and selects the shoot sprite");
        require(runtime.mainBulletCount() == 1,
                "the first gunboot shot creates a visible bullet");
        {
            // Isolate projectile/block coverage from later randomized layout.
            // Use a copy and a real room instance; original resources stay untouched.
            // O0 reserves all local snapshots in this function's stack frame.
            // Keep independent runtime copies on the heap, not the host stack.
            auto collisionStorage = std::make_unique<downwell::StartupRuntime>(runtime);
            auto& collision = *collisionStorage;
            std::size_t target = collision.activeInstanceCount();
            for (std::size_t i = 0; i < collision.activeInstanceCount(); ++i)
                if (collision.instanceAt(i).objectId == 60) { target = i; break; }
            require(target < collision.activeInstanceCount(), "projectile fixture has a room wall");
            auto& wall = const_cast<downwell::InstanceRecord&>(collision.instanceAt(target));
            wall.objectId = 92;
            wall.x = collision.mainBulletAt(0).trajectory.x.pixel;
            wall.y = collision.mainBulletAt(0).trajectory.y.pixel + 8;
            const auto before = collision.mainBlocksDestroyed();
            collision.step({}, false);
            require(!collision.instanceAt(target).active && collision.mainBlocksDestroyed() == before + 1 &&
                    collision.mainBulletCount() == 0,
                    "fractional projectile destroys the intersected block and retires on contact");
        }
        const float firstBulletY = runtime.mainBulletAt(0).y;
        for (int tick = 0; tick < 6; ++tick)
            runtime.step(fireMain, false);
        require(runtime.mainShotsFired() == 1,
                "held fire waits for the recovered seven-tick rate");
        require(runtime.mainBulletCount() == 1 &&
                    runtime.mainBulletAt(0).y > firstBulletY,
                "the machine-gun bullet travels downward each tick");
        runtime.step(fireMain, false);
        require(runtime.mainAmmo() == 6 && runtime.mainShotsFired() == 2,
                "held fire repeats when Alarm 2 clears the shot delay");

        downwell::Input releasedMain;
        const std::uint32_t rowsBefore = runtime.mainBuilderRowsGenerated();
        const int builderYBefore = runtime.mainBuilderY();
        const std::size_t instancesBefore = runtime.activeInstanceCount();
        for (int tick = 0;
             tick < 120 && runtime.mainBuilderRowsGenerated() == rowsBefore;
             ++tick)
            runtime.step(releasedMain, false);
        require(runtime.mainBuilderRowsGenerated() == rowsBefore + 1 &&
                    runtime.mainBuilderY() == builderYBefore + 16 &&
                    runtime.mainBuilderGenerationCount() ==
                        -30 + static_cast<int>(rowsBefore) + 1,
                "the first-boot builder advances one shared pattern row");
        require(runtime.activeInstanceCount() == instancesBefore + 2,
                "one builder row creates exactly two shaft-wall instances");
        bool foundLeftWall = false;
        bool foundRightWall = false;
        for (std::size_t index = 0; index < runtime.activeInstanceCount(); ++index) {
            const downwell::InstanceRecord& instance = runtime.instanceAt(index);
            if (instance.objectId != 60 || instance.y != builderYBefore)
                continue;
            foundLeftWall = foundLeftWall || instance.x == 160;
            foundRightWall = foundRightWall || instance.x == 320;
        }
        require(foundLeftWall && foundRightWall,
                "the generated row preserves the recovered shaft boundaries");
        for (int tick = 0;
             tick < 300 && runtime.mainBuilderRowsGenerated() < 29;
             ++tick)
            runtime.step(releasedMain, false);
        require(runtime.mainBuilderRowsGenerated() == 29 &&
                    runtime.mainBuilderY() == 1168 &&
                    runtime.mainBuilderGenerationCount() == -1,
                "the deterministic first-boot prelude emits all 29 shared rows");
        require(runtime.activeInstanceCount() == instancesBefore + 58,
                "the complete prelude retains its 58 generated boundary walls");
        const std::uint64_t drawsBeforeStart = runtime.randomDrawCount();
        for (int tick = 0;
             tick < 120 && runtime.mainBuilderStartPattern() < 0;
             ++tick)
            runtime.step(releasedMain, false);
        std::cerr << "Entrance after two spreading shots: pattern=" << runtime.mainBuilderStartPattern()
            << " reversed=" << runtime.mainBuilderStartReversed()
            << " rows=" << runtime.mainBuilderStartRowsGenerated() << ',' << runtime.mainBuilderRowsGenerated()
            << " instances=" << runtime.activeInstanceCount() - instancesBefore
            << " draws=" << runtime.randomDrawCount() - drawsBeforeStart << '\n';
        require(runtime.mainBuilderStartPattern() == 2 &&
                    !runtime.mainBuilderStartReversed() &&
                    runtime.mainBuilderGenerationCount() == 0,
                "corrected choice stream selects unmirrored entrance fragment two");
        require(runtime.mainBuilderStartRowsGenerated() == 6 &&
                    runtime.mainBuilderRowsGenerated() == 35,
                "the complete fragment advances its six shared row intervals");
        require(runtime.activeInstanceCount() == instancesBefore + 92,
                "the fragment creates its exact walls and breakable blocks");
        require(runtime.randomDrawCount() == drawsBeforeStart + 89,
                "box Create RNG ordering includes the reference gem-tile branch");

        downwell::Input steerToPlatform;
        steerToPlatform.left = true;
        for (int tick = 0; tick < 20; ++tick)
            runtime.step(steerToPlatform, false);
        for (int tick = 0; tick < 9; ++tick)
            runtime.step(steerToPlatform, false);
        const std::uint32_t shotsBeforeBlock = runtime.mainShotsFired();
        runtime.step(fireMain, true);
        // A grounded first press jumps; release and press again only if
        // needed. Releasing an airborne shot clears its cooldown in source.
        if (runtime.mainShotsFired() == shotsBeforeBlock) {
            runtime.step(releasedMain, false);
            runtime.step(fireMain, true);
        }
        require(runtime.mainShotsFired() == shotsBeforeBlock + 1,
                "block-shot fixture emits exactly one projectile after either jump or fall");
        downwell::Input steerToRemainingBlock;
        steerToRemainingBlock.right = true;
        for (int tick = 0; tick < 20; ++tick)
            runtime.step(steerToRemainingBlock, false);
        for (int tick = 0; tick < 120 && !runtime.mainPlayerGrounded(); ++tick)
            runtime.step(releasedMain, false);
        require(runtime.mainPlayerGrounded(),
                "the player lands on solid generated geometry");
        std::cerr << "Landing after spread: x=" << runtime.mainPlayerX() << " y=" << runtime.mainPlayerY()
            << " landings=" << runtime.mainLandings() << " patterns=" << runtime.mainBuilderMainPatternsGenerated()
            << " last=" << runtime.mainBuilderLastMainPattern()
            << " rows=" << runtime.mainBuilderRowsGenerated() << " blocks=" << runtime.mainBlocksDestroyed()
            << " sprite=" << runtime.mainPlayerSpriteId() << " vy=" << runtime.mainPlayerSpeedY() << '\n';
        require(runtime.mainPlayerY() >= 1199.0F &&
                    runtime.mainPlayerY() <= 1201.0F &&
                    runtime.mainPlayerSpeedY() == 0.0F &&
                    runtime.mainPlayerSpriteId() == 31,
                "landing settles on corrected entrance geometry after the shot");
        require(runtime.mainLandings() == 1 && runtime.mainAmmo() == 8,
                "the airborne-to-grounded edge fully recharges the gunboots");
        bool standingOnGeneratedTerrain = false;
        bool foundInitializedBlock = false;
        bool foundInitializedEnemy = false;
        bool foundMovingEnemy = false;
        for (std::size_t index = 0; index < runtime.activeInstanceCount(); ++index) {
            const downwell::InstanceRecord& instance = runtime.instanceAt(index);
            if (instance.active && instance.objectId == 92) {
                require(instance.imageSpeed == 0.0F &&
                            (instance.imageIndex == 16.0F ||
                             instance.imageIndex == 17.0F ||
                             instance.imageIndex == 22.0F),
                        "generated blocks retain recovered Create presentation state");
                foundInitializedBlock = true;
            }
            if (instance.active && instance.objectId == 93)
                require(instance.imageSpeed == 0.0F &&
                            instance.imageIndex == 23.0F,
                        "generated thin platforms retain their recovered tile frame");
            if (instance.active && instance.objectId == 289)
                require(instance.imageSpeed == 0.0F &&
                            (instance.scaleX == 1.0F || instance.scaleX == -1.0F) &&
                            instance.imageIndex >= 0.0F && instance.imageIndex <= 16.0F,
                        "generated scenery retains its recovered random Create state");
            int expectedHealth = 0;
            int expectedReward = 0;
            if (instance.objectId == 111) { expectedHealth = 5; expectedReward = 6; }
            if (instance.objectId == 112) { expectedHealth = 5; expectedReward = 14; }
            if (instance.objectId == 113) { expectedHealth = 35; expectedReward = 20; }
            if (instance.objectId == 116) { expectedHealth = 15; expectedReward = 12; }
            if (instance.objectId == 117) { expectedHealth = 20; expectedReward = 24; }
            if (instance.objectId == 119) { expectedHealth = 3; expectedReward = 6; }
            if (expectedHealth != 0) {
                require(instance.health == expectedHealth &&
                            instance.reward == expectedReward &&
                            instance.motionInitialized,
                        "generated enemies retain recovered combat values");
                foundInitializedEnemy = true;
                foundMovingEnemy = foundMovingEnemy ||
                    instance.x % 16 != 0 || instance.y % 16 != 0 ||
                    instance.velocityX != 0.0F || instance.velocityY != 0.0F;
            }
            if (instance.active && (instance.objectId == 60 || instance.objectId == 92 || instance.objectId == 93) &&
                instance.y == 1216 &&
                runtime.mainPlayerX() + 3.0F >= instance.x - 8.0F &&
                runtime.mainPlayerX() - 4.0F <= instance.x + 7.0F)
                standingOnGeneratedTerrain = true;
        }
        require(foundInitializedBlock && standingOnGeneratedTerrain,
                "grounded state remains backed by generated solid terrain, with initialized breakables in the room");
        require(foundInitializedEnemy,
                "the deterministic area-one slice contains a native enemy");
        if (!foundMovingEnemy) {
            // This entrance can contain only resting/offscreen actors.
            // Exercise host movement explicitly without changing its RNG tape.
            auto movementStorage = std::make_unique<downwell::StartupRuntime>(runtime);
            auto& movement = *movementStorage;
            std::size_t actorIndex = movement.activeInstanceCount();
            for (std::size_t i = 0; i < movement.activeInstanceCount(); ++i) {
                auto& actor = const_cast<downwell::InstanceRecord&>(movement.instanceAt(i));
                if (actor.objectId != 111 && actor.objectId != 112 && actor.objectId != 113 &&
                    actor.objectId != 116 && actor.objectId != 117 && actor.objectId != 119) continue;
                actorIndex = i;
                actor.objectId = 111; actor.spriteId = 163;
                actor.active = true; actor.activationPending = false;
                actor.x = static_cast<int>(movement.mainPlayerX());
                actor.y = static_cast<int>(movement.mainPlayerY()) - 64;
                actor.velocityX = 0.25F; actor.velocityY = 0;
                actor.grounded = false; actor.motionInitialized = false; actor.walker = {};
                break;
            }
            require(actorIndex < movement.activeInstanceCount(), "movement fixture has an enemy instance");
            const auto x = movement.instanceAt(actorIndex).x;
            movement.step({}, false);
            foundMovingEnemy = movement.instanceAt(actorIndex).preciseX != x;
        }
        require(foundMovingEnemy,
                "area-one enemies advance native fractional movement state");
        require(runtime.mainBuilderMainPatternsGenerated() == 1 &&
                    runtime.mainBuilderLastMainPattern() == 18 &&
                    runtime.mainBuilderGenerationCount() == 1,
                "the corrected stream generates regular fragment eighteen below the entrance");
        require(runtime.mainBuilderRowsGenerated() == 51,
                "regular fragments preserve row overlap and append pillar rows");
        require(runtime.mainShotsFired() == shotsBeforeBlock + 1 &&
                    runtime.mainBlocksDestroyed() == 1 &&
                    runtime.mainBulletCount() == 0,
                "the corrected layout's shot destroys one block and retires its projectile");
        {
            auto retirementStorage = std::make_unique<downwell::StartupRuntime>(runtime);
            auto& retirement = *retirementStorage;
            std::size_t boxIndex = retirement.activeInstanceCount();
            std::vector<std::uint32_t> retiredIds;
            for (std::size_t i = 0; i < retirement.activeInstanceCount(); ++i) {
                auto& object = const_cast<downwell::InstanceRecord&>(retirement.instanceAt(i));
                if (object.objectId != 92) continue;
                object.active = true;
                object.y = -512;
                boxIndex = i;
                retiredIds.push_back(object.instanceId);
            }
            require(boxIndex < retirement.activeInstanceCount(), "retirement fixture has a box");
            require(retiredIds.size() > 1, "retirement fixture exercises multiple compacting removals");
            const auto controller = *retirement.instance(downwell::StartupRuntime::controllerInstanceId);
            retirement.step(releasedMain, false);
            for (auto id : retiredIds)
                require(!retirement.hasInstance(id), "offscreen destruction removes identity rather than deactivating it");
            require(retirement.mainInstancesRetired() >= retiredIds.size() &&
                    retirement.instance(controller.instanceId)->objectId == controller.objectId,
                    "compacting retirement preserves surviving identities");
        }
        bool retainedOldStone = false;
        for (std::size_t index = 0; index < runtime.activeInstanceCount(); ++index) {
            const downwell::InstanceRecord& instance = runtime.instanceAt(index);
            retainedOldStone = retainedOldStone || (instance.active && instance.objectId == 60 &&
                instance.y < runtime.mainCameraViewY() - 160.0F);
            const bool generated = instance.objectId == 92 ||
                                   instance.objectId == 93 ||
                                   instance.objectId == 111 ||
                                   instance.objectId == 112 ||
                                   instance.objectId == 113 ||
                                   instance.objectId == 116 ||
                                   instance.objectId == 117 ||
                                   instance.objectId == 119 ||
                                   instance.objectId == 289;
            if (instance.active && generated)
                require(instance.y >= runtime.mainCameraViewY() - 160.0F &&
                            instance.y <= runtime.mainCameraViewY() + 3000.0F,
                        "active generated objects stay inside the lifecycle window");
        }
        require(retainedOldStone, "area-one stone walls do not use the area-five retirement rule");

        const float groundedY = runtime.mainPlayerY();
        {
            // A controlled ceiling fixture on a copy of the live room; never
            // mutate exported resources or the original test trajectory.
            auto headBoxStorage = std::make_unique<downwell::StartupRuntime>(runtime);
            auto& headBoxRuntime = *headBoxStorage;
            downwell::InstanceRecord* ceilingBox = nullptr;
            for (std::size_t i = 0; i < headBoxRuntime.activeInstanceCount(); ++i) {
                auto& candidate = const_cast<downwell::InstanceRecord&>(headBoxRuntime.instanceAt(i));
                if (candidate.objectId == 60) { ceilingBox = &candidate; break; }
            }
            require(ceilingBox != nullptr, "head-box fixture has a terrain instance");
            ceilingBox->objectId = 92;
            ceilingBox->active = true;
            ceilingBox->x = static_cast<int>(headBoxRuntime.mainPlayerX());
            ceilingBox->y = static_cast<int>(groundedY) - 24;
            const auto before = headBoxRuntime.mainBlocksDestroyed();
            headBoxRuntime.step(fireMain, true);
            headBoxRuntime.step(fireMain, false);
            headBoxRuntime.step(fireMain, false);
            require(!ceilingBox->active &&
                        headBoxRuntime.mainBlocksDestroyed() == before + 1 &&
                        headBoxRuntime.mainPlayerSpeedY() == 1.0F &&
                        headBoxRuntime.mainPlayerY() == groundedY - 11.0F &&
                        !headBoxRuntime.mainPlayerGrounded(),
                    "SDL head contact breaks one box and applies the source same-frame downward impulse");
        }
        const std::uint32_t shotsBeforeJump = runtime.mainShotsFired();
        runtime.step(fireMain, true);
        require(runtime.mainJumps() == 1 && !runtime.mainPlayerGrounded() &&
                    runtime.mainPlayerY() < groundedY &&
                    runtime.mainShotsFired() == shotsBeforeJump,
                "pressing fire on solid ground performs the recovered jump");
        runtime.step(releasedMain, false);
        runtime.step(fireMain, true);
        require(runtime.mainShotsFired() == shotsBeforeJump + 1 &&
                    runtime.mainAmmo() == 7 && runtime.mainBulletCount() == 1,
                "releasing after the jump unlocks an airborne gunboot shot");
        {
            // A controlled encounter, independent of the later side-room route.
            // Natural combat and collection remain mandatory in the paired scene tapes.
            auto combatStorage = std::make_unique<downwell::StartupRuntime>(runtime);
            auto& combat = *combatStorage;
            downwell::InstanceRecord* target = nullptr;
            std::size_t targetIndex = 0;
            for (std::size_t i = 0; i < combat.activeInstanceCount(); ++i) {
                auto& candidate = const_cast<downwell::InstanceRecord&>(combat.instanceAt(i));
                if (candidate.objectId == 111 || candidate.objectId == 112 || candidate.objectId == 113 ||
                    candidate.objectId == 116 || candidate.objectId == 117 || candidate.objectId == 119) {
                    target = &candidate; targetIndex = i; break;
                }
            }
            require(target != nullptr, "controlled encounter reuses a generated enemy instance");
            const auto bullet = combat.mainBulletAt(0);
            target->objectId = 111; target->spriteId = 163; target->reward = 6;
            target->scaleX = target->scaleY = 1;
            target->imageSpeed = 0.25F; target->imageIndex = 0;
            target->enemyImagePhase = 0; target->enemyImageRemainder = 0;
            target->active = true;
            target->activationPending = false;
            target->health = 5;
            target->x = static_cast<int>(bullet.x);
            target->y = static_cast<int>(bullet.y) + 8;
            target->preciseX = target->x;
            target->preciseY = target->y;
            target->velocityX = target->velocityY = 0;
            target->grounded = false;
            target->walker.initialize(target->x, target->y, 0, 0, false);
            // Reuse the same isolated bullet path for a surviving-hit fixture.
            auto batCombatStorage = std::make_unique<downwell::StartupRuntime>(combat);
            auto& batCombat = *batCombatStorage;
            auto& bat = const_cast<downwell::InstanceRecord&>(batCombat.instanceAt(targetIndex));
            bat.objectId = 116;
            bat.spriteId = 153;
            bat.health = 15;
            bat.reward = 12;
            bat.direction = 270;
            bat.moveSpeed = 1.25F;
            bat.imageSpeed = 0.2F;
            bat.alarms.reset();
            batCombat.step(releasedMain, false);
            require(batCombat.instanceAt(targetIndex).health == 6 &&
                        batCombat.instanceAt(targetIndex).batHit.pending,
                    "SDL usual bullet queues a surviving bat hit");
            const float batX = batCombat.instanceAt(targetIndex).preciseX;
            const float batY = batCombat.instanceAt(targetIndex).preciseY;
            for (int frame = 0; frame < 3; ++frame) {
                batCombat.step(releasedMain, false);
                const auto& state = batCombat.instanceAt(targetIndex);
                require(state.spriteId == 154 && state.alarms.get(0) == 3 - frame &&
                            state.preciseX == batX && state.preciseY == batY,
                        "SDL bat hit pauses movement with damage art for three ticks");
            }
            batCombat.step(releasedMain, false);
            require(!batCombat.instanceAt(targetIndex).batHit.stunned &&
                        batCombat.instanceAt(targetIndex).spriteId == 153,
                    "decoded bat Alarm 0 restores normal SDL presentation");
            const auto kills = combat.mainEnemiesDefeated();
            combat.step(releasedMain, false);
            require(combat.mainEnemiesDefeated() == kills + 1 && !combat.instanceAt(targetIndex).active &&
                        combat.mainCurrencyCount() > 0,
                    "a usual bullet defeats a five-health snake and spawns physical gems");
        }
        runtime.step(releasedMain, false);
        bool priorTraverseFire = false;
        for (int tick = 0;
             tick < 3000 && runtime.mainBuilderSideRoomsGenerated() == 0;
             ++tick) {
            downwell::Input traverse;
            traverse.left = (tick / 90) % 2 == 0;
            traverse.right = !traverse.left;
            traverse.fire = tick % 16 == 0 || tick % 16 == 3;
            runtime.step(traverse, traverse.fire && !priorTraverseFire);
            priorTraverseFire = traverse.fire;
        }
        std::cout << "Side-room tape: rooms=" << runtime.mainBuilderSideRoomsGenerated()
                  << " shops=" << runtime.mainBuilderShopsGenerated()
                  << " suppressed=" << runtime.mainBuilderShopSuppressed()
                  << " right=" << runtime.mainBuilderLastRightPattern()
                  << " generations=" << runtime.mainBuilderGenerationCount()
                  << " rows=" << runtime.mainBuilderRowsGenerated()
                  << " specials=" << runtime.mainBuilderSpecialObjectsGenerated() << '\n';
        // This is a tape-specific regression, not a Windows trajectory oracle.
        // Rounded player-mask X in upward terrain probes changes this route's
        // trajectory and later selection. The pre-correction movement reproduces
        // pattern 0/row 89; the corrected tape selects pattern 4/row 94.
        // Both recovered patterns contain v,O,_,@,{,} and no J torch marker.
        require(runtime.mainBuilderSideRoomsGenerated() == 1 &&
                    runtime.mainBuilderShopsGenerated() == 0 &&
                    runtime.mainBuilderShopSuppressed() &&
                    runtime.mainBuilderLastRightPattern() == 4 &&
                    runtime.mainBuilderGenerationCount() == 4 &&
                    runtime.mainBuilderRowsGenerated() == 94,
                "the first scheduled side room preserves deterministic selection and rows");
        require(runtime.mainBuilderSpecialObjectsGenerated() == 6,
                "every special marker in deterministic right-room pattern is interpreted");
        bool foundTorch = false;
        bool foundAmbience = false;
        bool foundTimeField = false;
        bool foundSideCamera = false;
        bool foundRoomReward = false;
        for (std::size_t index = 0; index < runtime.activeInstanceCount(); ++index) {
            const downwell::InstanceRecord& instance = runtime.instanceAt(index);
            foundTorch = foundTorch || instance.objectId == 296;
            foundAmbience = foundAmbience || instance.objectId == 376;
            foundTimeField = foundTimeField || instance.objectId == 82;
            foundSideCamera = foundSideCamera || instance.objectId == 62;
            foundRoomReward = foundRoomReward || instance.objectId == 293 ||
                              instance.objectId == 109;
        }
        require(!foundTorch && foundAmbience && foundTimeField &&
                    foundSideCamera && foundRoomReward,
                "pattern 4 emits its utility/reward IDs without inventing a J torch marker");
    }
}

} // namespace

int main(int argc, char** argv) {
    testDesktopBootState();
    testFailedRoomLoadStopsUpdates();
    testInitialDeferredFailureStopsRoomStart();
    testNonDesktopSkipsSplash();
    testRecoveredDesktopTiming();
    testAdvanceInputUsesPressEdges();
    if (argc >= 2)
        testGeneratedPackEntersRecoveredMenu(
            argv[1], argc >= 3 ? argv[2] : nullptr,
            argc >= 4 ? argv[3] : nullptr);
    std::cout << "startup runtime tests passed\n";
    return 0;
}
