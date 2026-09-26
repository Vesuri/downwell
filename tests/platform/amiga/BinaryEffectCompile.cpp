#include "core/BinaryEffectSurface.h"
#include "core/BloodEffect.h"
#include "core/BloodEffects.h"
#include "core/BloodCollisionBounds.h"
#include "core/BloodMaskSample.h"
#include "core/BloodGoalContact.h"
#include "core/RunSession.h"
#include "core/PlayerTimeStop.h"
#include "core/TimeChangeEffect.h"
#include "core/TimeShardBirth.h"
#include "core/TimeShardLifecycle.h"
#include "core/TimeShardMotion.h"
#include "core/TimeShardAnimation.h"
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
