#include "core/StartupRuntime.h"
#include "core/PlayerChildCreation.h"
#include <memory>
#include "core/BuilderPatternPack.h"
#include "core/ObjectPack.h"
#include "core/RoomPack.h"
#include "core/GameplayContactProfileInput.h"
#include "core/EnemyPresentation.h"
#include "core/GameMakerRounding.h"
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

template<class Runtime> void stepMany(Runtime& runtime, int count) {
    for (int i = 0; i < count; ++i)
        runtime.step(false);
}

void verifyLiveLayers(const downwell::StartupRuntime& runtime){
    const auto& room=runtime.sourceRoomLayers();
    require(room.succeeded(),"live room layer owner remains healthy");
    unsigned bound=0;
    require(room.layers().visitLayers([&](unsigned layer,const auto& metadata){
        return room.layers().visitElements(layer,[&](unsigned e){
            const auto* entry=room.instances().get(e);
            if(!entry||!entry->instance.generation)return true;
            const auto* state=room.lifetimes().resolve(entry->instance);
            require(state&&state->sourceId==entry->sourceId&&state->depth.attached&&state->depth.layerId==metadata.id,
                "live instance element and lifetime agree on source and layer identity");
            ++bound;return true;
        });
    }),"inspect live mixed room layers");
    require(bound==runtime.activeInstanceCount(),"every allocated runtime instance has one layer binding");
    for(std::size_t i=0;i<runtime.activeInstanceCount();++i){
        const auto* state=room.lifetimes().resolve(runtime.instanceHandleAt(i));
        require(state&&state->sourceId==runtime.instanceAt(i).instanceId&&state->inactive==!runtime.instanceAt(i).active,
            "live record activation and generation match layer state");
    }
}

void verifyPlayerChildren(const downwell::StartupRuntime& runtime,int room){
    bool found=false;
    for(std::size_t i=0;i<runtime.activeInstanceCount();++i){
        const auto& player=runtime.instanceAt(i);
        if(player.objectId!=35)continue;
        require(!found&&i+2<runtime.activeInstanceCount(),"player owns synchronous child prefix");
        found=true;
        const auto& meter=runtime.instanceAt(i+1);
        const auto& camera=runtime.instanceAt(i+2);
        require(meter.objectId==66&&camera.objectId==64,
                "depth meter and camera are born before the next authored room instance");
        require(meter.roomId==room&&camera.roomId==room,
                "synchronous children belong to destination room, not previous room");
        require(camera.instanceId==meter.instanceId+1,
                "child births share the original numeric allocator");
        require(player.playerCameraId==camera.instanceId&&runtime.instance(player.playerCameraId)==&camera,
                "myCam retains the completed camera's source numeric identity");
        require(camera.cameraFreeCam==((room>=20&&room<=24)||room==10||room==17)&&!camera.cameraAutoScroll,
                "camera Create defaults precede room-specific scrSpawnCamera mode assignments");
    }
    require(found,"room contains a player for synchronous birth test");
}

// Test branches reconstruct independent owners from identical resources and
// inputs. Copying a runtime would alias borrowed lifecycle/registration state.
class ReplayRuntime:public downwell::StartupRuntime {
    struct Frame {downwell::Input input;bool advance=false,legacy=false;};
    downwell::PlatformProfile profile_;
    const downwell::RoomPack* rooms_;
    const downwell::ObjectPack* objects_;
    std::uint32_t seed_;
    const downwell::BuilderPatternPack* patterns_;
    std::vector<Frame> tape_;
public:
    ReplayRuntime(downwell::PlatformProfile profile,const downwell::RoomPack* rooms,
        const downwell::ObjectPack* objects,std::uint32_t seed,const downwell::BuilderPatternPack* patterns)
        :StartupRuntime(profile,rooms,objects,seed,patterns),profile_(profile),rooms_(rooms),
         objects_(objects),seed_(seed),patterns_(patterns){}
    void step(bool advance){tape_.push_back({{},advance,true});StartupRuntime::step(advance);}
    void step(const downwell::Input& input,bool advance){
        tape_.push_back({input,advance,false});StartupRuntime::step(input,advance);
    }
    std::unique_ptr<ReplayRuntime> replay()const{
        auto result=std::make_unique<ReplayRuntime>(profile_,rooms_,objects_,seed_,patterns_);
        for(const auto& frame:tape_){
            if(frame.legacy)result->step(frame.advance);
            else result->step(frame.input,frame.advance);
        }
        require(result->stateDigest()==stateDigest()&&result->randomDrawCount()==randomDrawCount()&&
                result->nextRuntimeInstanceId()==nextRuntimeInstanceId(),
                "independent fixture replay reproduces state, RNG and identity before mutation");
        return result;
    }
};

