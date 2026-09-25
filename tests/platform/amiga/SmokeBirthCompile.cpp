#include "core/SmokeBirth.h"
#include "core/GameMakerRandom.h"
bool compileSmokeBirth(downwell::SmokeBirth& state,downwell::GameMakerRandom& random,
                      downwell::MotionBinary64 x,downwell::MotionBinary64 y) {
    return state.create(x,y,random) && state.sampleImageSpeed(random);
}
