#include "core/BinaryEffectSurface.h"
#include "core/InstanceStore.h"
bool compileInstanceHandles(downwell::InstanceStore<4>& store,unsigned id){
    if(store.create(id,82,1,false)!=downwell::CreateInstanceResult::success)return false;
    auto handle=store.handleAt(store.size()-1);store.enterRoom(2);
    return store.resolve(handle)==nullptr;
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
bool compileOrdinaryTimeStopContact(downwell::RunnerImageValue x,downwell::RunnerImageValue scale,const downwell::OrdinaryTimeField& field,bool& hit){
    return downwell::TimeStopGoalContact::query(downwell::TimeStopGoalContact::Mask::Shard,x,x,scale,scale,field,hit);
}
bool compileTimeStopGoalContact(downwell::RunnerImageValue x,downwell::RunnerImageValue scale,const downwell::GoalTimeField& field,bool& hit){
    return downwell::TimeStopGoalContact::query(downwell::TimeStopGoalContact::Mask::Player,x,x,scale,scale,field,hit);
}
#include "core/RunSession.h"
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
#include "core/RunnerObjectMembership.h"
#include "core/RunnerObjectFamilies.h"
bool compileObjectFamilies(downwell::RunnerObjectFamilies<386>& families,
    const downwell::ObjectPack& objects,unsigned root,std::uint32_t initialFlags){
    return families.bind(objects,[&](const auto&){return initialFlags;}) &&
        families.markSpatialFamily(root) && families.prepareBirth(root);
}
bool compileInitialSpatialFlags(downwell::RunnerObjectFamilies<386>& families,
    const downwell::ObjectPack& objects){return families.bindSpatialFlags(objects);}
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
    return assets.bind(pack,size);
}
bool compileBinaryEffect(downwell::BinaryEffectSurface<>& surface,
    const downwell::BinaryEffectSurface<>::Mask& effect,
    const downwell::BinaryEffectSurface<>::Mask& shade,int x,int y) {
    surface.clear();
    return surface.add(effect,x,y) && surface.subtract(shade,x-32,y-16);
}
