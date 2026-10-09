#include "core/BinaryEffectSurface.h"
#include "core/InstanceStore.h"
#include "core/RunnerLayerElements.h"
#include "core/RunnerLayers.h"
#include "core/RunnerDepthChanges.h"
#include "core/RunnerLayerDraw.h"
#include "core/RunnerLayerInstances.h"
#include "core/RunnerLayerLifetimes.h"
#include "core/StoredLayerLifecycle.h"
#include "core/InstanceLifecycle.h"
#include "core/StoredRoomLayers.h"
bool compileStoredRoomLayers(downwell::StoredRoomLayers<8,4,16,2>& owner,
    const downwell::RoomPack& pack,downwell::InstanceStore<8>& instances,int room){
    return owner.leave()&&owner.prepare(pack,room,instances)&&owner.serviceDepths()&&owner.visitDraw(
        [](auto,unsigned,auto& state){state.flags=0x10;state.object=true;state.sprite=true;return true;},
        [](auto,unsigned,int,auto){return true;},[](const auto&,int){return true;});
}
bool compileLayerObserver(downwell::InstanceLifecycleBinding& binding,
    downwell::StoredLayerLifecycle<4,16,8>& observer,downwell::InstanceHandle h,const downwell::InstanceRecord& r){
    return binding.observeLayers(observer)&&binding.birth(h,r)&&binding.activation(h,r,false)&&
        binding.membership(h,r,false)&&binding.publishedDepth(h,r,downwell::MotionBinary64::fromInteger(-60000))&&binding.retire(h,r);
}
bool compileLayerLifetimes(downwell::RunnerLayerLifetimes<4,16,8>& owner,downwell::InstanceHandle h,unsigned id){
    return owner.birth(h,id,{},false)&&owner.writeDepth(h,downwell::MotionBinary64::fromInteger(-60000))&&
        owner.serviceDepths()&&owner.retire(h);
}
bool compileLayerRoomLifetimes(downwell::RunnerLayerLifetimes<4,16,8>& owner,
    const downwell::RunnerLayerLifetimes<4,16,8>::State& state){
    return owner.adopt(state)&&owner.initializeRoom([&](auto visit){return visit(state.handle);},
        [](unsigned){return false;})&&owner.uninitializeRoom([](unsigned){return false;});
}
#include "core/PackedRoomLayerLoader.h"
#include "core/RunnerLayerBackground.h"
#include "core/RunnerLayerBackgrounds.h"
#include "core/RunnerLayerBackgroundDraw.h"
bool compileBackgroundDraw(const downwell::RunnerLayerBackground& background,unsigned& color){
    return downwell::RunnerLayerBackgroundDraw::visit(background,[](int,bool& exists){exists=false;return true;},
        [&](unsigned value){color=value;return true;},[](const auto&){return false;});
}
bool compileLayerBackgroundOwner(downwell::RunnerLayerBackgrounds<4,16,2>& owner,
    downwell::RunnerLayers<4,16>& layers,unsigned layer,const downwell::PackedRoomLayer& source){
    return owner.create(layers,layer,source,[](unsigned){return false;});
}
bool compileLayerBackground(const downwell::PackedRoomLayer& source,downwell::RunnerLayerBackground& out){
    if(!downwell::RunnerLayerBackground::create(source,out))return false;
    out.initialize();return out.initialized;
}
#include "core/RoomLoader.h"
bool compileRoomPreparation(downwell::InstanceStore<4>& store){
    return downwell::RoomLoader<4>::enterChecked(store,1,nullptr,0,false,
        [](auto&,auto){return true;},[](int){return true;},[](auto&){return true;},
        downwell::NoInstanceLifecycle{},[](auto&,int){return true;})==downwell::LoadRoomResult::success;
}
bool compilePackedLayers(const downwell::RoomPack& pack,int room,downwell::RunnerLayers<4,16>& layers,
    downwell::RunnerLayerInstances<4,16>& registry,downwell::RunnerLayerIds& ids){
    return downwell::PackedRoomLayerLoader::load(pack,room,layers,registry,[](unsigned,const auto&){return true;},&ids);
}
bool compileLayerIds(downwell::RunnerLayerIds& ids,downwell::RunnerLayers<4,16>& layers,int depth,unsigned& slot){
    return ids.createDynamic(layers,depth,slot);
}
bool compileLayerInstances(downwell::RunnerLayerInstances<4,16>& registry,downwell::RunnerLayers<4,16>& layers,
    unsigned layer,unsigned id,downwell::InstanceHandle handle,downwell::RunnerDepthService::Instance& state){
    return registry.reserve(layers,layer,id)&&registry.attach(layers,layer,id,handle,state,false)&&registry.retire(layers,id,handle);
}
bool compileRoomLayerInitialization(downwell::RunnerLayerInstances<4,16>& registry,
    downwell::RunnerLayers<4,16>& layers,downwell::RunnerDepthService::Instance& state,
    downwell::InstanceHandle handle,unsigned sourceId){
    return registry.initializeRoom(layers,
        [&](unsigned id,downwell::InstanceHandle& h,downwell::RunnerDepthService::Instance*& out){
            if(id==sourceId){h=handle;out=&state;}return true;
        },[&](auto visit){return visit(sourceId,handle,state,false);},
        [](unsigned){return false;},[](int,unsigned&){return false;});
}
bool compileLayerDraw(const downwell::RunnerLayers<4,16>& layers,unsigned flags){
    return downwell::RunnerLayerDraw::visit(layers,[](unsigned,const auto&){return true;},
        [](unsigned,const auto&){return true;},[&](unsigned,auto& e){e={2,flags,true,true,true,true};return true;},
        [](unsigned,unsigned,auto){return true;},[](unsigned,const auto&){return true;});
}
bool compileRoomLayerFallback(downwell::RunnerLayerInstances<4,16>& registry,
    downwell::RunnerLayers<4,16>& layers,unsigned id,downwell::InstanceHandle handle,
    downwell::RunnerDepthService::Instance& state,int nextLayerId){
    return registry.attachRoomFallback(layers,id,handle,state,false,[&](int depth,unsigned& slot){
        return layers.insertAfterEquals(nextLayerId,depth,true,slot);
    });
}
bool compileReservedLayerInitialization(downwell::RunnerLayerInstances<4,16>& registry,
    downwell::RunnerLayers<4,16>& layers,unsigned element,downwell::InstanceHandle handle,
    downwell::RunnerDepthService::Instance& state){
    return registry.initializeReserved(layers,element,
        [&](unsigned,downwell::InstanceHandle& h,downwell::RunnerDepthService::Instance*& out){
            h=handle;out=&state;return true;
        });
}
bool compileDepthChanges(downwell::RunnerDepthChanges<16>& queue,downwell::RunnerLayers<4,16>& layers,
    downwell::RunnerDepthService::Instance& instance,unsigned element,downwell::MotionBinary64 value){
    return queue.request(element,instance.depth,value)&&queue.service([&](unsigned e){
        return downwell::RunnerDepthService::apply(layers,instance,e,[&](unsigned found){return e==found;},
            [](int,unsigned&){return false;},[](unsigned,int){return true;},[](unsigned){return false;});
    });
}
bool compileLayers(downwell::RunnerLayers<4,16>& owner,unsigned slot,int depth){
    return owner.move(slot,depth,true,[](unsigned,int){return true;},[](unsigned){return false;});
}
bool compileLayerActivation(downwell::RunnerLayers<4,16>& owner,unsigned element,bool inactive){
    return owner.reconcileInstanceActivation(element,inactive);
}
bool compileRoomLayerNormalization(downwell::RunnerLayers<4,16>& owner,unsigned inactive){
    return owner.normalizeRoomInstances([&](unsigned element){return element==inactive;});
}
bool compileLayerElements(downwell::RunnerLayerElements<4,16>& owner,unsigned slot,unsigned inactive){
    return owner.insert(0,slot,true,false,[](unsigned){return false;}) &&
        owner.concatenate(1,0) && owner.normalize(1,[&](unsigned i){return i==inactive;}) &&
        owner.visitForward(1,[](unsigned){return true;}) && owner.remove(slot);
}
bool compileInstanceHandles(downwell::InstanceStore<4>& store,unsigned id){
    if(store.create(id,82,1,false)!=downwell::CreateInstanceResult::success)return false;
    auto handle=store.handleAt(store.size()-1);store.enterRoom(2);
    return store.resolve(handle)==nullptr;
}
bool compileGlobalInstanceOrder(const downwell::InstanceStore<4>& store,downwell::RunnerStepSnapshot<4>& snapshot){
    return store.captureGlobalInstanceOrder(snapshot);
}
#include "core/RunnerSpatialSplit.h"
#include "core/RunnerSpatialNode.h"
#include "core/RunnerSpatialIndex.h"
#include "core/RunnerSpatialDirtyQueue.h"
#include "core/RunnerSpatialBoundsUpdate.h"
#include "core/RunnerSpriteBounds.h"
#include "core/SpritePack.h"
#include "core/SpriteCollisionPack.h"
#include "core/SpriteMaskPack.h"
#include "core/RunnerMaskTransform.h"
#include "core/RunnerMaskDispatch.h"
#include "core/RunnerMaskPair.h"
bool compileMaskPair(const downwell::RunnerMaskPair::Shape& a,
    const downwell::RunnerMaskPair::Shape& b,downwell::RunnerMaskPair::Result& result,
    downwell::RunnerImageValue c,downwell::RunnerImageValue s){
    return downwell::RunnerMaskPair::test(a,b,{},
        [&](auto,auto& cosine,auto& sine){cosine=c;sine=s;return true;},
        [](auto value,std::int32_t& pixel){return value.promoted().roundPixel(pixel);},result);
}
#include "core/RunnerPlaceQuery.h"
#include "core/RunnerSpatialInstances.h"
#include "core/RunnerObjectFamilies.h"
#include "core/RoomLoader.h"
#include "core/StoredCollisionLifecycle.h"
bool compileStoredCollisionQuery(downwell::RunnerSpatialInstances<4,8>& owner,
    downwell::RunnerSpatialInstances<4,8>::Properties (&properties)[4],
    const downwell::InstanceStore<4>& records,const downwell::ObjectPack& objects,
    const downwell::SpriteCollisionPack& sprites,const downwell::SpriteMaskPack& masks,
    downwell::RunnerObjectFamilies<386>& graph,downwell::InstanceHandle self,
    downwell::MotionBinary64 x,downwell::MotionBinary64 y,
    downwell::RunnerSpatialInstances<4,8>::Result& result){
    auto trig=[](auto,auto&,auto&){return false;};
    auto round=[](auto value,std::int32_t& out){return value.promoted().roundPixel(out);};
    downwell::StoredCollisionLifecycle<4,8,decltype(graph),decltype(trig),decltype(round)>
        lifecycle{owner,properties,objects,sprites,graph,trig,round};
    return lifecycle.placeVm(records,self,82,x,y,masks,{},trig,result);
}
bool compileStoredCollisionBirth(downwell::RunnerSpatialInstances<4,8>& owner,
    downwell::RunnerSpatialInstances<4,8>::Properties (&properties)[4],
    const downwell::ObjectPack& objects,const downwell::SpriteCollisionPack& sprites,
    downwell::RunnerObjectFamilies<386>& graph,downwell::InstanceHandle handle,
    const downwell::InstanceRecord& record){
    auto trig=[](auto,auto&,auto&){return false;};
    auto round=[](auto value,std::int32_t& out){return value.promoted().roundPixel(out);};
    downwell::StoredCollisionLifecycle<4,8,decltype(graph),decltype(trig),decltype(round)>
        lifecycle{owner,properties,objects,sprites,graph,trig,round};
    auto binding=downwell::InstanceLifecycleBinding::bind(lifecycle);
    return binding.birth(handle,record)&&binding.activation(handle,record,true)&&
        binding.membership(handle,record,true)&&lifecycle.move(handle,record,{}, {})&&
        lifecycle.mask(handle,record,record.maskId)&&
        lifecycle.sprite(handle,record,record.spriteId)&&
        lifecycle.transform(handle,record,downwell::RunnerSpatialInstances<4,8>::TransformField::scaleX,
                            {0x3f800000u},{0x3f800000u},0xffffffu)&&
        lifecycle.refresh(handle,record,true)&&
        lifecycle.rebind(handle,record,record.objectId,true);
}
bool compileResourceObjectBinding(downwell::RunnerSpatialInstances<4,8>& owner,
    downwell::InstanceHandle h,int object,downwell::RunnerSpatialInstances<4,8>::Properties& properties,
    const downwell::ObjectPack& objects,const downwell::SpriteCollisionPack& sprites,
    const downwell::RunnerObjectFamilies<386>& graph){
    return owner.bindObjectFromPacks(h,object,true,properties,objects,sprites,graph,false);
}
bool compileRoomSpatialLifecycle(downwell::InstanceStore<4>& store,
    downwell::RunnerSpatialInstances<4,8>& owner,const downwell::RoomInstanceDefinition* rooms,unsigned count){
    struct Lifecycle {
        downwell::RunnerSpatialInstances<4,8>& owner;
        bool birth(downwell::InstanceHandle h,const downwell::InstanceRecord& record){
            return owner.attach(h,record.objectId,{});
        }
        bool retire(downwell::InstanceHandle h,const downwell::InstanceRecord&){return owner.retire(h);}
    } lifecycle{owner};
    return downwell::RoomLoader<4>::enter(store,2,rooms,count,false,
        [](auto&,auto){},[](int){},[](auto&){},lifecycle)==downwell::LoadRoomResult::success;
}
bool compileSpatialInstances(downwell::RunnerSpatialInstances<64,64>& owner,
    downwell::RunnerObjectFamilies<386>& graph,downwell::InstanceHandle self,
    downwell::RunnerSpatialInstances<64,64>::Result& result){
    return owner.place(self,0,{}, {},false,graph,
        [&](downwell::InstanceHandle h,bool publish){
            const auto* state=owner.resolve(h);if(!state)return false;
            const auto bounds=state->bounds;
            return owner.refresh(h,0x30,false,&bounds,false,publish);
        },[](auto,auto,bool,bool& hit){hit=true;return true;},result);
}
bool compileSpatialRetirement(downwell::RunnerSpatialInstances<64,64>& owner,
    downwell::InstanceHandle handle){return owner.retire(handle);}
