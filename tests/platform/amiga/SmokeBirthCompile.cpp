#include "core/SmokeBirth.h"
#include "core/SmokeMotion.h"
#include "core/SmokeEffect.h"
#include "core/SmokeDirection.h"
#include "core/GameMakerRandom.h"
bool compileSmokeBirth(downwell::SmokeBirth& state,downwell::GameMakerRandom& random,
                      downwell::MotionBinary64 x,downwell::MotionBinary64 y) {
    return state.create(x,y,random) && state.sampleImageSpeed(random);
}
extern bool smokeDirection(downwell::MotionBinary64,downwell::MotionBinary64,
                           downwell::MotionBinary64&,downwell::MotionBinary64&);
bool compileSmokeMotion(downwell::SmokeMotion& smoke,downwell::GameMakerRandom& random) {
    return smoke.step(random,smokeDirection) && smoke.collideWall();
}
bool compileSmokeAnimation(downwell::SmokeEffect& smoke) {return smoke.advanceImage();}
extern bool smokeTrig(downwell::MotionBinary64,downwell::MotionBinary64&,downwell::MotionBinary64&);
bool compileSmokeDirection(downwell::MotionBinary64 speed,downwell::MotionBinary64 angle,
                          downwell::MotionBinary64& x,downwell::MotionBinary64& y) {
    return downwell::SmokeDirection::vector(speed,angle,x,y,smokeTrig);
}