struct LifecycleAudit {
    const downwell::StartupRuntime* observedRuntime=nullptr;
    unsigned initialViewsSeen=0;
    bool markDestroyed(downwell::InstanceHandle h,const downwell::InstanceRecord& r){
        if(r.instanceId==outOfViewId){
            require(matches(h,r)&&!r.destroyRequested,"out-of-view event marks the live identity before changing its flag");
            ++outOfViewMarks;
            if(rejectOutOfViewMark)return false;
        }
        if(r.objectId==256){
            require(matches(h,r)&&!r.destroyRequested,"balloon destruction publishes before changing the stored flag");
            ++balloonMarks;if(rejectBalloonMark)return false;
        }
        ++calls;
        require(matches(h,r)&&!r.destroyRequested,"destroy marks a still-live lifetime");
        if(r.objectId==61)++ditherMarks;
        if(r.objectId==92&&r.box.exhausted()){
            ++boxRemovals;
            if(rejectBoxRemoval){rejectedBoxId=r.instanceId;return false;}
        }
        return true;
    }
    struct Entry {std::uint32_t generation=0,id=0;bool live=false;};
    std::array<Entry,downwell::StartupRuntime::instanceCapacity> entries{};
    unsigned births=0,retires=0,activations=0,inactivations=0,memberships=0,moves=0,sprites=0,terrainSprites=0,spriteChanges=0,indices=0,transforms=0,calls=0,fail=0;
    unsigned walkingTransforms=0;
    unsigned frogTransforms=0;
    unsigned batTransforms=0;
    unsigned playerMoves=0;
    unsigned builderMoves=0,rejectBuilderMove=0;
    unsigned boxRemovals=0;
    bool rejectBoxRemoval=false;
    std::uint32_t rejectedBoxId=0;
    bool rejectBatScale=false;
    bool rejectBatMask=false;
    bool rejectCompanionImage=false;
    unsigned companionImages=0;
    unsigned balloonMarks=0,balloonRetires=0;
    std::uint32_t outOfViewId=0;
    unsigned outOfViewMarks=0,outOfViewRetires=0;
    bool rejectOutOfViewMark=false,rejectOutOfViewRetire=false;
    bool rejectBalloonMark=false,rejectBalloonRetire=false;
    unsigned ditherMoves=0,ditherMarks=0,rejectDitherMove=0;
    int rejectBirthObject=-1;
    int rejectBirthRoom=-1;
    unsigned rejectBirthOccurrence=1,matchingBirths=0;
    unsigned masks=0;
    bool mask(downwell::InstanceHandle h,const downwell::InstanceRecord& r,int next){
        if(r.objectId==289){
            require(matches(h,r)&&r.spriteId==550&&r.maskId==-1&&r.imageSpeed==0&&next==550,
                "scenery Create publishes its explicit sprite mask after prepared image state");
            return true;
        }
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
        const bool enemy=r.objectId==111||r.objectId==112||r.objectId==113||r.objectId==116||r.objectId==117||r.objectId==119;
        if((fail==9&&enemy)||(fail==11&&r.objectId==35))return false;
        ++transforms;return true;
    }
    bool imageIndex(downwell::InstanceHandle h,const downwell::InstanceRecord& r,downwell::RunnerImageValue value){
        // Animation uses reconciled room membership, not a pending active flag.
        // Lifetime identity and numeric validity are still mandatory.
        ++calls;require(matches(h,r)&&value.promoted().valid(),"animation publication resolves an allocated actor");
        if(r.objectId==249||r.objectId==256){
            ++companionImages;
            if(rejectCompanionImage)return false;
        }
        if(fail==8)return false;
        ++indices;return true;
    }
    bool sprite(downwell::InstanceHandle h,const downwell::InstanceRecord& r,int next){
        ++calls;require(matches(h,r)&&r.active,"sprite setter resolves an active published lifetime");
        if(fail==7)return false;
        ++sprites;if(r.objectId==60||r.objectId==93)++terrainSprites;
        if(next!=r.spriteId)++spriteChanges;return true;
    }
    bool move(downwell::InstanceHandle h,const downwell::InstanceRecord& r,
              downwell::RunnerImageValue x,downwell::RunnerImageValue y){
        ++calls;require(matches(h,r)&&r.active,"movement resolves an active published lifetime");
        std::int32_t px,py;
        require(x.promoted().roundPixel(px)&&y.promoted().roundPixel(py),"published enemy coordinates are representable");
        if(r.objectId==65){
            require(px==r.x&&py==r.y+16,
                "stored builder publishes each row through its live owner before updating the record");
            return ++builderMoves!=rejectBuilderMove;
        }
        if(r.objectId==61&&++ditherMoves==rejectDitherMove)return false;
        if(r.objectId==111||r.objectId==112||r.objectId==113||r.objectId==116||r.objectId==117||r.objectId==119){
            if(fail==6)return false;
            ++moves;
        }else{
            if(fail==10)return false;
            ++playerMoves;
        }
        return true;
    }
    bool matches(downwell::InstanceHandle h,const downwell::InstanceRecord& r)const{
        return h.slot<entries.size()&&entries[h.slot].live&&entries[h.slot].generation==h.generation&&entries[h.slot].id==r.instanceId;
    }
    bool birth(downwell::InstanceHandle h,const downwell::InstanceRecord& r){
        if(observedRuntime&&r.roomId==11&&r.objectId==35){
            const auto* view=observedRuntime->roomEntryView();
            require(view&&view->enabled&&view->x.bits==0&&view->y.bits==0&&
                    view->width.bits==0x43200000u&&view->height.bits==0x438e0000u,
                    "rmMain resource camera is ready before player Create and its children");
            ++initialViewsSeen;
        }
        ++calls;if(fail==1||(r.objectId==rejectBirthObject&&r.roomId==rejectBirthRoom&&
            ++matchingBirths==rejectBirthOccurrence))return false;
        require(h.slot<entries.size()&&!entries[h.slot].live&&h.generation>entries[h.slot].generation,
                "live runtime publishes births before reusing generation-checked slots");
        entries[h.slot]={h.generation,r.instanceId,true};++births;return true;
    }
    bool retire(downwell::InstanceHandle h,const downwell::InstanceRecord& r){
        ++calls;require(matches(h,r),"retirement still resolves the published lifetime");
        if(r.instanceId==outOfViewId){
            require(r.destroyRequested&&outOfViewMarks==1,"out-of-view cleanup follows one accepted event destruction request");
            ++outOfViewRetires;
            if(rejectOutOfViewRetire)return false;
        }
        if(r.objectId==256&&r.destroyRequested){
            ++balloonRetires;if(rejectBalloonRetire)return false;
        }
        if(fail==2)return false;
        entries[h.slot].live=false;++retires;return true;
    }
    bool activation(downwell::InstanceHandle h,const downwell::InstanceRecord& r,bool active){
        ++calls;require(matches(h,r)&&active!=r.active,"activation publication sees the old live flag");
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

struct BoxEndStepAudit:LifecycleAudit {
    std::uint32_t target=0;
    unsigned neighbours=0,queries=0,publications=0,reject=0;
    bool outside=false;
    bool placeMeeting(downwell::InstanceHandle h,const downwell::InstanceRecord& r,int object,
                      downwell::MotionBinary64 x,downwell::MotionBinary64 y,bool& hit){
        if(r.instanceId!=target){
            require(object==82||object==255,"unrelated fixture queries are player field/goal checks");
            hit=false;return true;
        }
        const int dx[]={0,16,0,-16},dy[]={-16,0,16,0};
        require(matches(h,r)&&object==92&&queries<4&&r.destroyRequested==outside,
            "box autotiling retains its caller after optional destruction");
        const auto ex=downwell::MotionBinary64::fromInteger(r.x+dx[queries]);
        const auto ey=downwell::MotionBinary64::fromInteger(r.y+dy[queries]);
        require(x.high==ex.high&&x.low==ex.low&&y.high==ey.high&&y.low==ey.low&&
            unsigned(r.box.scriptIndex)==(neighbours&((1u<<queries)-1)),
            "box neighbour queries and script-index writes follow up/right/down/left order");
        hit=(neighbours&(1u<<queries))!=0;
        return ++queries!=reject;
    }
    bool imageIndex(downwell::InstanceHandle h,const downwell::InstanceRecord& r,downwell::RunnerImageValue value){
        if(r.instanceId==target){
            downwell::RunnerImageValue expected;require(expected.assignInteger(neighbours),"fixture image index");
            require(queries==4&&!r.box.materialBreakable&&!r.box.checked&&value.bits==expected.bits,
                "material becomes wood before image publication, checked follows it");
            ++publications;if(reject==5)return false;
        }
        return LifecycleAudit::imageIndex(h,r,value);
    }
};

void testLiveLifecycle(const downwell::RoomPack& rooms,const downwell::ObjectPack& objects,
                       const downwell::BuilderPatternPack& patterns){
    using Runtime=downwell::StartupRuntime;
    const auto healthy=[](const Runtime& r){return r.effectsSucceeded()&&r.roomLoadSucceeded()&&r.deferredCommandsSucceeded();};
    {
        auto runtime=std::make_unique<Runtime>(downwell::PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns);
        runtime->skipToGameForTesting();
        unsigned spawner=0;
        for(unsigned tick=0;tick<600&&!spawner;++tick){
            for(unsigned i=0;i<runtime->activeInstanceCount();++i){
                const auto& r=runtime->instanceAt(i);
                if(r.objectId!=78)continue;
                require(!spawner,"one builder spawner birth");spawner=r.instanceId;
                const auto* s=runtime->spawnerState(spawner);
                require(s&&s->channelCount==1&&s->enemyMaxCount==20&&!s->scriptActive&&r.active&&
                    s->channels[0].enemies[0]==115&&s->channels[0].interval==160,
                    "natural builder invokes owned source cavern spawner Create");
            }
            if(!spawner)runtime->step(downwell::gameplayContactProfileInput(tick,runtime->mainPlayerX(),runtime->mainPlayerY()),false);
        }
        require(spawner&&healthy(*runtime),"natural spawner initialization preserves room health");
        runtime->reset(downwell::PlatformProfile::nonDesktop);
        require(!runtime->spawnerState(spawner),"reset invalidates retained spawner state");
    }
    for(unsigned mask=0;mask<16;++mask)for(bool outside:{false,true})
    for(unsigned reject=0;reject<=(mask==15?5u:0u);++reject){
        BoxEndStepAudit audit;audit.neighbours=mask;audit.outside=outside;audit.reject=reject;
        auto runtime=std::make_unique<Runtime>(downwell::PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
            downwell::InstanceLifecycleBinding::bindQueryDestruction(audit));
        runtime->skipToGameForTesting();
        bool foundBox=false;
        for(unsigned tick=0;tick<600&&!foundBox;++tick){
            runtime->step(downwell::gameplayContactProfileInput(tick,runtime->mainPlayerX(),runtime->mainPlayerY()),false);
            for(unsigned i=0;i<runtime->activeInstanceCount();++i){
                const auto& r=runtime->instanceAt(i);
                foundBox|=r.objectId==92&&r.active&&!r.destroyRequested;
            }
        }
        require(foundBox&&healthy(*runtime),"box End Step fixture reaches generated terrain");
        const bool ok=runtime->stepWithCollisionPass({},false,[&]{
            for(unsigned i=0;i<runtime->activeInstanceCount();++i){
                auto& r=const_cast<downwell::InstanceRecord&>(runtime->instanceAt(i));
                if(r.objectId!=92||!r.active||r.destroyRequested)continue;
                audit.target=r.instanceId;r.x=240;
                r.y=outside?-10000:static_cast<int>(runtime->mainCameraViewY())+128;
                r.box.autoTile=true;r.box.checked=false;r.box.materialBreakable=true;r.box.scriptIndex=99;
                return true;
            }
            require(false,"box End Step fixture has a generated box");return false;
        });
        require(audit.target&&ok==!reject&&healthy(*runtime)==!reject&&
                audit.queries==(reject&&reject<5?reject:4)&&audit.publications==(reject&&reject<5?0u:1u),
            "live box End Step executes all neighbours or stops at the rejected publication");
        const auto* r=runtime->instance(audit.target);
        if(outside&&!reject)require(!r,"out-of-view box completes autotiling before cleanup");
        else{
            require(r&&r->destroyRequested==outside&&r->box.checked==!reject,
                "box event retains destruction and checked state at the exact failure boundary");
            const unsigned expected=reject&&reject<5?mask&((1u<<(reject-1))-1):mask;
            require(unsigned(r->box.scriptIndex)==expected&&r->box.materialBreakable==(reject&&reject<5),
                "box autotile state preserves only the accepted query prefix");
        }
    }
    for(unsigned reject:{0u,1u,2u}){
        LifecycleAudit audit;
        auto runtime=std::make_unique<Runtime>(downwell::PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
            downwell::InstanceLifecycleBinding::bindMaskDestruction(audit));
        runtime->skipToGameForTesting();
        for(unsigned tick=0;tick<600&&!audit.outOfViewId;++tick){
            runtime->step(downwell::gameplayContactProfileInput(tick,runtime->mainPlayerX(),runtime->mainPlayerY()),false);
            for(unsigned i=0;i<runtime->activeInstanceCount();++i){
                const auto& r=runtime->instanceAt(i);
                if(r.objectId==93&&r.active&&!r.destroyRequested){audit.outOfViewId=r.instanceId;break;}
            }
        }
        require(audit.outOfViewId&&healthy(*runtime),"out-of-view fixture has a live thin platform");
        audit.rejectOutOfViewMark=reject==1;audit.rejectOutOfViewRetire=reject==2;
        unsigned collisions=0;
        const bool ok=runtime->stepWithCollisionPass({},false,[&]{
            ++collisions;
            auto* r=const_cast<downwell::InstanceRecord*>(runtime->instance(audit.outOfViewId));
            require(r&&!r->destroyRequested&&!audit.outOfViewMarks,
                "End Step destruction has not run during Collision");
            // Controlled collision-time movement: End Step must read this
            // position, not a pre-Collision list of retirement candidates.
            r->y=-10000;
            return true;
        });
        require(collisions==1&&ok==(reject==0)&&healthy(*runtime)==(reject==0)&&audit.outOfViewMarks==1,
            "selected End Step publishes destruction and propagates failures");
        const auto* retained=runtime->instance(audit.outOfViewId);
        require(audit.outOfViewRetires==(reject==1?0u:1u)&&
            (reject?(retained&&retained->destroyRequested==(reject==2)):!retained),
            "cleanup retains the exact accepted destruction prefix on failure");
        if(reject){
            runtime->step({},false);
            require(audit.outOfViewMarks==1&&audit.outOfViewRetires==(reject==1?0u:1u),
                "failed retirement is terminal rather than retrying callbacks");
        }
    }
    for(unsigned reject:{1u,30u}){
        LifecycleAudit audit;audit.rejectBuilderMove=reject;
        auto runtime=std::make_unique<Runtime>(downwell::PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
            downwell::InstanceLifecycleBinding::bindMaskDestruction(audit));
        runtime->skipToGameForTesting();
        for(unsigned tick=0;tick<1800&&healthy(*runtime);++tick)
            runtime->step(downwell::gameplayContactProfileInput(tick,runtime->mainPlayerX(),runtime->mainPlayerY()),false);
        const auto* builder=runtime->instance(100856);
        require(!healthy(*runtime)&&audit.builderMoves==reject&&builder&&
            builder->y==704+int(reject-1)*16,
            "rejected builder setter retains exactly its accepted row prefix");
        const auto calls=audit.calls;const auto births=audit.births;const auto draws=runtime->randomDrawCount();
        runtime->step(false);
        require(audit.calls==calls&&audit.births==births&&runtime->randomDrawCount()==draws,
            "failed builder publication is terminal, without later births or replaying RNG");
    }
    for(bool low:{false,true}){
        LifecycleAudit audit;
        auto runtime=std::make_unique<Runtime>(downwell::PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
            downwell::InstanceLifecycleBinding::bindMaskDestruction(audit));
        runtime->skipToGameForTesting();
        runtime->configureDitherAboveEnvironment(low,false);
        for(std::size_t i=0;i<runtime->activeInstanceCount();++i)if(runtime->instanceAt(i).objectId==61){
            require(runtime->publishCollisionPosition(runtime->instanceHandleAt(i),
                {0x44080400u},{0xc2a10000u}),"publish fractional authored dither position"); // 544.0625,-80.5
        }
        for(unsigned tick=0;tick<40;++tick){
            const auto* dither=runtime->instance(100854);
            const auto* player=runtime->instance(100855);
            const auto* camera=player?runtime->instance(player->playerCameraId):nullptr;
            require(dither&&camera&&healthy(*runtime),"source dither/camera lifetimes available");
            const float expectedX=camera->preciseX;
            float expectedY=dither->preciseY;
            const bool advance=double(camera->preciseY)-double(expectedY)>=double(low?208:272);
            if(advance)expectedY=float(double(expectedY)+16.0);
            expectedY=float(downwell::roundGameMakerCoordinate(double(expectedY)));
            const auto moves=audit.ditherMoves;
            runtime->step(false);
            dither=runtime->instance(100854);
            const auto* builder=runtime->instance(100856);
            require(builder&&audit.builderMoves==runtime->mainBuilderRowsGenerated()&&
                builder->y==704+int(audit.builderMoves)*16,
                "stored builder row state and owner publications agree after each Step");
            require(healthy(*runtime)&&dither&&dither->preciseX==expectedX&&dither->preciseY==expectedY&&
                audit.ditherMoves==moves+(advance?3:2),"Begin Step reads previous camera and publishes each source setter");
        }
        runtime->configureDitherAboveEnvironment(low,true);
        runtime->step(false);
        const auto* dither=runtime->instance(100854);
        require(healthy(*runtime)&&audit.ditherMarks==1&&(!dither||dither->destroyRequested),
            "ending marks authored dither through its owner");
    }
    for(unsigned reject:{1u,2u}){
        LifecycleAudit audit;
        auto runtime=std::make_unique<Runtime>(downwell::PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
            downwell::InstanceLifecycleBinding::bindMaskDestruction(audit));
        runtime->skipToGameForTesting();
        audit.rejectDitherMove=audit.ditherMoves+reject;
        runtime->step(false);
        require(!runtime->effectsSucceeded()&&audit.ditherMoves==audit.rejectDitherMove,
            "failed dither setter stops Begin Step");
        const auto calls=audit.calls;
        runtime->step(false);
        require(audit.calls==calls,"failed dither event prefix is not replayed");
    }
    {
        auto runtime=std::make_unique<Runtime>(downwell::PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns);
        auto control=std::make_unique<Runtime>(downwell::PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns);
        for(auto* world:{runtime.get(),control.get()}){
            require(world->grantUpgrade(15)&&world->grantUpgrade(9),"grant companion alarm fixture");
            const_cast<downwell::UpgradeInventory&>(world->upgrades()).levels[9]=2;
            world->skipToGameForTesting();
        }
        unsigned companions=0;
        const auto pending=downwell::MotionBinary64::fromInteger(7);
        for(unsigned i=0;i<runtime->activeInstanceCount();++i){
            auto& record=const_cast<downwell::InstanceRecord&>(runtime->instanceAt(i));
            if(record.objectId!=249&&record.objectId!=256)continue;
            ++companions;
            auto* state=const_cast<downwell::PlayerCompanionCreateState*>(runtime->companionCreateState(record.instanceId));
            require(state,"alarm fixture resolves child Create state by source ID");
            if(record.objectId==249){
                state->drone.shotDelay=state->drone.droneShooting=pending;
                record.alarms.set(3,1);record.alarms.set(2,2);
            }else{
                state->balloon.hitStun=state->balloon.noHold=pending;
                record.alarms.set(1,1);record.alarms.set(0,2);
            }
        }
        require(companions==3,"both drone lifetimes and balloon are exercised");
        for(unsigned tick=1;tick<=3;++tick){
            runtime->step(false);control->step(false);
            require(healthy(*runtime)&&runtime->randomDrawCount()==control->randomDrawCount()&&
                runtime->randomState().stateIndex()==control->randomState().stateIndex(),
                "companion alarm bodies consume no RNG");
            for(unsigned word=0;word<16;++word)
                require(runtime->randomState().stateWord(word)==control->randomState().stateWord(word),
                    "companion alarm dispatch preserves every RNG word");
            for(unsigned i=0;i<runtime->activeInstanceCount();++i){
                const auto& record=runtime->instanceAt(i);
                if(record.objectId!=249&&record.objectId!=256)continue;
                const auto* state=runtime->companionCreateState(record.instanceId);
                if(record.objectId==249){
                    require(state->drone.droneShooting.high==pending.high&&state->drone.droneShooting.low==pending.low&&
                        (tick==1?state->drone.shotDelay.high==pending.high:state->drone.shotDelay.zero())&&
                        record.alarms.get(3)==(tick==1?0:-1)&&record.alarms.get(2)==2-int(tick),
                        "empty drone Alarm 3 preserves shooting/delay; Alarm 2 clears only delay");
                }else {
                    require(state->balloon.hitStun.zero()&&
                    (tick==1?state->balloon.noHold.high==pending.high:state->balloon.noHold.zero())&&
                    record.alarms.get(1)==(tick==1?0:-1)&&record.alarms.get(0)==2-int(tick),
                    "balloon alarms release hit stun and no-hold on their own due frames");
                }
            }
        }
    }
    for(auto profile:{downwell::PlatformProfile::desktop,downwell::PlatformProfile::nonDesktop})
    for(unsigned failure=0;failure<3;++failure){
        LifecycleAudit audit;
        auto runtime=std::make_unique<Runtime>(profile,&rooms,&objects,2,&patterns,
            downwell::InstanceLifecycleBinding::bindMaskDestruction(audit));
        require(runtime->grantUpgrade(15),"grant balloon retirement fixture");
        runtime->skipToGameForTesting();
        std::uint32_t id=0;
        for(unsigned i=0;i<runtime->activeInstanceCount();++i)
            if(runtime->instanceAt(i).objectId==256)id=runtime->instanceAt(i).instanceId;
        require(id&&runtime->companionCreateState(id),"balloon retirement starts with a real child");
        runtime->step(false);
        require(healthy(*runtime)&&runtime->instance(id)&&audit.balloonMarks==0,
            "balloon End Step preserves the lifetime while its upgrade is enabled");
        const_cast<downwell::UpgradeInventory&>(runtime->upgrades()).levels[15]=0;
        audit.rejectBalloonMark=failure==1;audit.rejectBalloonRetire=failure==2;
        runtime->step(false);
        require(audit.balloonMarks==1&&audit.balloonRetires==(failure==1?0u:1u),
            "balloon removal publishes destruction once and retires only after acceptance");
        if(!failure){
            require(healthy(*runtime)&&!runtime->instance(id)&&!runtime->companionCreateState(id),
                "successful retirement removes both source identity and custom companion state");
            audit.verify(*runtime);
        }else{
            const auto* record=runtime->instance(id);
            require(!healthy(*runtime)&&record&&record->destroyRequested==(failure==2)&&runtime->companionCreateState(id),
                "failed destruction or retirement retains its exact accepted prefix");
            const auto calls=audit.calls;
            runtime->step(false);
            require(audit.calls==calls&&audit.balloonMarks==1,
                "failed balloon removal is terminal and not retried");
        }
    }
    for(unsigned balloon=0;balloon<2;++balloon)for(unsigned drones=0;drones<3;++drones){
        auto runtime=std::make_unique<Runtime>(downwell::PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns);
        if(balloon)require(runtime->grantUpgrade(15),"grant Heart Balloon before player Create");
        if(drones)require(runtime->grantUpgrade(9),"grant Drone before player Create");
        // Normal upgrade offers cap at one. Exercise the source's explicit
        // value-two branch as a controlled global-state input, not a second grant.
        if(drones==2)const_cast<downwell::UpgradeInventory&>(runtime->upgrades()).levels[9]=2;
        runtime->skipToGameForTesting();
        require(healthy(*runtime),"companion player Create enters live main room");
        unsigned player=0;
        while(player<runtime->activeInstanceCount()&&runtime->instanceAt(player).objectId!=35)++player;
        require(player<runtime->activeInstanceCount(),"find companion parent");
        unsigned child=player+1;std::uint32_t previous=0;
        const auto inspect=[&](int object){
            const auto& record=runtime->instanceAt(child++);
            require(record.objectId==object&&record.roomId==11,"companion children precede camera in source order");
            if(previous)require(record.instanceId==previous+1,"companion children share numeric allocator");
            previous=record.instanceId;return &record;
        };
        if(balloon){
            const auto* record=inspect(256);const auto* state=runtime->companionCreateState(record->instanceId);
            require(state&&record->imageSpeed==0.08F&&state->balloon.noTouch.high==0x3ff00000u&&
                state->balloon.stringLength.high==0x40340000u&&state->balloon.noHoldTime.high==0x40440000u,
                "balloon Create stores real child state, not a reserved ID");
        }
        for(unsigned i=0;i<drones;++i){
            const auto* record=inspect(249);const auto* state=runtime->companionCreateState(record->instanceId);
            const auto angle=downwell::MotionBinary64::fromInteger(i?180:0);
            require(state&&record->imageSpeed==0.5F&&state->drone.xdir.high==angle.high&&
                state->drone.xdir.low==angle.low&&state->drone.ydir.high==angle.high&&
                state->drone.shotDelay.zero()&&state->drone.burstCount.zero()&&state->drone.droneShooting.zero(),
                "second drone gets post-Create 180-degree fields");
        }
        inspect(66);const auto* camera=inspect(64);inspect(73);
        require(runtime->instanceAt(player).playerCameraId==camera->instanceId&&runtime->droneNumber()==(drones?1u:0u),
            "player camera and droneNum use source assignments after companion births");
        float droneImage=0,balloonImage=0;
        for(unsigned tick=0;tick<64;++tick){
            runtime->step(false);
            droneImage+=0.5F;if(droneImage>=2)droneImage-=2;
            balloonImage+=0.08F;if(balloonImage>=4)balloonImage-=4;
            require(healthy(*runtime),"companion animation survives repeated wraps in live scheduler");
            unsigned count=0;
            for(unsigned n=0;n<runtime->activeInstanceCount();++n){
                const auto& r=runtime->instanceAt(n);
                if(r.objectId!=249&&r.objectId!=256)continue;
                const auto expected=r.objectId==249?droneImage:balloonImage;
                require(r.imageIndex==expected,"live companion image follows single-rounded source timeline");
                const auto* state=runtime->companionCreateState(r.instanceId);
                std::uint32_t bits;std::memcpy(&bits,&expected,4);
                require(state&&state->timeline.bits==bits,"companion timeline and published image agree");
                ++count;
            }
            require(count==balloon+drones,"animation does not create or retire companion lifetimes");
        }
    }
    {
        LifecycleAudit audit;
        auto runtime=std::make_unique<Runtime>(downwell::PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
            downwell::InstanceLifecycleBinding::bindMaskDestruction(audit));
        require(runtime->grantUpgrade(9),"grant drone animation failure fixture");
        runtime->skipToGameForTesting();
        require(healthy(*runtime),"animation failure fixture enters gameplay");
        audit.rejectCompanionImage=true;
        const auto before=audit.companionImages;
        runtime->step(false);
        require(!healthy(*runtime)&&audit.companionImages==before+1,
            "rejected companion image publication stops animation phase");
        for(unsigned n=0;n<runtime->activeInstanceCount();++n){
            const auto& r=runtime->instanceAt(n);
            if(r.objectId!=249)continue;
            require(r.imageIndex==0&&runtime->companionCreateState(r.instanceId)->timeline.bits==0,
                "failed publication does not advance companion record or timeline");
        }
        runtime->step(false);
        require(audit.companionImages==before+1,"failed companion animation is not retried");
    }
    for(unsigned rejected=0;rejected<3;++rejected){
        LifecycleAudit failure;failure.rejectBirthObject=rejected?249:256;failure.rejectBirthRoom=11;
        failure.rejectBirthOccurrence=rejected==2?2:1;
        auto runtime=std::make_unique<Runtime>(downwell::PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
            downwell::InstanceLifecycleBinding::bindMaskDestruction(failure));
        require(runtime->grantUpgrade(15)&&runtime->grantUpgrade(9),"grant companion failure fixture");
        const_cast<downwell::UpgradeInventory&>(runtime->upgrades()).levels[9]=2;
        runtime->skipToGameForTesting();
        require(!healthy(*runtime)&&runtime->hasInstance(100855)&&!runtime->hasInstance(100854),
            "companion birth failure retains parent but stops later room creation");
        unsigned companions=0;
        for(std::size_t i=0;i<runtime->activeInstanceCount();++i){
            const auto& record=runtime->instanceAt(i);
            if(record.roomId!=11)continue;
            require(record.objectId!=66&&record.objectId!=64,"failed companion prevents camera/depth meter births");
            if(record.objectId==249||record.objectId==256){
                ++companions;
                require((runtime->companionCreateState(record.instanceId)!=nullptr)==(companions<=rejected),
                    "rejected birth is retained without running its Create; earlier Creates survive");
            }
        }
        require(companions==rejected+1,"failed allocation prefix includes exactly the attempted children");
        require(runtime->droneNumber()==(rejected==1?0u:1u),
            "droneNum retains the source prefix: old value before reset, zero before first birth, one before second");
        const auto calls=failure.calls;runtime->step(false);
        require(calls==failure.calls,"failed companion Create is not retried");
    }
    for(int rejectedObject:{66,64}){
        LifecycleAudit failure;failure.rejectBirthObject=rejectedObject;failure.rejectBirthRoom=11;
        auto runtime=std::make_unique<Runtime>(downwell::PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
            downwell::InstanceLifecycleBinding::bindMaskDestruction(failure));
        runtime->skipToGameForTesting();
        require(!healthy(*runtime),"synchronous player-child birth failure stops room entry");
        require(runtime->hasInstance(100855)&&!runtime->hasInstance(100854),
                "failed child retains parent but prevents the next authored controller birth");
        require(runtime->instance(100855)->playerCameraId==0,
                "failed child creation does not publish a myCam result");
        bool meter=false,camera=false;
        for(std::size_t i=0;i<runtime->activeInstanceCount();++i){
            meter|=runtime->instanceAt(i).objectId==66;
            camera|=runtime->instanceAt(i).objectId==64;
        }
        require(meter&&camera==(rejectedObject==64),
                "failure retains its allocated child and does not execute later child births");
        const auto calls=failure.calls;const auto tick=runtime->tick();
        runtime->step(false);
        require(failure.calls==calls&&runtime->tick()==tick,"failed child creation is not retried");
    }
    struct BulletAudit:LifecycleAudit {
        struct Retired {downwell::InstanceHandle key;float x,y;bool destroyed;};
        std::vector<Retired> bullets;
        bool retire(downwell::InstanceHandle key,const downwell::InstanceRecord& r){
            if(r.objectId==261)bullets.push_back({key,r.preciseX,r.preciseY,r.destroyRequested});
            return LifecycleAudit::retire(key,r);
        }
    } audit;
    auto indexed=std::make_unique<Runtime>(downwell::PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
        downwell::InstanceLifecycleBinding::bindMaskDestruction(audit));
    auto plain=std::make_unique<Runtime>(downwell::PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns);
    audit.observedRuntime=indexed.get();
    indexed->skipToGameForTesting();plain->skipToGameForTesting();audit.verify(*indexed);
    require(audit.initialViewsSeen==1,"live room entry observed source initial camera view");
    unsigned collisionShots=0,postCollisionRetirements=0;
    for(unsigned tick=0;tick<1800;++tick){
        const auto input=downwell::gameplayContactProfileInput(tick,indexed->mainPlayerX(),indexed->mainPlayerY());
        std::vector<downwell::StartupRuntime::MainBulletState> shots;
        std::vector<std::pair<float,float>> publishedPositions;
        audit.bullets.clear();
        require(indexed->stepWithCollisionPass(input,false,[&]{
            for(std::size_t i=0;i<indexed->mainBulletCount();++i){
                const auto& shot=indexed->mainBulletAt(i);
                require(shot.trajectory.initialized,"live collision pass sees completed bullet Step initialization");
                shots.push_back(shot);++collisionShots;
                const auto* record=indexed->resolveInstance(shot.instance);
                require(record,"Collision bullet has a retained instance");
                publishedPositions.push_back({record->preciseX,record->preciseY});
            }
            return true;
        }),"live retained Step completes before collision observation");
        plain->step(input,false);
        std::size_t retained=0;
        for(std::size_t i=0;i<shots.size();++i){
            const auto& before=shots[i];
            if(!indexed->resolveInstance(before.instance)){
                unsigned matches=0;
                for(const auto& retired:audit.bullets)if(retired.key.slot==before.instance.slot&&retired.key.generation==before.instance.generation){
                    require(retired.destroyed&&retired.x==publishedPositions[i].first&&retired.y==publishedPositions[i].second,
                        "End Step retires a marked bullet without moving it again");++matches;
                }
                require(matches==1,"removed Collision bullet has exactly one lifecycle retirement");
                ++postCollisionRetirements;continue;
            }
            require(retained<indexed->mainBulletCount(),"surviving bullet remains in the compact view");
            const auto& after=indexed->mainBulletAt(retained++);
            require(before.instance.slot==after.instance.slot&&before.instance.generation==after.instance.generation&&
                before.trajectory.x.pixel==after.trajectory.x.pixel&&before.trajectory.x.fraction==after.trajectory.x.fraction&&
                before.trajectory.y.pixel==after.trajectory.y.pixel&&before.trajectory.y.fraction==after.trajectory.y.fraction,
                "bullet position observed at Collision is retained through End Step");
        }
        require(retained==indexed->mainBulletCount(),"post-Collision phase creates no extra bullet or duplicate entry");
        require(healthy(*indexed)&&indexed->stateDigest()==plain->stateDigest(),
                "lifecycle publication preserves complete live gameplay state and RNG");
        const auto* builder=indexed->instance(100856);
        require(builder&&audit.builderMoves==indexed->mainBuilderRowsGenerated()&&
            builder->y==704+int(audit.builderMoves)*16,
            "natural generation keeps stored and published builder rows synchronized");
        audit.verify(*indexed);
    }
    require(collisionShots>0,"natural contact tape exercises bullet Step at the collision boundary");
    require(postCollisionRetirements>0,"natural contact tape exercises bullet End Step retirement");
    std::cout << "Live lifecycle births=" << audit.births << " retires=" << audit.retires
              << " activations=" << audit.activations << " inactivations=" << audit.inactivations
              << " memberships=" << audit.memberships << " moves=" << audit.moves
              << " sprites=" << audit.sprites << " spriteChanges=" << audit.spriteChanges << " indices=" << audit.indices
              << " transforms=" << audit.transforms << '\n';
    // Source sprite assignments and engine position publication are separate
    // events (including projectile setters); they are not paired one-for-one.
    // Verify each coverage category, while the callbacks above check lifetime
    // validity and the paired run checks every frame's complete state/RNG.
    require(audit.births>100&&audit.retires>2&&audit.memberships>0&&audit.inactivations>0&&audit.moves>0&&
            audit.terrainSprites>0&&audit.sprites>audit.terrainSprites&&audit.spriteChanges>0&&audit.indices>audit.sprites,
            "natural gameplay covers dynamic births, contact inactivation, retirement and reconciliation");
    require(audit.playerMoves>=1800,"live player publishes each Begin Step position to its owner");
    require(audit.builderMoves>30,"natural contact tape exercises multi-row builder publication");
    indexed->reset(downwell::PlatformProfile::nonDesktop);audit.verify(*indexed);
    require(healthy(*indexed),"reset retires old lifetimes before creating a fresh room");
    for(bool reject:{false,true}){
        LifecycleAudit boxes;boxes.rejectBoxRemoval=reject;
        auto runtime=std::make_unique<Runtime>(downwell::PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
            downwell::InstanceLifecycleBinding::bindMaskDestruction(boxes));
        runtime->skipToGameForTesting();
        for(unsigned tick=0;tick<1800&&healthy(*runtime);++tick)
            runtime->step(downwell::gameplayContactProfileInput(tick,runtime->mainPlayerX(),runtime->mainPlayerY()),false);
        require(boxes.boxRemovals>0,"natural contact tape reaches checked box removal");
        require(healthy(*runtime)!=reject,"box publication rejection is terminal");
        std::cout<<"Natural box requests="<<boxes.boxRemovals<<" rejected="<<reject
                 <<" destroyed="<<runtime->mainBlocksDestroyed()<<'\n';
        if(!reject)require(runtime->mainBlocksDestroyed()==boxes.boxRemovals,
                "successful box destruction requests match the gameplay count");
        if(reject){
            require(runtime->mainBlocksDestroyed()==0,"rejected first box removal does not count a destroyed block");
            const auto* box=runtime->instance(boxes.rejectedBoxId);
            require(box&&box->active&&!box->destroyRequested&&box->box.exhausted(),
                    "rejected destroy retains the exhausted live box without publishing the mark");
            const auto calls=boxes.calls;const auto tick=runtime->tick();
            runtime->step(false);
            require(boxes.calls==calls&&runtime->tick()==tick,"box failure remains latched");
        }
    }
    for(unsigned reject:{0u,1u,2u}){
        LifecycleAudit checked;
        auto runtime=std::make_unique<Runtime>(downwell::PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
            downwell::InstanceLifecycleBinding::bindMaskDestruction(checked));
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
        unsigned collisionCalls=0;float collisionX=0,collisionY=0;
        const bool tickSucceeded=runtime->stepWithCollisionPass({},false,[&]{
            ++collisionCalls;const auto* moved=runtime->instance(id);
            require(moved&&checked.batTransforms==before+1&&
                (moved->preciseX!=startX||moved->preciseY!=startY),
                "selected enemy Step publishes movement before the collision phase");
            collisionX=moved->preciseX;collisionY=moved->preciseY;return true;
        });
        require(tickSucceeded==!reject&&collisionCalls==(reject?0u:1u),
                "enemy Step failure stops the scheduler before Collision and End Step");
        const auto* bat=runtime->instance(id);
        if(!reject)require(bat&&bat->preciseX==collisionX&&bat->preciseY==collisionY,
                "enemy motion is not dispatched again by a post-phase family loop");
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
            downwell::InstanceLifecycleBinding::bindMaskDestruction(walking));
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
            downwell::InstanceLifecycleBinding::bindMaskDestruction(changed));
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
    for(unsigned failure=0;failure<=11;++failure){
        LifecycleAudit failing;
        auto runtime=std::make_unique<Runtime>(downwell::PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
            downwell::InstanceLifecycleBinding::bindMaskDestruction(failing));
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
        downwell::InstanceLifecycleBinding::bindMaskDestruction(failedReset));
    failedReset.fail=2;const auto births=failedReset.births;
    runtime->reset(downwell::PlatformProfile::desktop);
    require(!healthy(*runtime)&&failedReset.births==births,"failed reset retirement cannot publish replacement births");
    LifecycleAudit failedBirth;failedBirth.fail=1;
    auto rejected=std::make_unique<Runtime>(downwell::PlatformProfile::desktop,&rooms,&objects,2,&patterns,
        downwell::InstanceLifecycleBinding::bindMaskDestruction(failedBirth));
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

    if(objects){
        // Exercise the actual resource Create/ordinary Step dispatch with a
        // controlled stopped actor. These operations are a test fixture, not
        // a replacement for live trigonometry/effect bindings.
        using N=downwell::MotionBinary64;using F=downwell::RunnerImageValue;
        struct Ops {
            unsigned& speeds;unsigned& sprites;
            bool position(bool,F v,F& out){out=v;return true;}
            bool playerY(N& out){out={};return true;}
            bool playerX(N& out){out={};return true;}
            bool pointDirection(N,N,N,N,N& out){out=N::fromInteger(90);return true;}
            bool direction(N v,N& out){out=v;return true;}
            bool imageSpeed(F v,F& out){++speeds;out=v;return true;}
            bool sprite(int id){require(id==176,"stopped jelly publishes original sprite");++sprites;return true;}
            bool enemyPlayerY(N&){return false;}bool enemyPlayerX(N&){return false;}
            bool imageIndex(int){return false;}bool sound(int,int,int,int,const char*){return false;}
            bool alarm(int,int){return false;}bool lengthdir(bool,N,N,N&){return false;}
            bool meeting(N,N,int,bool&){return false;}bool enemyDeath(){return false;}
            bool smoke(N,N,int,int){return false;}bool movingFx(N,N,int,N,N,N){return false;}
            bool flash(N,N,int,int,int){return false;}bool currency(int){return false;}bool destroy(){return false;}
            bool viewY(N& out){out={};return true;}bool room(int&){return false;}
            bool emitTimer(N&){return false;}bool effectAlarmReset(N){return false;}
        };
        auto runtime=std::make_unique<downwell::StartupRuntime>(downwell::PlatformProfile::desktop,&pack,objects);
        auto owner=runtime->whiteJellyStates<2>();unsigned speeds=0,sprites=0,stopReads=0;
        downwell::InstanceHandle key;unsigned id=0;
        require(runtime->withWhiteJellyEvents(owner,[&](auto){return Ops{speeds,sprites};},
            [&](bool& stop){++stopReads;stop=true;return true;},[&]{
                if(!runtime->spawnWhiteJelly(24,56,key))return false;
                for(std::size_t i=0;i<runtime->activeInstanceCount();++i)
                    if(runtime->instanceAt(i).objectId==115)id=runtime->instanceAt(i).instanceId;
                require(owner.state(id)&&owner.state(id)->initialized&&speeds==1,"resource Create reaches jelly owner");
                runtime->step(false);return runtime->effectsSucceeded()&&runtime->deferredCommandsSucceeded();
            }),"scoped jelly resource event dispatch succeeds");
        require(stopReads==1&&speeds==2&&sprites==1,"ordinary room phase executes stopped jelly Step exactly once");
        require(owner.state(id)->imageSpeed.bits==0,"stopped jelly image speed retained");
        const auto count=runtime->activeInstanceCount();
        require(!runtime->spawnWhiteJelly(24,56,key)&&runtime->activeInstanceCount()==count,
                "jelly resource birth cannot reuse expired stack-bound operations");
    }

    if(objects&&patterns){
        struct CorpseAudit:LifecycleAudit {
            unsigned fieldQueries=0,motionQueries=0;bool fieldHit=false;
            bool birthAt(downwell::InstanceHandle h,const downwell::InstanceRecord& r,
                downwell::RunnerImageValue,downwell::RunnerImageValue){return birth(h,r);}
            bool placeMeeting(downwell::InstanceHandle h,const downwell::InstanceRecord& r,int object,
                downwell::MotionBinary64,downwell::MotionBinary64,bool& hit){
                require(matches(h,r),"controlled collision fixture resolves the querying lifetime");
                if(r.objectId==263){
                    require(object==57||object==56||object==82,"corpse uses source Create, movement and time-field families");
                    if(object==82){++fieldQueries;hit=fieldHit;return true;}
                    if(object==56)++motionQueries;
                }
                hit=false;return true;
            }
            bool instancePlace(downwell::InstanceHandle,const downwell::InstanceRecord&,int,
                downwell::MotionBinary64,downwell::MotionBinary64,downwell::InstancePlaceResult&){return false;}
            bool instanceNumber(int,unsigned&){return false;}
        } audit;
        auto runtime=std::make_unique<downwell::StartupRuntime>(downwell::PlatformProfile::nonDesktop,
            &pack,objects,2,patterns,downwell::InstanceLifecycleBinding::bindPositionedBirths(audit));
        runtime->skipToGameForTesting();
        require(runtime->effectsSucceeded(),"enter room before registered corpse death fixture");
        require(runtime->grantUpgrade(3),"grant Rest in Pieces for corpse Create");
        auto corpses=runtime->corpseRegistration<2>();
        const auto id=runtime->nextRuntimeInstanceId(),births=audit.births;
        const auto draws=runtime->randomDrawCount();
        downwell::DeadBodySource source;source.x=downwell::MotionBinary64::fromInteger(240);
        source.y=downwell::MotionBinary64::fromInteger(1200);source.imageXscale=downwell::MotionBinary64::fromInteger(-1);
        require(runtime->createRegisteredCorpseFromDeath(corpses,source,206,downwell::MotionBinary64::fromInteger(2),102,
            [&](auto h,int layer,auto,auto,bool& hit){
                require(runtime->instance(id)&&h.generation&&layer==100,"tile query sees synchronous corpse birth");
                hit=false;return true;
            }),"create registered corpse through the real runtime allocator and RNG");
        require(source.myDeadBody==id&&runtime->nextRuntimeInstanceId()==id+1&&audit.births==births+1&&
            runtime->randomDrawCount()==draws+3,"corpse uses enclosing ID stream and three Create draws");
        const auto* state=corpses.state(corpses.sourceHandle(id));
        require(state&&state->health.high==0x3ff00000u&&state->health.low==0&&state->hitEffect==1&&runtime->instance(id)->spriteId==206,
            "runtime upgrade and wrapper overrides reach registered corpse");
        unsigned steps=0,alarmCalls=0;
        const auto corpseKey=corpses.sourceHandle(id);
        auto execute=[&](auto,auto phase,unsigned index,auto& corpse,auto& publish,bool& good){
            struct Events {
                decltype(publish)& output;bool& good;int currentArea;
                bool meeting(downwell::MotionBinary64,downwell::MotionBinary64,bool& hit){hit=false;return true;}
                bool publish(downwell::RunnerImageValue x,downwell::RunnerImageValue y){return output.position(x,y);}
                int area(){return currentArea;}
                int restInPieces(){return 1;}
                bool create(int,downwell::MotionBinary64,downwell::MotionBinary64){return false;}
                bool create(int,downwell::MotionBinary64,downwell::MotionBinary64,std::uint32_t&){return false;}
                bool sprite(unsigned,int){return false;}
                void destroy(){if(!output.destroy())good=false;}
            } events{publish,good,runtime->runStage().area};
            if(phase==downwell::StartupRuntime::CorpsePhase::alarm){
                ++alarmCalls;require(index==3,"only activation alarm fires in short corpse tape");
                return corpse.alarmEvent(index,events);
            }
            ++steps;
            if(steps==10)corpse.yy=downwell::MotionBinary64::fromInteger(-10000);
            return corpse.stepInWorld(steps==2||steps==3,480,publish,events,good);
        };
        downwell::RunnerImageValue previousX,previousY;
        for(unsigned tick=1;tick<=10;++tick){
            audit.fieldHit=tick==3;
            require(runtime->withCorpseEvents(corpses,execute,[&]{runtime->step(false);return runtime->effectsSucceeded();}),
                "corpse joins actual animation/alarm/Step/End Step/cleanup scheduling");
            if(tick<10){
                state=corpses.state(corpseKey);const auto* record=runtime->instance(id);
                require(state&&record&&state->active==(tick>=8)&&record->alarms.get(3)==(tick<8?8-int(tick):tick==8?0:-1),
                    "room alarm countdown is not applied twice by corpse owner");
                downwell::RunnerImageValue publicX,publicY,publicIndex;
                std::memcpy(&publicX.bits,&record->preciseX,4);std::memcpy(&publicY.bits,&record->preciseY,4);
                std::memcpy(&publicIndex.bits,&record->imageIndex,4);
                require(publicX.bits==state->x.bits&&publicY.bits==state->y.bits&&publicIndex.bits==state->imageIndex.bits,
                    "corpse motion and animation publish the registered builtin state");
                if(tick==2)require(publicX.bits==previousX.bits&&publicY.bits==previousY.bits&&record->alarms.get(0)==179,
                    "frozen corpse retains position and refunds only source cooldown alarms");
                previousX=publicX;previousY=publicY;
            }
        }
        require(steps==10&&alarmCalls==1&&audit.fieldQueries==2&&audit.motionQueries==18&&
            !runtime->instance(id)&&!corpses.state(corpseKey),
            "out-of-view End Step retires corpse at room cleanup");
        require(corpses.clear()&&!runtime->instance(id),"retire corpse before room teardown");
        auto expiry=runtime->corpseExpiryRegistration<1>();
        auto skeletons=runtime->skeletonRegistration<1>();downwell::InstanceHandle dyingSkeleton;
        require(runtime->spawnSkeleton(skeletons,240,1200,dyingSkeleton),"registered dying caller");
        auto smokeBurst=runtime->smokeRegistration<6>();
        auto smokeRandom=runtime->randomState();
        const auto firstSmokeId=runtime->nextRuntimeInstanceId();
        downwell::SmokeEffect expectedSmoke[6];unsigned expectedCount=0;
        const auto smokeX=downwell::MotionBinary64::fromInteger(240);
        const auto smokeY=downwell::MotionBinary64::fromInteger(1200);
        require(downwell::skeletonSmokeBurst(smokeX,smokeY,smokeRandom,[&](int object,auto x,auto y){
            require(object==225&&expectedCount<6,"scalar skeleton burst child");
            return expectedSmoke[expectedCount++].create(x,y,smokeRandom);
        }),"scalar complete synchronous smoke burst");
        require(skeletons.dispatchPublished(dyingSkeleton,[&](auto& actor,auto&,auto& publish,bool&){
            return publish.smoke([&](auto& caller){
                require(runtime->emitRegisteredSkeletonSmoke(smokeBurst,caller,actor.xx,actor.yy),
                    "skeleton caller emits room-owned smoke burst");
                return caller.smokenum==int(expectedCount)&&caller.vary==1&&
                    actor.smokeScript.smokenum==int(expectedCount);
            });
        }),"script caller fields published on actual skeleton");
        require(runtime->nextRuntimeInstanceId()==firstSmokeId+expectedCount&&
            runtime->randomDrawCount()==smokeRandom.drawCount()&&runtime->smokeEffects().size()==0,
            "source IDs and synchronous RNG belong to room, not compatibility smoke pool");
        for(unsigned i=0;i<16;++i)require(runtime->randomState().stateWord(i)==smokeRandom.stateWord(i),
            "complete registered burst RNG agrees with scalar source path");
        for(unsigned i=0;i<expectedCount;++i){
            const auto* r=runtime->instance(firstSmokeId+i);const auto* s=r?smokeBurst.state(*r):nullptr;
            const auto& expected=expectedSmoke[i];
            require(s&&s->x.bits==expected.x.bits&&s->y.bits==expected.y.bits&&
                s->speed.high==expected.speed.high&&s->speed.low==expected.speed.low&&
                s->direction.high==expected.direction.high&&s->direction.low==expected.direction.low&&
                s->imageSpeed.bits==expected.imageSpeed.bits&&!s->allSet,
                "registered children complete Create without consuming first-Step random draw");
        }
        require(smokeBurst.clear(),"explicit burst fixture teardown");
        const auto parentId=runtime->nextRuntimeInstanceId();
        require(skeletons.dispatchPublished(dyingSkeleton,[&](auto& actor,auto&,auto& publish,bool&){
            actor.xsp={0x3fb99999u,0x9999999au}; // 0.1 triggers source fallback.
            actor.xx=downwell::MotionBinary64::fromInteger(-1000); // Not the published spawn position.
            require(publish.facing(-1),"dying skeleton facing publication");
            return publish.deadBody([&](auto& caller){
                require(runtime->createRegisteredCorpseFromDeath(corpses,caller,206,
                    downwell::MotionBinary64::fromInteger(9),102,
                    [](auto,int,auto,auto,bool& hit){hit=false;return true;}),"create corpse from live skeleton fields");
                require(actor.myDeadBody==parentId&&actor.bodySprite==206&&
                    actor.impactSp.high==0x40100000u&&actor.impactSp.low==0&&
                    actor.xsp.high==0xbff80000u&&actor.xsp.low==0,
                    "script mutations reach the live caller before returning");
                return true;
            });
        }),"registered skeleton invokes source dead-body wrapper");
        const auto* born=corpses.state(corpses.sourceHandle(parentId));
        require(born&&born->x.bits==0x43700000u&&born->y.bits==0x44960000u&&
            born->imageXscale.bits==0xbf800000u&&born->xsp.high==0xc0180000u&&born->xsp.low==0,
            "corpse uses published anchor, facing and capped impact instead of private accumulators");
        require(skeletons.mark(dyingSkeleton)&&skeletons.retire(dyingSkeleton),"retire fixture caller after its death callback");
        const auto parentKey=corpses.sourceHandle(parentId);
        require(corpses.dispatchPublished(parentKey,[](auto& corpse,auto&,bool&){
            corpse.alarms.set(0,1);return true;
        }),"set expiry alarm before room countdown");
        const auto childId=runtime->nextRuntimeInstanceId();unsigned expired=0;
        auto expiryAlarm=[&](auto,auto phase,unsigned index,auto& corpse,auto& publish,bool&){
            require(phase==downwell::StartupRuntime::CorpsePhase::alarm&&index==0,
                "expiry alarm destroys parent before ordinary Step");
            const auto rng=runtime->randomDrawCount();
            require(expiry.alarmEvent(corpse,index,publish,[](int sprite){
                require(sprite==102,"source-selected expiry sprite");return 12u;
            }),"registered child Create, override and parent mark complete synchronously");
            require(corpse.myExplosion==childId&&runtime->instance(parentId)->destroyRequested&&
                runtime->instance(childId)&&runtime->randomDrawCount()==rng,
                "expiry uses real numeric identity and consumes no RNG");
            ++expired;return true;
        };
        auto tickExpiry=[&]{return runtime->withCorpseExpiryEvents(expiry,[&]{
            return runtime->withCorpseEvents(corpses,expiryAlarm,[&]{runtime->step(false);return runtime->effectsSucceeded();});
        });};
        require(tickExpiry()&&expired==1&&!runtime->instance(parentId)&&runtime->instance(childId),
            "room cleanup retires only parent on child birth frame");
        require(expiry.state(childId)&&expiry.state(childId)->timeline.bits==0&&
            runtime->instance(childId)->imageSpeed==0.3f&&runtime->nextRuntimeInstanceId()==childId+1,
            "child waits for next animation walk and publishes Create speed");
        unsigned childDraws=0;downwell::PackedObject expiryDefinition;
        require(objects->findObject(242,expiryDefinition)&&runtime->visitSourceDrawChecked(
            [&](auto,const auto& record,int depth,auto action){
                if(record.instanceId==childId){++childDraws;
                    require(action==downwell::RunnerLayerDraw::Action::sprite&&
                        depth==expiryDefinition.creationDepth&&record.spriteId==102,
                        "registered expiry joins ordinary source layer drawing at serialized depth");
                }
                return true;
            },[](const auto&,int){return true;})&&childDraws==1,
            "expiry is included once in mixed room Draw traversal");
        downwell::CorpseExpiryEffect oracle;
        require(oracle.create(source.x,source.y),"create scalar expiry oracle");
        unsigned frames=0;
        while(!oracle.destroyRequested&&frames<50){
            require(oracle.advanceImage()&&tickExpiry(),"room expiry animation matches source unit track");++frames;
            const auto* child=expiry.state(childId);
            if(!oracle.destroyRequested)require(child&&child->timeline.bits==oracle.timeline.bits&&
                child->imageIndex.bits==oracle.imageIndex.bits,"registered expiry preserves animation state");
            else require(!child&&!runtime->instance(childId),"Animation End mark reaches deferred room cleanup");
        }
        require(oracle.destroyRequested&&expired==1&&expiry.clear()&&corpses.clear(),
            "expiry chain completes once without leaked room records");
        audit.verify(*runtime);
        auto smoke=runtime->smokeRegistration<1>();std::uint32_t smokeId=0;
        require(runtime->createRegisteredSmoke(smoke,downwell::MotionBinary64::fromInteger(240),
            downwell::MotionBinary64::fromInteger(1200),smokeId),"registered smoke synchronous Create");
        require(runtime->instance(smokeId)&&smoke.state(*runtime->instance(smokeId))&&
            !smoke.state(*runtime->instance(smokeId))->allSet,"smoke waits for first room Step");
        unsigned smokeFrames=0;
        while(runtime->instance(smokeId)&&smokeFrames<200){
            require(runtime->withSmokeEvents(smoke,[&]{runtime->step(false);return runtime->effectsSucceeded();}),
                "registered smoke joins actual room phases");
            ++smokeFrames;
            if(const auto* r=runtime->instance(smokeId)){
                const auto* s=smoke.state(*r);downwell::RunnerImageValue x,y,index;
                std::memcpy(&x.bits,&r->preciseX,4);std::memcpy(&y.bits,&r->preciseY,4);
                std::memcpy(&index.bits,&r->imageIndex,4);
                require(s&&s->allSet&&s->x.bits==x.bits&&s->y.bits==y.bits&&s->imageIndex.bits==index.bits,
                    "smoke publishes position and animation to real room record");
            }
        }
        require(smokeFrames>1&&smokeFrames<200&&!runtime->instance(smokeId)&&smoke.clear(),
            "smoke Animation End retires at room cleanup");
        downwell::InstanceHandle failedCaller;
        require(runtime->spawnSkeleton(skeletons,240,1200,failedCaller),"failure-retention caller");
        const auto failedBody=runtime->nextRuntimeInstanceId();unsigned callbacks=0;
        require(!skeletons.dispatchPublished(failedCaller,[&](auto&,auto&,auto& publish,bool&){
            return publish.deadBody([&](auto& caller){
                ++callbacks;
                require(runtime->createRegisteredCorpseFromDeath(corpses,caller,206,
                    downwell::MotionBinary64::fromInteger(9),102,
                    [](auto,int,auto,auto,bool& hit){hit=false;return true;}),"retain completed child before caller failure");
                return false;
            });
        }),"caller failure is terminal after synchronous child creation");
        const auto* failed=skeletons.instances().find(failedCaller);
        require(failed&&failed->state.myDeadBody==failedBody&&failed->state.bodySprite==206&&
            failed->state.impactSp.high==0x40100000u&&failed->state.xsp.high==0x3ff80000u&&
            corpses.state(corpses.sourceHandle(failedBody))&&!runtime->effectsSucceeded(),
            "failed event retains actual caller mutations and child lifetime");
        require(!skeletons.dispatchPublished(failedCaller,[&](auto&,auto&,auto&,bool&){++callbacks;return true;})&&callbacks==1,
            "failed caller cannot replay Create or RNG");
        require(corpses.clear()&&skeletons.clear(),"explicit teardown of retained failure prefix");
    }

    if (objects != nullptr) {
        auto cached = std::make_unique<downwell::StartupRuntime>(
            downwell::PlatformProfile::nonDesktop, &pack, objects,
            downwell::StartupRuntime::defaultRandomSeed, patterns);
        std::array<std::uint64_t,180> checkpoints{};
        for (unsigned i=0;i<checkpoints.size();++i) {
            cached->step(i%32==0);
            verifyLiveLayers(*cached);
            checkpoints[i]=cached->stateDigest();
        }
        cached->reset(downwell::PlatformProfile::nonDesktop);
        for (unsigned i=0;i<checkpoints.size();++i) {
            cached->step(i%32==0);
            verifyLiveLayers(*cached);
            require(cached->stateDigest()==checkpoints[i],
                    "alarm binding cache reset preserves complete deterministic startup state");
        }
    }

    auto runtimeStorage=std::make_unique<ReplayRuntime>(
        downwell::PlatformProfile::nonDesktop, &pack, objects,
        downwell::StartupRuntime::defaultRandomSeed, patterns);
    auto& runtime=*runtimeStorage;
    require(runtime.roomLoadSucceeded(), "packed rmInit loads");
    verifyLiveLayers(runtime);
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
    verifyLiveLayers(runtime);
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
        auto edgeEntryStorage=std::make_unique<downwell::StartupRuntime>(
            downwell::PlatformProfile::nonDesktop, &pack, objects,
            downwell::StartupRuntime::defaultRandomSeed, patterns);
        auto& edgeEntry=*edgeEntryStorage;
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

        require(runtime.instance(105187) != nullptr &&
                    runtime.instance(105187)->objectId == 66,
                "player Create spawns the depth meter first");
        require(runtime.instance(105188) != nullptr &&
                    runtime.instance(105188)->objectId == 64,
                "player Create spawns camMain second");
        verifyPlayerChildren(runtime,20);
        const downwell::InstanceRecord* title = runtime.instance(105193);
        require(title != nullptr && title->objectId == 317 &&
                    title->x == 328 && title->y == 432 &&
                    title->imageSpeed == 0.0F,
                "well Create spawns the stationary first-boot title");
        const downwell::InstanceRecord* fade = runtime.instance(105194);
        require(fade != nullptr && fade->objectId == 382 && fade->roomId == 20 &&
                    fade->x == 0 && fade->y == 0,
                "rmMenu creates ditherFade with the authoritative runtime ID");
        require(fade->imageSpeed == 0.0F && fade->alarms.get(0) == 30,
                "ditherFade Create freezes animation and arms Alarm 0");
        require(runtime.nextRuntimeInstanceId() == 105195,
                "runtime allocator advances over all recovered Create spawns");
        require(runtime.deferredCommandsSucceeded(),
                "deferred room-creation commands apply successfully");

        stepMany(runtime, 30);
        fade = runtime.instance(105194);
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
        title = runtime.instance(105193);
        require(title != nullptr && title->alarms.get(2) >= 0,
                "crossing beneath the title arms its recovered reveal delay");
        for (int tick = 0; tick < 50; ++tick)
            runtime.step(wake, false);
        title = runtime.instance(105193);
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
        verifyLiveLayers(runtime);
        require(runtime.hasInstance(100855),
                "rmMain creates its authoritative player instance");
        verifyPlayerChildren(runtime,11);
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
        require(runtime.mainBulletAt(0).animation.index.bits==0,
                "a Begin Step birth misses the earlier animation traversal and retains Create frame zero");
        {
            auto marked=runtime.replay();const auto key=marked->mainBulletAt(0).instance;
            require(marked->markBulletDestroyed(key)&&marked->markBulletDestroyed(key),
                    "projectile destruction can be requested twice without retiring the executing record");
            const auto* record=marked->resolveInstance(key);
            require(record&&record->destroyRequested&&record->objectId==261,
                    "marked projectile remains resolvable for the Step tail");
        }
        {
            // Isolate projectile/block coverage from later randomized layout.
            // Start an independent resource-backed owner with a real box lifetime;
            // original resources and the main input trajectory stay untouched.
            auto collisionStorage=std::make_unique<downwell::StartupRuntime>(
                downwell::PlatformProfile::nonDesktop,&pack,objects,2,patterns);
            auto& collision = *collisionStorage;
            collision.skipToGameForTesting();
            std::size_t target=collision.activeInstanceCount();
            for(unsigned tick=0;tick<1800;++tick){
                collision.step(downwell::gameplayContactProfileInput(tick,collision.mainPlayerX(),collision.mainPlayerY()),false);
                require(collision.effectsSucceeded(),"projectile fixture reaches a live box without runtime failure");
                target=collision.activeInstanceCount();
                if(collision.mainBulletCount()==1)for(std::size_t i=0;i<collision.activeInstanceCount();++i){
                    const auto& r=collision.instanceAt(i);
                    if(r.objectId==92&&r.active&&!r.destroyRequested&&!r.box.exhausted()){target=i;break;}
                }
                if(target<collision.activeInstanceCount())break;
            }
            require(target < collision.activeInstanceCount(), "projectile fixture has a generated box");
            // Isolated collision placement only, not natural encounter coverage:
            // move the real box and shot outside unrelated generated terrain.
            auto& projectile=const_cast<downwell::StartupRuntime::MainBulletState&>(collision.mainBulletAt(0));
            projectile.x=1000;projectile.y=collision.mainCameraRenderViewY()+50;
            projectile.trajectory.reset(1000,int(projectile.y),false);
            auto& wall = const_cast<downwell::InstanceRecord&>(collision.instanceAt(target));
            const auto targetId=wall.instanceId;
            wall.x = collision.mainBulletAt(0).trajectory.x.pixel;
            wall.y = collision.mainBulletAt(0).trajectory.y.pixel + 8;
            const auto before = collision.mainBlocksDestroyed();
            collision.step({}, false);
            const auto* damaged=collision.instance(targetId);
            require(collision.effectsSucceeded()&&damaged&&damaged->active&&!damaged->destroyRequested&&
                    damaged->box.exhausted()&&collision.mainBlocksDestroyed()==before&&collision.mainBulletCount()==0,
                    "fractional projectile damages the box and retires without running box Step early");
            collision.step({},false);
            require(collision.effectsSucceeded()&&!collision.instance(targetId)&&collision.mainBlocksDestroyed()==before+1,
                    "the next box Step requests destruction and pre-Draw cleanup retires its lifetime");
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
        const auto firstNewInstance = runtime.nextRuntimeInstanceId();
        auto newTerrainCount=[&](){
            unsigned count=0;
            for(std::size_t i=0;i<runtime.activeInstanceCount();++i){
                const auto& record=runtime.instanceAt(i);
                if(record.instanceId>=firstNewInstance&&
                    (record.objectId==60||record.objectId==92||record.objectId==93))++count;
            }
            return count;
        };
        for (int tick = 0;
             tick < 120 && runtime.mainBuilderRowsGenerated() == rowsBefore;
             ++tick)
            runtime.step(releasedMain, false);
        require(runtime.mainBuilderRowsGenerated() == rowsBefore + 1 &&
                    runtime.mainBuilderY() == builderYBefore + 16 &&
                    runtime.mainBuilderGenerationCount() ==
                        -30 + static_cast<int>(rowsBefore) + 1,
                "the first-boot builder advances one shared pattern row");
        // Registered projectiles/effects may retire during these ticks. Count
        // new wall identities, not the net size of all instance families.
        unsigned newWalls=0;
        bool foundLeftWall = false;
        bool foundRightWall = false;
        for (std::size_t index = 0; index < runtime.activeInstanceCount(); ++index) {
            const downwell::InstanceRecord& instance = runtime.instanceAt(index);
            if(instance.objectId==60&&instance.instanceId>=firstNewInstance)++newWalls;
            if (instance.objectId != 60 || instance.y != builderYBefore)
                continue;
            foundLeftWall = foundLeftWall || instance.x == 160;
            foundRightWall = foundRightWall || instance.x == 320;
        }
        require(newWalls==2&&foundLeftWall && foundRightWall,
                "the generated row preserves the recovered shaft boundaries");
        for (int tick = 0;
             tick < 300 && runtime.mainBuilderRowsGenerated() < 29;
             ++tick)
            runtime.step(releasedMain, false);
        require(runtime.mainBuilderRowsGenerated() == 29 &&
                    runtime.mainBuilderY() == 1168 &&
                    runtime.mainBuilderGenerationCount() == -1,
                "the deterministic first-boot prelude emits all 29 shared rows");
        require(newTerrainCount() == 58,
                "the complete prelude retains its 58 generated boundary walls");
        const std::uint64_t drawsBeforeStart = runtime.randomDrawCount();
        for (int tick = 0;
             tick < 120 && runtime.mainBuilderStartPattern() < 0;
             ++tick)
            runtime.step(releasedMain, false);
        std::cerr << "Entrance after two spreading shots: pattern=" << runtime.mainBuilderStartPattern()
            << " reversed=" << runtime.mainBuilderStartReversed()
            << " rows=" << runtime.mainBuilderStartRowsGenerated() << ',' << runtime.mainBuilderRowsGenerated()
            << " terrain=" << newTerrainCount()
            << " draws=" << runtime.randomDrawCount() - drawsBeforeStart << '\n';
        // startTimeField's Create now consumes its source draw before being
        // destroyed on level 1. This shifts the subsequent entrance choice.
        require(runtime.mainBuilderStartPattern() == 1 &&
                    !runtime.mainBuilderStartReversed() &&
                    runtime.mainBuilderGenerationCount() == 0,
                "start-field Create stream selects unmirrored entrance fragment one");
        require(runtime.mainBuilderStartRowsGenerated() == 5 &&
                    runtime.mainBuilderRowsGenerated() == 34,
                "the complete fragment advances its five shared row intervals");
        require(newTerrainCount() == 73,
                "the fragment creates its exact walls and breakable blocks");
        require(runtime.randomDrawCount() == drawsBeforeStart + 62,
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
        require(runtime.mainPlayerY() >= 1327.0F &&
                    runtime.mainPlayerY() <= 1329.0F &&
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
                instance.y == 1344 &&
                runtime.mainPlayerX() + 3.0F >= instance.x - 8.0F &&
                runtime.mainPlayerX() - 4.0F <= instance.x + 7.0F)
                standingOnGeneratedTerrain = true;
        }
        require(foundInitializedBlock && standingOnGeneratedTerrain,
                "grounded state remains backed by generated solid terrain, with initialized breakables in the room");
        require(runtime.mainBuilderMainPatternsGenerated()==2&&runtime.mainBuilderLastMainPattern()==2&&
                runtime.mainBuilderGenerationCount()==2&&runtime.mainBuilderRowsGenerated()==58,
                "start-field RNG stream generates two regular fragments with the decoded row overlap");
        require(runtime.mainShotsFired()==shotsBeforeBlock+1&&runtime.mainBlocksDestroyed()==0&&runtime.mainBulletCount()==0,
                "this corrected layout's shot retires without hitting a breakable block");
        // The corrected entry RNG can select a terrain-only landing slice.
        // Continue with real inputs until the next generated actor, keeping
        // the actor assertions instead of requiring this particular platform
        // to contain one or injecting an enemy into the generation stream.
        auto encounterStorage=runtime.replay();
        auto& encounters=*encounterStorage;
        for(unsigned tick=0;!foundInitializedEnemy&&tick<480;++tick){
            downwell::Input walk;walk.left=true;
            encounters.step(walk,false);
            for(unsigned i=0;i<encounters.activeInstanceCount();++i){
                const auto& actor=encounters.instanceAt(i);
                const int ids[]={111,112,113,116,117,119};
                const int health[]={5,5,35,15,20,3},reward[]={6,14,20,12,24,6};
                for(unsigned kind=0;kind<6;++kind)if(actor.objectId==ids[kind]){
                    require(actor.health==health[kind]&&actor.reward==reward[kind]&&actor.motionInitialized,
                        "first naturally generated actor retains recovered Create combat state");
                    foundInitializedEnemy=true;
                    foundMovingEnemy=foundMovingEnemy||actor.x%16!=0||actor.y%16!=0||actor.velocityX!=0.0F||actor.velocityY!=0.0F;
                }
            }
        }
        require(foundInitializedEnemy,
                "the deterministic area-one slice contains a native enemy");
        if (!foundMovingEnemy) {
            // This entrance can contain only resting/offscreen actors.
            // Exercise host movement explicitly without changing its RNG tape.
            auto movementStorage = encounters.replay();
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
        {
            auto retirementStorage = runtime.replay();
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
        {
            auto snapStorage=runtime.replay();
            auto& snap=*snapStorage;
            const int oldView=int(snap.mainCameraViewY());
            std::uint32_t boxId=0;
            for(std::size_t i=0;i<snap.activeInstanceCount();++i){
                auto& box=const_cast<downwell::InstanceRecord&>(snap.instanceAt(i));
                if(box.objectId!=92)continue;
                box.active=true;box.y=oldView-120;boxId=box.instanceId;break;
            }
            require(boxId&&snap.registerSideCameraLocker(int(snap.mainCameraViewX())+80,oldView+222),
                "host camera retirement fixture has a box and overlapping locker");
            auto* shop=snap.registerShop(480,oldView-280);
            require(shop,"host published-view fixture allocates a shop");
            auto& dither=const_cast<downwell::ExitDither&>(snap.exitDither());
            dither.active=true;dither.y=oldView+284;
            snap.step(releasedMain,false);
            require(snap.effectsSucceeded()&&snap.mainCameraViewY()==oldView+80,
                "host camera collision publishes the new view in this frame");
            require(snap.hasInstance(boxId),
                "host box End Step lifetime retains the pre-collision published view");
            require(shop->present&&dither.y==oldView+284,
                "host shop and dither read the cached view throughout Step and End Step");
            snap.step(releasedMain,false);
            require(snap.effectsSucceeded()&&!snap.hasInstance(boxId),
                "host box retires on the next update after Draw publishes the snap");
            require(!shop->present&&dither.y==oldView+292,
                "host shop retires and dither advances after the new view is published");
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
            // A controlled ceiling fixture in a replayed live room; never
            // mutate exported resources or the original test trajectory.
            auto headBoxStorage = runtime.replay();
            auto& headBoxRuntime = *headBoxStorage;
            downwell::InstanceRecord* ceilingBox = nullptr;
            for (std::size_t i = 0; i < headBoxRuntime.activeInstanceCount(); ++i) {
                auto& candidate = const_cast<downwell::InstanceRecord&>(headBoxRuntime.instanceAt(i));
                if (candidate.objectId == 92 && candidate.active && !candidate.destroyRequested &&
                    !candidate.box.exhausted()) { ceilingBox = &candidate; break; }
            }
            require(ceilingBox != nullptr, "head-box fixture has a live generated box");
            const auto ceilingId=ceilingBox->instanceId;
            ceilingBox->x = static_cast<int>(headBoxRuntime.mainPlayerX());
            ceilingBox->y = static_cast<int>(groundedY) - 24;
            const auto before = headBoxRuntime.mainBlocksDestroyed();
            headBoxRuntime.step(fireMain, true);
            headBoxRuntime.step(fireMain, false);
            headBoxRuntime.step(fireMain, false);
            require(headBoxRuntime.effectsSucceeded()&&!headBoxRuntime.instance(ceilingId) &&
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
            auto combatStorage = encounters.replay();
            auto& combat = *combatStorage;
            for(unsigned frame=0;combat.mainBulletCount()==0&&frame<12;++frame){
                downwell::Input shot;shot.fire=frame%3!=1;
                combat.step(shot,false);
            }
            require(combat.mainBulletCount()>0,"controlled encounter emits a real projectile before positioning its target");
            downwell::InstanceRecord* target = nullptr;
            std::size_t targetIndex = 0;
            for (std::size_t i = 0; i < combat.activeInstanceCount(); ++i) {
                auto& candidate = const_cast<downwell::InstanceRecord&>(combat.instanceAt(i));
                if (candidate.active && (candidate.objectId == 111 || candidate.objectId == 112 || candidate.objectId == 113 ||
                    candidate.objectId == 116 || candidate.objectId == 117 || candidate.objectId == 119)) {
                    target = &candidate; targetIndex = i; break;
                }
            }
            require(target != nullptr, "controlled encounter reuses a generated enemy instance");
            const auto targetId = target->instanceId;
            const auto bullet = combat.mainBulletAt(0);
            // Branch before fixture edits, then copy only the explicit actor
            // record setup below (not runtime owners or borrowed bindings).
            auto batCombatStorage = combat.replay();
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
            auto& batCombat = *batCombatStorage;
            auto& bat = const_cast<downwell::InstanceRecord&>(batCombat.instanceAt(targetIndex));
            require(bat.instanceId==targetId,"replayed actor retains fixture identity");
            bat=*target;
            const auto batId = bat.instanceId;
            bat.objectId = 116;
            bat.spriteId = 153;
            bat.health = 15;
            bat.reward = 12;
            bat.direction = 270;
            bat.moveSpeed = 1.25F;
            bat.imageSpeed = 0.2F;
            bat.alarms.reset();
            batCombat.step(releasedMain, false);
            require(batCombat.instance(batId)->health == 6 &&
                        batCombat.instance(batId)->batHit.pending,
                    "SDL usual bullet queues a surviving bat hit");
            const float batX = batCombat.instance(batId)->preciseX;
            const float batY = batCombat.instance(batId)->preciseY;
            for (int frame = 0; frame < 3; ++frame) {
                batCombat.step(releasedMain, false);
                require(batCombat.instance(batId)!=nullptr,"bat lifetime survives stun");
                const auto& state = *batCombat.instance(batId);
                require(state.spriteId == 154 && state.alarms.get(0) == 3 - frame &&
                            state.preciseX == batX && state.preciseY == batY,
                        "SDL bat hit pauses movement with damage art for three ticks");
            }
            batCombat.step(releasedMain, false);
            require(!batCombat.instance(batId)->batHit.stunned &&
                        batCombat.instance(batId)->spriteId == 153,
                    "decoded bat Alarm 0 restores normal SDL presentation");
            const auto kills = combat.mainEnemiesDefeated();
            combat.step(releasedMain, false);
            const auto* defeated = combat.instance(targetId);
            require(combat.mainEnemiesDefeated() == kills + 1 && (!defeated || !defeated->active) &&
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
        // The source startTimeField Create consumes a radius draw even when it
        // destroys itself on level 1. With that entry draw this tape selects
        // pattern 7/row 86. VM string 1022 contains v,O,_,@,{,} and no J marker.
        require(runtime.mainBuilderSideRoomsGenerated() == 1 &&
                    runtime.mainBuilderShopsGenerated() == 0 &&
                    runtime.mainBuilderShopSuppressed() &&
                    runtime.mainBuilderLastRightPattern() == 7 &&
                    runtime.mainBuilderGenerationCount() == 4 &&
                    runtime.mainBuilderRowsGenerated() == 86,
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
                "pattern 7 emits its utility/reward IDs without inventing a J torch marker");
    }
}

} // namespace

int main(int argc, char** argv) {
    for(int room:{11,14,17,20,24})for(unsigned balloon:{0u,1u})for(unsigned drones:{0u,1u,2u}){
        std::vector<int> expected;
        if(balloon)expected.push_back(256);
        expected.push_back(1000);
        if(drones){expected.push_back(249);expected.push_back(1001);}
        if(drones==2){expected.push_back(249);expected.push_back(1002);}
        expected.push_back(66);expected.push_back(64);expected.push_back(1003);
        if(room==11)expected.push_back(73);
        for(unsigned failure=0;failure<=expected.size();++failure){
            std::vector<int> actual;std::uint32_t captured=0;
            auto accept=[&](int op){actual.push_back(op);return actual.size()!=failure;};
            const bool ok=downwell::PlayerChildCreation::run(room,240,176,balloon,drones,
                [&](int object,int x,int y)->std::uint32_t{
                    require(x==(object==66?-100:240)&&y==176,"source child coordinates");
                    if(!accept(object))return 0;
                    // Deliberately nonconsecutive: nested child Creates may consume IDs.
                    return captured=100000+unsigned(actual.size())*3;
                },[&](unsigned count){return accept(1000+int(count));},
                [&](std::uint32_t id){require(id==captured,"second drone uses captured child ID");return accept(1002);},
                [&](std::uint32_t id,bool freeCam,bool clearScroll){
                    require(id==captured&&freeCam==((room>=20&&room<=24)||room==17)&&clearScroll==(room==11),
                        "camera reference and post-Create room modes");
                    return accept(1003);
                },[&](int destination){require(destination==room,"entry wipe destination");return accept(73);});
            const unsigned count=failure?failure:unsigned(expected.size());
            require(ok==!failure&&actual.size()==count,"child sequence stops at rejected operation");
            for(unsigned i=0;i<count;++i)require(actual[i]==expected[i],"child source-order prefix");
        }
    }
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