bool compileLiveSpatialRemoval(downwell::RunnerSpatialInstances<64,64>& owner,
    downwell::InstanceHandle handle){return owner.removeSpatialMembership(handle);}
bool compileBindingProperties(downwell::RunnerSpatialInstances<64,64>& owner,
    downwell::InstanceHandle handle,std::uint32_t flags,bool allPrepared){
    return owner.applyBindingProperties(handle,flags,allPrepared,[](downwell::InstanceHandle){return true;});
}
bool compileOwnedSpriteChange(downwell::RunnerSpatialInstances<64,64>& owner,
    downwell::InstanceHandle handle,downwell::RunnerSpatialInstances<64,64>::Sprite& sprite,
    downwell::RunnerImageValue& timeline,downwell::RunnerImageValue& cached,int next,unsigned frames){
    return owner.changeSprite(handle,sprite,timeline,cached,next,frames,0x38);
}
bool compileResourceSpriteChange(downwell::RunnerSpatialInstances<64,64>& owner,
    downwell::InstanceHandle handle,downwell::RunnerSpatialInstances<64,64>::Sprite& sprite,
    downwell::RunnerImageValue& timeline,downwell::RunnerImageValue& cached,int next,
    const downwell::SpriteCollisionPack& pack){
    return owner.changeSpriteFromPack(handle,sprite,timeline,cached,next,pack,0x38);
}
bool compileStoreSpatialRetirement(downwell::InstanceStore<4>& store,
    downwell::RunnerSpatialInstances<4,8>& owner){
    return store.enterRoom(2,[&](auto handle,const auto&){return owner.retire(handle);})&&
        store.clear([&](auto handle,const auto&){return owner.retire(handle);});
}
bool compileSpatialBirth(downwell::RunnerSpatialInstances<64,64>& owner,
    downwell::RunnerObjectFamilies<386>& graph,downwell::InstanceHandle handle){
    return owner.publishBirth(handle,graph,[&](auto h,bool publish){
        const auto* s=owner.resolve(h);if(!s)return false;
        const auto box=s->bounds;
        return owner.refresh(h,0x30,false,&box,false,publish);
    });
}
bool compileSpatialSpriteRefresh(downwell::RunnerSpatialInstances<64,64>& owner,
    downwell::InstanceHandle handle,const downwell::SpriteCollisionPack& pack,
    const downwell::RunnerSpatialInstances<64,64>::Sprite& sprite,
    downwell::RunnerImageValue cosine,downwell::RunnerImageValue sine){
    return owner.refreshSprite(handle,0x30,false,pack,sprite,true,
        [&](auto,auto& c,auto& s){c=cosine;s=sine;return true;},
        [](auto value,std::int32_t& pixel){return value.promoted().roundPixel(pixel);},true);
}
bool compileOwnedCollision(downwell::RunnerSpatialInstances<64,64>& owner,
    const downwell::SpriteCollisionPack& sprites,const downwell::SpriteMaskPack& masks,
    const downwell::RunnerSpatialInstances<64,64>::Sprite* visuals,bool& hit){
    auto trig=[](auto,auto& c,auto& s){c={0x3f800000};s={};return true;};
    auto round=[](auto value,std::int32_t& pixel){return value.promoted().roundPixel(pixel);};
    return owner.collideCompatibility({0,1},{1,1},visuals[0],visuals[1],sprites,masks,{},true,
        [&](auto h,bool publish){return owner.refreshSprite(h,0x30,false,sprites,visuals[h.slot],true,trig,round,publish);},
        trig,round,hit);
}
bool compilePlaceQuery(downwell::RunnerImageValue& x,downwell::RunnerImageValue& y,
    std::uint32_t& flags,downwell::RunnerPlaceQuery::Box& box,downwell::RunnerPlaceQuery::Result& result){
    using Query=downwell::RunnerPlaceQuery;
    return Query::run(x,y,flags,box,{}, {},
        []{return Query::Path::spatial;},[]{return true;},[&](bool){flags&=~8u;return true;},
        [&](auto nx,auto ny){x=nx;y=ny;flags|=8u;return true;},
        [](auto visit){visit(7);return true;},[](auto,auto visit){visit(8);return true;},
        [](unsigned,bool,bool& hit){hit=true;return true;},result);
}
downwell::RunnerMaskDispatch::Path compileMaskDispatch(downwell::RunnerMaskDispatch::Shape a,
    downwell::RunnerMaskDispatch::Shape b,downwell::MotionBinary64 epsilon){
    downwell::RunnerMaskDispatch dispatch;
    if(!dispatch.configure(epsilon))return downwell::RunnerMaskDispatch::Path::unsupported;
    return dispatch.select(a,b);
}
bool compileMaskTransform(downwell::RunnerMaskTransform& transform,downwell::RunnerImageValue scale,
    downwell::RunnerImageValue c,downwell::RunnerImageValue s,const downwell::SpriteMaskPack::Mask& mask,bool& hit){
    return transform.prepare({}, {},scale,scale,c,s,24,24)&&transform.contains(mask,1,2,hit);
}
bool compileAxisMaskTransform(downwell::RunnerMaskTransform& transform,downwell::RunnerImageValue scale,
    const downwell::SpriteMaskPack::Mask& mask,bool& hit){
    return transform.prepareAxisAligned({}, {},scale,scale,24,24)&&transform.contains(mask,1,2,hit);
}
bool compilePreciseMask(const std::uint8_t* bytes,std::size_t size,unsigned sprite,int x,int y){
    downwell::SpriteMaskPack pack(bytes,size);downwell::SpriteMaskPack::Mask mask;
    return pack.mask(sprite,0,mask)&&mask.contains(x,y);
}
bool compileCompactCollision(const std::uint8_t* bytes,std::size_t size,int id,
    downwell::RunnerSpriteBounds::Box& result){
    downwell::SpriteCollisionPack pack(bytes,size);downwell::PackedSpriteCollision sprite;
    return pack.collisionSprite(id,-1,sprite)&&downwell::RunnerSpriteBounds::fromSprite(sprite,
        {}, {},{0x3f800000},{0x3f800000},false,{}, {},true,
        [](downwell::RunnerImageValue value,std::int32_t& pixel){return value.promoted().roundPixel(pixel);},result);
}
bool compilePackedSpriteBounds(const downwell::PackedSprite& sprite,downwell::RunnerSpriteBounds::Box& result){
    return downwell::RunnerSpriteBounds::fromSprite(sprite,{}, {},{0x3f800000},{0x3f800000},false,{}, {},true,
        [](downwell::RunnerImageValue value,std::int32_t& pixel){return value.promoted().roundPixel(pixel);},result);
}
bool compileRunnerSpriteBounds(downwell::RunnerSpriteBounds::Box margins,
    downwell::RunnerImageValue scale,downwell::RunnerSpriteBounds::Box& result){
    return downwell::RunnerSpriteBounds::unrotated(margins,12,12,{}, {},scale,scale,true,
        [](downwell::RunnerImageValue value,std::int32_t& pixel){return value.promoted().roundPixel(pixel);},result);
}
bool compileRunnerRotatedSpriteBounds(downwell::RunnerSpriteBounds::Box margins,
    downwell::RunnerImageValue scale,downwell::RunnerImageValue c,downwell::RunnerImageValue s,
    downwell::RunnerSpriteBounds::Box& result){
    return downwell::RunnerSpriteBounds::rotated(margins,12,12,{}, {},scale,scale,c,s,true,
        [](downwell::RunnerImageValue value,std::int32_t& pixel){return value.promoted().roundPixel(pixel);},result);
}
#include "core/RunnerSpatialPosition.h"
bool compileRunnerSpatialPosition(downwell::RunnerImageValue& x,downwell::RunnerImageValue& y,std::uint32_t& flags,
    downwell::RunnerSpatialDirtyQueue<256>& spatial,downwell::RunnerSpatialDirtyQueue<256>& collision){
    return downwell::RunnerSpatialPosition::assign(1,x,y,flags,{},{},false,true,0x1a,spatial,collision);
}
bool compileRunnerSpatialBoundsUpdate(downwell::RunnerSpatialIndex<256>& index,downwell::RunnerSpatialSplit::Box box,std::uint32_t& flags){
    return downwell::RunnerSpatialBoundsUpdate::finishRefresh(index,123,0x30,flags,false,
        box,&box,{}, {},true,true);
}
bool compileRunnerSpatialDirtyQueue(downwell::RunnerSpatialDirtyQueue<256>& queue,unsigned slot){
    return queue.mark(slot)&&queue.remove(slot);
}
bool compileRunnerSpatialFlush(downwell::RunnerSpatialDirtyQueue<256>& queue,
    downwell::RunnerSpatialIndex<256>& index,std::uint32_t* flags,
    downwell::RunnerSpatialSplit::Box* boxes){
    return queue.flush([&](unsigned i){return flags[i];},[&](unsigned i){
        return downwell::RunnerSpatialBoundsUpdate::finishRefresh(index,i,0x30,
            flags[i],false,boxes[i],&boxes[i],{}, {},false,true);
    });
}
bool compileRunnerSpatialIndex(downwell::RunnerSpatialIndex<256>& index,downwell::RunnerSpatialSplit::Box box,unsigned id,unsigned& found){
    return index.insert(box,id)&&index.query(box,[&](unsigned hit){found=hit;return false;});
}
bool compileRunnerSpatialRemoval(downwell::RunnerSpatialIndex<256>& index,downwell::RunnerSpatialSplit::Box box,unsigned id,bool& removed){
    return index.remove(box,id,true,removed);
}
bool compileRunnerSpatialNode(downwell::RunnerSpatialNode& node,downwell::RunnerSpatialNode& sibling,const downwell::RunnerSpatialNode::Entry& entry,unsigned index){
    return node.erase(index)&&node.append(entry)&&node.split(entry,sibling);
}
bool compileRunnerSpatialSplit(const downwell::RunnerSpatialSplit::Box (&boxes)[7],downwell::RunnerSpatialSplit::Result& result){
    return downwell::RunnerSpatialSplit::partition(boxes,result);
}
#include "core/BloodEffect.h"
#include "core/BloodEffects.h"
#include "core/BloodCollisionBounds.h"
#include "core/BloodMaskSample.h"
#include "core/BloodGoalContact.h"
#include "core/OrdinaryTimeField.h"
bool compileOrdinaryTimeField(downwell::OrdinaryTimeField& field,downwell::GameMakerRandom& random,downwell::ContactBounds bounds){
    field.create(240,512,random);field.advanceDraw(false,random);
    return field.meetsRelativeBounds(bounds);
}
#include "core/OrdinaryTimeFields.h"
const downwell::OrdinaryTimeField* compileFieldLifetime(downwell::OrdinaryTimeFields<64>& fields,
    std::uint32_t identity,downwell::OrdinaryTimeFields<64>::BirthSequence sequence){
    if(!fields.restoreEmptyBirthSequence(sequence))return nullptr;
    if(!fields.adoptCreate(240,512,1))return nullptr;
    return fields.find(identity);
}
bool compileOrdinaryTimeFieldOrder(downwell::OrdinaryTimeFields<64>& fields,downwell::GoalTimeField& goal,downwell::GameMakerRandom& random,unsigned boundary){
    return fields.advanceDraw(false,random,true,boundary,[&]{goal.advanceDraw(false,random);});
}
#include "core/TimeStopGoalContact.h"
bool compileBloodOrdinaryContact(const downwell::BloodEffect& blood,const downwell::OrdinaryTimeField& field,bool& hit){
    return downwell::BloodGoalContact::query(blood,field,hit);
}
bool compileOrdinaryTimeStopContact(downwell::RunnerImageValue x,downwell::RunnerImageValue scale,const downwell::OrdinaryTimeField& field,bool& hit){
    return downwell::TimeStopGoalContact::query(downwell::TimeStopGoalContact::Mask::Shard,x,x,scale,scale,field,hit);
}
bool compileTimeStopGoalContact(downwell::RunnerImageValue x,downwell::RunnerImageValue scale,const downwell::GoalTimeField& field,bool& hit){
    return downwell::TimeStopGoalContact::query(downwell::TimeStopGoalContact::Mask::Player,x,x,scale,scale,field,hit);
}
#include "core/RunSession.h"
bool compileRuntimeTimeField(downwell::RunSession& state,int x,int y,downwell::GameMakerRandom& random){
    return state.createTimeField(x,y,random);
}
bool compileParentTimeFieldContact(downwell::RunSession& state,downwell::MotionBinary64 x,
                                  downwell::RunnerImageValue scale,bool& hit){
    return state.parentTimeFieldContact(downwell::TimeStopGoalContact::Mask::Player,x,x,scale,scale,hit);
}
#include "core/PlayerTimeStop.h"
#include "core/TimeChangeEffect.h"
#include "core/TimeShardBirth.h"
#include "core/TimeShardLifecycle.h"
#include "core/TimeShardMotion.h"
#include "core/TimeShardAnimation.h"
#include "core/TimeShardEffects.h"
#include "core/TimeShardBurst.h"
#include "core/RunnerPointDirection.h"
#include "core/PortableAtan.h"
#include "core/TimeStopEffects.h"
bool compileTimeStopEffects(downwell::TimeStopEffects<24>& state,downwell::GameMakerRandom& random,bool hit){
    const auto phase=state.changes.beginPhase();
    state.changes.advanceAlarm(phase,0,state.roomSpeed);
    state.changes.advanceAlarm(phase,1,state.roomSpeed);
    return state.beginStep(true,false,{},false,random,[&](bool& result){result=hit;return true;},
        [](auto,auto,auto& id,auto& x,auto& y){id=1;x={};y={};return true;},
        [](auto){return false;},[](auto){return true;});
}
bool compilePortableAtan(downwell::MotionBinary64 x,downwell::MotionBinary64 y,downwell::MotionBinary64& out){
    return downwell::RunnerPointDirection::evaluate({}, {},x,y,out,downwell::portableAtan);
}
bool compilePointDirection(downwell::MotionBinary64 x,downwell::MotionBinary64 y,downwell::MotionBinary64& out){
    return downwell::RunnerPointDirection::evaluate({}, {},x,y,out,
        [](auto dy,auto,auto& angle){angle=dy;return true;});
}
bool compileTimeShardBurst(downwell::TimeShardBurst& burst,downwell::TimeShardEffects<24>& owner,downwell::GameMakerRandom& random){
    return burst.emit({}, {},{}, {},false,random,owner,
        [](auto,auto,auto& id,auto&,auto&){id=-4;return true;},
        [](auto,auto,auto,auto,auto& out){out={};return true;},
        [](auto length,auto,bool,auto& out){out=length;return true;});
}
bool compileTimeShardEffects(downwell::TimeShardEffects<24>& owner,downwell::GameMakerRandom& random,bool stopped){
    const auto phase=owner.beginPhase();
    owner.advanceAlarm(phase,0);
    return owner.advanceAnimation(phase) && owner.step(phase,stopped,random,
        [&](auto,const auto&,auto,auto,bool& hit){hit=stopped;return true;},
        [](auto,const auto&,bool& retire){retire=false;return true;});
}
bool compileTimeShardAnimation(downwell::TimeShardAnimation& shard){return shard.advanceImage();}
bool compileTimeShardMotion(downwell::TimeShardMotion& shard,downwell::GameMakerRandom& random,bool stopped){
    return shard.step(stopped,random,[&](auto,auto,bool& hit){hit=stopped;return true;},
        [](){return 1;},[](const auto&,bool& retire){retire=false;return true;});
}
bool compileTimeShardLifecycle(downwell::TimeShardLifecycle& shard,downwell::GameMakerRandom& random,unsigned alarm){
    shard.advanceAlarm(alarm);
    return shard.playerContact({},downwell::MotionBinary64::fromInteger(-1),random);
}
bool compileTimeShard(downwell::TimeShardBirth& shard,downwell::GameMakerRandom& random){
    return shard.create({}, {},false,random);
}
bool compileTimeChange(downwell::TimeChangeEffect& effect,int& speed){
    if(!effect.create(speed,[](unsigned){return true;}))return false;
    effect.advanceAlarms(speed);return !effect.destroyRequested;
}
bool compilePlayerTimeStop(downwell::PlayerTimeStop& state,bool main,bool side,bool field){
    return state.step(main,side,[&](bool& hit){hit=field;return true;},
        [](auto){return false;},[](auto){return true;});
}
bool compileBloodSession(downwell::RunSession& run,bool stopped){
    run.stepBloodBound(run.bloodEffects.beginPhase(),stopped,480,
        [](const auto&,bool& hit){hit=false;return true;});
    return run.effectsSucceeded;
}
bool compileBloodGoal(const downwell::BloodEffect& effect,const downwell::GoalTimeField& field,bool& hit){
    return downwell::BloodGoalContact::query(effect,field,hit);
}
bool compileBloodSample(const downwell::BloodEffect& effect,int x,int y,bool& hit){
    downwell::BloodMaskSample sample;return sample.prepare(effect)&&sample.contains(x,y,hit);
}
bool compileBloodBounds(const downwell::BloodEffect& effect,downwell::ContactBounds& bounds){
    return downwell::BloodCollisionBounds::calculate(effect,bounds);
}
#include "core/TimeStopBound.h"
#include "core/GoalTimeField.h"
bool compileGoalFieldBounds(downwell::ContactBounds bounds){
    return downwell::GoalTimeField::meetsRelativeBounds(bounds);
}
bool compileTimeStopBound(bool stopped,downwell::MotionBinary64 x,int width,bool inField,bool& allowed){
    return downwell::TimeStopBound::evaluate(stopped,x,{},width,
        [&](auto,auto,bool& hit){hit=inField;return true;},allowed);
}
bool compileBloodOwner(downwell::BloodEffects<32>& owner, bool allowed) {
    if(!owner.create({}, {},{}))return false;
    const auto phase=owner.beginPhase();
    return owner.advanceAnimation(phase) && owner.step(phase,[&](auto,const auto&){return allowed;});
}
bool compileBloodEffect(downwell::BloodEffect& effect,downwell::MotionBinary64 x,
    downwell::MotionBinary64 y,downwell::MotionBinary64 angle,bool timeAllowed) {
    return effect.create(x,y,angle) && effect.advanceImage() && effect.step(timeAllowed);
}
#include "core/PaletteEffectSurface.h"
bool compilePaletteEffect(downwell::PaletteEffectSurface<>& surface,
    const downwell::PaletteEffectSurface<>::Asset& asset,int x,int y) {
    surface.clear();return surface.add(asset,x,y) && surface.subtract(asset.alpha,x-32,y-16);
}
#include "core/EffectMaskAssets.h"
#include "core/SmokeComposition.h"
#include "core/CurrencySmokeComposition.h"
#include "core/SmokeEffects.h"
#include "core/StoredSmokeRegistration.h"
#include "core/SmokeInstances.h"
bool compileNativeSmoke(downwell::SmokeInstances<32>& owner,downwell::GameMakerRandom& random,std::uint32_t id){
    downwell::InstanceHandle key;
    return owner.create(id,{}, {},random,key)&&owner.step(key,random)&&owner.advanceAnimation(key)&&
        owner.alarmEvent(key,0)&&owner.wallContact(key)&&owner.activate(key,false)&&owner.clear();
}
bool compileRegisteredSmoke(downwell::StoredSmokeRegistration<16,4>& owner,
    downwell::GameMakerRandom& random,const downwell::InstanceRecord& record,std::uint32_t& id){
    downwell::SmokeBurstCaller caller;
    return owner.skeletonBurst(caller,{}, {},random)&&owner.create({}, {},random,id)&&owner.step(record,random)&&
        owner.advanceAnimation(record)&&owner.alarmEvent(record,0)&&owner.wallContact(record)&&owner.clear();
}
bool compileSmokePublication(downwell::SmokeMotion& smoke,downwell::GameMakerRandom& random,bool& good){
    struct Publish {
        bool imageSpeed(downwell::RunnerImageValue){return true;}
        bool position(downwell::RunnerImageValue,downwell::RunnerImageValue){return true;}
    } publish;
    return smoke.stepPublished(random,[](auto speed,auto direction,auto& x,auto& y){
        return downwell::SmokeDirection::vector(speed,direction,x,y,downwell::smokeTrig);
    },publish,downwell::smokeEscapedDistance,good);
}
bool compileSmokeContacts(downwell::SmokeEffects<32>& smoke){
    return smoke.collideWalls(smoke.beginPhase(),[](downwell::ContactBounds,auto emit){return emit();});
}
bool compileCurrencySmokeComposition(downwell::CurrencySmokeComposition& composition,
    const downwell::SmokeEffects<32>& smoke,const downwell::EffectMaskAssets& assets,
    const downwell::EffectSurfacePlacement::View& view,
    downwell::SmokeComposition::Settings settings) {
    return composition.prepare([&](auto emit){emit(false,0,view.x,view.y);},
        smoke,assets,view,settings,[&](auto emit){emit(view.x,view.y);},
        [](downwell::MotionBinary64 n,std::int32_t& out){return n.roundPixel(out);});
}
bool compileLateEffectComposition(downwell::CurrencySmokeComposition& composition,
    const downwell::SmokeEffects<32>& smoke,const downwell::EffectMaskAssets& assets,
    const downwell::EffectSurfacePlacement::View& view){
    return composition.prepareWithLateEffects([](auto){},smoke,assets,view,{},
        [&](auto emit){downwell::EffectMaskAssets::Asset asset;
            return assets.debris(115,0,asset)&&emit(asset,view.x,view.y);},
        [](auto){},[](downwell::MotionBinary64 n,std::int32_t& out){return n.roundPixel(out);});
}
#include "core/RunnerObjectMembership.h"
#include "core/RunnerObjectFamilies.h"
bool compileObjectFamilies(downwell::RunnerObjectFamilies<386>& families,
    const downwell::ObjectPack& objects,unsigned root,std::uint32_t initialFlags){
    return families.bind(objects,[&](const auto&){return initialFlags;}) &&
        families.markSpatialFamily(root) && families.prepareBirth(root);
}
bool compileInitialSpatialFlags(downwell::RunnerObjectFamilies<386>& families,
    const downwell::ObjectPack& objects){return families.bindSpatialFlags(objects);}
