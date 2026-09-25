#include "core/SmokeBirth.h"
#include "core/SmokeMotion.h"
#include "core/SmokeEffect.h"
#include "core/SmokeDirection.h"
#include "core/SmokeTrig.h"
#include "core/SmokeEffects.h"
#include "core/GameMakerRandom.h"
bool compileSmokeBirth(downwell::SmokeBirth& state,downwell::GameMakerRandom& random,
                      downwell::MotionBinary64 x,downwell::MotionBinary64 y) {
    return state.create(x,y,random) && state.sampleImageSpeed(random);
}
static bool smokeDirection(downwell::MotionBinary64 speed,downwell::MotionBinary64 angle,
                           downwell::MotionBinary64& x,downwell::MotionBinary64& y){
    return downwell::SmokeDirection::vector(speed,angle,x,y,downwell::smokeTrig);
}
bool compileSmokeMotion(downwell::SmokeMotion& smoke,downwell::GameMakerRandom& random) {
    return smoke.step(random,smokeDirection) && smoke.collideWall();
}
bool compileSmokeAnimation(downwell::SmokeEffect& smoke) {return smoke.advanceImage();}
bool compileSmokeOwner(downwell::SmokeEffects<16>& smoke,downwell::GameMakerRandom& random) {
    if(!smoke.skeletonBurst({}, {},random))return false;
    const auto phase=smoke.beginPhase();
    return smoke.advanceAnimation(phase) && smoke.step(phase,random);
}
bool compileSmokeDirection(downwell::MotionBinary64 speed,downwell::MotionBinary64 angle,
                          downwell::MotionBinary64& x,downwell::MotionBinary64& y) {
    return downwell::SmokeDirection::vector(speed,angle,x,y,downwell::smokeTrig);
}
