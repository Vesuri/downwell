#include "core/MotionBinary64.h"
#include "core/BoneProjectile.h"
#include "core/RunnerImageValue.h"
#include "core/LegacyImageAnimation.h"
#include "core/SequenceTimeline.h"
#include "core/UnitSequenceFrames.h"
#include "core/SkeletonAnimation.h"
// Code-generation fixture only, not linked into the playable executable.
// Variable operands force the compiler to retain the complete arithmetic path.
extern "C" bool motionBinary64Compile(downwell::MotionBinary64* position,
    downwell::MotionBinary64* velocity, std::int32_t* pixel) {
    return velocity->add({0x3fb99999u, 0x9999999au}) &&
        position->add(*velocity) && position->roundPixel(*pixel);
}
namespace {
struct BoneEvents {
    unsigned trace = 0;
    void destroy() { trace = (trace << 2) | 1; }
    void effect(int, int) { trace = (trace << 2) | 2; }
    void damage(int, int, int) { trace = (trace << 2) | 3; }
};
}
extern "C" unsigned boneProjectileCompile(downwell::BoneProjectile* bone,
    bool stopped, bool playerCollision, bool wallCollision) {
    BoneEvents events;
    if (!bone->step(stopped, [](int x,int y) { return x > 0 && x < 160 && y > 0 && y < 284; }, events))
        return 0xffffffffu;
    if (playerCollision) bone->collidePlayer(stopped, events);
    if (wallCollision) bone->collideWall(events);
    return events.trace;
}
extern "C" bool randomBinary64Compile(unsigned word, downwell::MotionBinary64 scale,
    downwell::MotionBinary64* result) {
    return downwell::randomBinary64(word,scale,*result);
}
extern "C" bool runnerImageValueCompile(downwell::MotionBinary64 input,
    downwell::MotionBinary64* output) {
    downwell::RunnerImageValue stored;
    if (!stored.assign(input)) return false;
    *output = stored.promoted();
    return true;
}
extern "C" bool legacyImageAnimationCompile(downwell::RunnerImageValue* index,
    downwell::RunnerImageValue speed, unsigned frames, bool* ended) {
    return downwell::advanceLegacyImage(*index, speed, frames, *ended);
}
extern "C" bool sequenceTimelineCompile(downwell::RunnerImageValue* position,
    downwell::RunnerImageValue speed, unsigned length, bool* crossed) {
    return downwell::SequenceTimeline::advance(*position, speed, length, *crossed);
}
extern "C" bool unitSequenceFramesCompile(downwell::RunnerImageValue* position,
    downwell::RunnerImageValue speed, unsigned frames, downwell::RunnerImageValue* image,
    bool* crossed, bool* reconcile) {
    return downwell::UnitSequenceFrames::needsReconcile(*image,*position,*reconcile) &&
        downwell::UnitSequenceFrames::advance(*position,speed,frames,*image,*crossed);
}
extern "C" bool skeletonAnimationCompile(downwell::SkeletonAnimation* state,
    int sprite, bool stopped, bool* crossed) {
    if (!state->setSprite(sprite) || !state->advanceImage(*crossed)) return false;
    state->beginStep(stopped);
    return true;
}