bool compileInitialInstanceFlags(downwell::RunnerObjectFamilies<386>& families,
    const downwell::ObjectPack& objects){return families.bindInstanceFlags(objects);}
#include "core/RunnerSpatialObjectPreparation.h"
downwell::RunnerSpatialObjectPreparation::Path compileObjectPreparation(
    downwell::RunnerSpatialIndex<64>& index,std::uint32_t& flags,
    downwell::RunnerSpatialSplit::Box& box){
    return downwell::RunnerSpatialObjectPreparation::prepare(1,index.height(),0,false,
        []{return true;},[&](auto emit){emit(1,flags,box);},
        [](auto,auto&,auto&){return true;},
        [&](auto bounds,auto id){return index.insert(bounds,id);});
}
#include "core/RunnerRoomMembership.h"
#include "core/EventScheduler.h"
bool compileSpatialObjectRebind(downwell::InstanceStore<4>& store,
    downwell::RunnerSpatialInstances<4,8>& owner,downwell::InstanceHandle handle,
    int object,bool registered) {
    return store.bindObject(handle,object,registered,
        [&](downwell::InstanceHandle h,const downwell::InstanceRecord&,int next,bool member){
            return owner.rebindObjectMembership(h,next,member);
        });
}
bool compileSpatialMembershipStep(downwell::InstanceStore<4>& store,
    downwell::RunnerSpatialInstances<4,8>& owner) {
    return downwell::EventScheduler<4>::runStep(store,
        [](const downwell::InstanceRecord&){return downwell::AlarmBank::FiredMask(0);},
        [](downwell::InstanceRecord&,downwell::StepEvent){},
        [](downwell::StepEventPhase){},
        [&](downwell::InstanceHandle h,const downwell::InstanceRecord&,bool registered){
            return owner.registerObject(h,registered);
        });
}
bool compileSpatialActivation(downwell::RunnerSpatialInstances<4,8>& owner,
    downwell::InstanceHandle handle,bool active,std::uint32_t objectFlags) {
    return owner.setActive(handle,active,objectFlags);
}
bool compileStoredSpatialActivation(downwell::InstanceStore<4>& store,
    downwell::RunnerSpatialInstances<4,8>& owner,downwell::InstanceHandle handle,
    bool active,std::uint32_t objectFlags) {
    return store.requestActivation(handle,active,
        [&](downwell::InstanceHandle h,const downwell::InstanceRecord&,bool requested){
            return owner.setActive(h,requested,objectFlags);
        });
}
unsigned compileRoomMembership(downwell::RunnerRoomMembership<64>& rooms,
    unsigned slot,downwell::RunnerImageValue depth,std::uint64_t dormant) {
    if(!rooms.bind(slot,depth))return 0;
    unsigned publications=0;
    rooms.reconcile([&](unsigned member){return (dormant>>member)&1;},
        [&](unsigned,bool){++publications;});
    return publications;
}
unsigned compileObjectMembership(downwell::RunnerObjectMembership<64>& registry,
    unsigned slot,int object,std::uint32_t identity,int parent) {
    if(!registry.bind(slot,object,identity))return 0;
    return registry.count(parent,[](int child,int ancestor){return child==ancestor+1;});
}
bool compileSmokeComposition(downwell::SmokeComposition& composition,
    const downwell::SmokeEffects<32>& smoke,const downwell::EffectMaskAssets& assets,
    const downwell::EffectSurfacePlacement::View& view,
    downwell::SmokeComposition::Settings settings) {
    return composition.prepare(smoke,assets,view,settings,
        [&](auto emit){emit(view.x,view.y);},
        [](downwell::MotionBinary64 n,std::int32_t& out){return n.roundPixel(out);});
}
bool compileEffectAssets(downwell::EffectMaskAssets& assets,const std::uint8_t* pack,std::size_t size) {
    downwell::EffectMaskAssets::Asset debris;
    return assets.bind(pack,size) && assets.debris(91,0,debris);
}
bool compileBinaryEffect(downwell::BinaryEffectSurface<>& surface,
    const downwell::BinaryEffectSurface<>::Mask& effect,
    const downwell::BinaryEffectSurface<>::Mask& shade,int x,int y) {
    surface.clear();
    return surface.add(effect,x,y) && surface.subtract(shade,x-32,y-16);
}
#include "core/DebrisBirth.h"
#include "core/DebrisMotion.h"
#include "core/DebrisAnimation.h"
#include "core/DebrisEffects.h"
#include "core/DeadBodyCreation.h"
#include "core/CorpseBirth.h"
#include "core/CorpseLifecycle.h"
#include "core/CorpseMotion.h"
#include "core/CorpseAnimation.h"
#include "core/CorpseDraw.h"
#include "core/CorpseInstances.h"
#include "core/ConsumptionBloodEffect.h"
struct ConsumptionBloodCompilePublisher {
    bool gravity(downwell::MotionBinary64);downwell::MotionBinary64 gravity();downwell::MotionBinary64 fallSpeed();
    bool imageSpeed(downwell::RunnerImageValue);bool imageIndex(downwell::RunnerImageValue);
    bool position(downwell::RunnerImageValue,downwell::RunnerImageValue);bool destroy();
};
bool compileConsumptionBlood(downwell::ConsumptionBloodEffect& effect,downwell::GameMakerRandom& random,
    ConsumptionBloodCompilePublisher& publish){
    return effect.create({}, {},random,publish)&&effect.advanceImage(publish)&&effect.step(publish)&&effect.kill(publish);
}
bool compileNativeCorpse(downwell::CorpseInstances<32>& owner,downwell::GameMakerRandom& random,std::uint32_t id){
    downwell::InstanceHandle key;
    return owner.create(id,{}, {},false,random,[](auto,int,auto,auto,bool& hit){hit=false;return true;},key)&&
        owner.sprite(key,206)&&owner.facing(key,downwell::MotionBinary64::fromInteger(-1))&&
        owner.advanceAnimation(key)&&owner.dispatchPublished(key,[](auto& state,auto& publish,bool&){
            state.destroyRequested=true;return publish.destroy();
        })&&owner.retire(key);
}
bool compileNativeCorpseDeath(downwell::CorpseInstances<32>& owner,downwell::GameMakerRandom& random,
    std::uint32_t id,downwell::DeadBodySource& source){
    return owner.createFromDeath(id,source,206,downwell::MotionBinary64::fromInteger(4),102,false,random,
        [](auto,int,auto,auto,bool& hit){hit=false;return true;});
}
#include "core/SmokeDraw.h"
bool compileSmokeSurface(downwell::RunnerImageValue index,downwell::RunnerImageValue x,downwell::RunnerImageValue y,
    const downwell::EffectMaskAssets& assets,const downwell::EffectSurfacePlacement::View& view){
    return downwell::SmokeDraw::surface(index,x,y,assets,view,[](const auto& asset,auto px,auto py){
        return asset.mask.data&&px.valid()&&py.valid();
    });
}
bool compileCorpseDraw(const downwell::CorpseAnimation& corpse,
                       const downwell::EffectSurfacePlacement::View& view,
                       downwell::CorpseDraw::Command& out){
    return downwell::CorpseDraw::prepare(corpse,view,false,false,true,out);
}
bool compileCorpseSurface(const downwell::CorpseAnimation& corpse,const downwell::EffectMaskAssets& assets,
    const downwell::EffectSurfacePlacement::View& view){
    return downwell::CorpseDraw::surface(corpse,assets,view,[](const auto& asset,auto x,auto y){
        return asset.mask.data&&x.valid()&&y.valid();
    });
}
#include "core/CorpseEffects.h"
#include "core/StoredCorpseRegistration.h"
#include "core/SkeletonInstances.h"
#include "core/StoredCorpseExpiryRegistration.h"
#include "core/CorpseExpiryInstances.h"
bool compileNativeExpiry(downwell::CorpseExpiryInstances<4>& owner,std::uint32_t sourceId){
    downwell::InstanceHandle key;
    return owner.create(sourceId,{}, {},key)&&owner.sprite(key,102,12)&&
        owner.advanceAnimation(key)&&owner.activate(key,false)&&owner.clear();
}
bool compileRegisteredExpiry(downwell::StoredCorpseExpiryRegistration<16,4>& owner,
    downwell::CorpseAnimation& corpse,downwell::StoredCorpseRegistration<16,4>::Publisher& publish,
    const downwell::InstanceRecord& record){
    return owner.alarmEvent(corpse,0,publish,[](int){return 12u;})&&
        owner.advanceAnimation(record)&&owner.retire(record)&&owner.clear();
}
struct CorpseTileProbe {
    bool operator()(downwell::InstanceHandle,int,downwell::MotionBinary64,downwell::MotionBinary64,bool&);
};
bool compileSkeletonCorpse(downwell::StoredCorpseRegistration<16,4>& corpses,
    downwell::SkeletonInstances<4>::Publisher& publish,downwell::GameMakerRandom& random,CorpseTileProbe tile){
    return publish.deadBody([&](auto& source){return corpses.createFromDeath(source,206,{},102,false,random,tile);});
}
bool compileRegisteredCorpse(downwell::StoredCorpseRegistration<16,4>& owner,
    downwell::DeadBodySource& source,downwell::GameMakerRandom& random,CorpseTileProbe tile){
    if(!owner.createFromDeath(source,206,{},102,false,random,tile))return false;
    const auto key=owner.sourceHandle(source.myDeadBody);
    struct Environment {
        int area(){return 1;}
        int restInPieces(){return 0;}
        bool create(int,downwell::MotionBinary64,downwell::MotionBinary64){return false;}
    } environment;
    return owner.advanceAnimation(key)&&owner.dispatchPublished(key,[&](auto& state,auto& publish,bool& good){
        return state.stepInWorld(true,480,publish,environment,good);
    })&&
        owner.endStep(key,{},true)&&owner.clear();
}
namespace {
struct CorpseOwnerHooks {
    bool sprite(unsigned,const downwell::CorpseAnimation&){return true;}
    bool facing(unsigned,const downwell::CorpseAnimation&){return true;}
    bool birth(unsigned,const downwell::CorpseAnimation&){return true;}
    bool retire(unsigned,const downwell::CorpseAnimation&){return true;}
    CorpseOwnerHooks& queries(unsigned,downwell::CorpseAnimation&){return *this;}
    bool wall(downwell::MotionBinary64,downwell::MotionBinary64,bool& hit){hit=false;return true;}
    bool tile(downwell::MotionBinary64,downwell::MotionBinary64,bool& hit){hit=false;return true;}
    bool publish(downwell::RunnerImageValue,downwell::RunnerImageValue){return true;}
};
}
bool compileCorpseOwner(downwell::CorpseEffects<8>& owner,downwell::GameMakerRandom& random){
    CorpseOwnerHooks hooks;
    downwell::DeadBodySource source;source.imageXscale=downwell::MotionBinary64::fromInteger(1);
    return owner.createFromDeath(source,206,{},102,false,random,hooks)&&
        owner.dispatch(owner.beginPhase(),[](auto,auto& effect){return effect.advanceImage();},hooks)&&owner.clear(hooks);
}
bool compileCorpseAnimation(downwell::CorpseAnimation& corpse,int sprite){
    return corpse.setSprite(sprite,[](const auto&){return true;})&&corpse.advanceImage();
}
namespace {
struct CorpseMotionEvents {
    bool meeting(downwell::MotionBinary64,downwell::MotionBinary64,bool& hit){hit=true;return true;}
    int area(){return 3;}
    int restInPieces(){return 2;}
    bool publish(downwell::RunnerImageValue,downwell::RunnerImageValue){return true;}
    bool create(int,downwell::MotionBinary64,downwell::MotionBinary64){return true;}
    void destroy(){}
};
}
bool compileCorpseMotion(downwell::CorpseMotion& corpse,bool stopped){
    CorpseMotionEvents events;return corpse.step(stopped,events);
}
namespace {
struct CorpseBirthQueries {
    bool wall(downwell::MotionBinary64,downwell::MotionBinary64,bool& hit){hit=true;return true;}
    bool tile(downwell::MotionBinary64,downwell::MotionBinary64,bool& hit){hit=false;return true;}
    bool publish(downwell::RunnerImageValue,downwell::RunnerImageValue){return true;}
};
}
bool compileCorpseBirth(downwell::CorpseBirth& birth,downwell::GameMakerRandom& random){
    CorpseBirthQueries queries;
    return birth.create({}, {},false,random,queries);
}
namespace {
struct CorpseCompileEvents {
    unsigned trace=0;
    bool incrementBodiesEaten(){++trace;return true;}
    void eatSound(int a,int b,int c,int d){trace+=a+b+c+d;}
    downwell::MotionBinary64 playerY(){return downwell::MotionBinary64::fromInteger(trace);}
    downwell::MotionBinary64 playerX(){return downwell::MotionBinary64::fromInteger(trace);}
    void destroy(){++trace;}
    bool create(int object,downwell::MotionBinary64 x,downwell::MotionBinary64 y,std::uint32_t& id){trace+=object+x.low+y.low;id=42;return true;}
    bool sprite(unsigned id,int value){trace+=id+value;return true;}
    bool verticalSpeed(unsigned id,downwell::MotionBinary64 value){trace+=id+value.low;return true;}
    bool facing(unsigned id,downwell::MotionBinary64 value){trace+=id+value.high;return true;}
    bool explosion(unsigned id,int value){trace+=id+value;return true;}
    bool horizontalSpeed(unsigned id,downwell::MotionBinary64 value){trace+=id+value.low;return true;}
};
}
unsigned compileDeadBodyCreation(downwell::DeadBodySource& source,downwell::MotionBinary64 impact){
    CorpseCompileEvents events;
    return downwell::createDeadBody(source,206,impact,102,events)?events.trace:0xffffffffu;
}
bool compileCorpseLifecycle(downwell::CorpseLifecycle& corpse,unsigned alarm){
    CorpseCompileEvents events;
    return corpse.advanceAlarm(alarm,events)&&corpse.collidePlayer(true,false,events)&&corpse.endStep(true,{},events);
}
#include "core/DebrisDraw.h"
bool compileDebrisDraw(const downwell::DebrisAnimation& effect,
                       const downwell::EffectSurfacePlacement::View& view,
                       downwell::DebrisDraw::Command& command){
    return downwell::DebrisDraw::prepare(effect,view,false,false,true,command);
}
bool compileDebrisEffects(downwell::DebrisEffects<16>& owner,downwell::GameMakerRandom& random){
    const auto phase=owner.beginPhase();
    return owner.create({}, {},1,random)&&owner.advanceAnimation(phase)&&owner.advanceAlarm(phase,0)&&
        owner.step(phase,random,[](auto,const auto&,bool& stopped){stopped=false;return true;},
                   [](auto,const auto&,auto,auto,bool& hit){hit=false;return true;});
}
bool compileDebrisAnimation(downwell::DebrisAnimation& effect){return effect.advanceImage();}
bool compileDebrisMotion(downwell::DebrisMotion& effect,downwell::GameMakerRandom& random,bool stopped){
    effect.advanceAlarm(0);
    return effect.step(stopped,random,[](auto,auto,bool& hit){hit=false;return true;});
}
bool compileDebrisBirth(downwell::DebrisBirth& birth,int area,downwell::GameMakerRandom& random){
    return birth.create(area,random);
}
