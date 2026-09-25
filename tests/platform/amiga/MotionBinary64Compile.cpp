#include "core/MotionBinary64.h"
#include "core/BoneProjectile.h"
#include "core/RunnerImageValue.h"
#include "core/LegacyImageAnimation.h"
#include "core/SequenceTimeline.h"
#include "core/UnitSequenceFrames.h"
#include "core/SkeletonAnimation.h"
#include "core/SkeletonDecisions.h"
#include "core/SkeletonActor.h"
#include "core/BoneProjectileStore.h"
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
extern "C" unsigned boneContactCompile(downwell::BoneProjectile* bone,
    downwell::ContactActor player,bool stopped,bool wall) {
    BoneEvents events;
    // Compile each adapter; this fixture does not assert collision-family order.
    if(wall) bone->dispatchWall([](downwell::ContactBounds b){return b.left<0;},events);
    else bone->dispatchPlayer(player,stopped,events);
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
namespace {
struct SkeletonDecisionEvents {
    unsigned trace = 0;
    void hurtSound(int sound) { trace = (trace << 8) ^ unsigned(sound); }
    bool throwBone(int facing) { trace = (trace << 2) ^ unsigned(facing); return true; }
    void enemyDeath() { ++trace; }
    void blood(int a,int b) { trace+=a+b; }
    void smoke(downwell::MotionBinary64 x,downwell::MotionBinary64 y,int a,int b) { trace^=x.low+y.low+a+b; }
    void flash(downwell::MotionBinary64 x,downwell::MotionBinary64 y,int a,int b,int c) { trace^=x.high+y.high+a+b+c; }
    void currency(int a) { trace+=a; }
    void deathSound(int a) { trace+=a; }
    void deadBody(int a,downwell::MotionBinary64 b,int c) { trace+=a+b.high+b.low+c; }
    void debris(int a,int b) { trace+=a+b; }
    void destroy() { ++trace; }
    void destroyBullet() { ++trace; }
    downwell::MotionBinary64 impactAngle(int x,int y) { trace+=x+y; return {}; }
    void momentDelay() { ++trace; }
    void bulletHitSound(int sound) { trace+=sound; }
    void impactEffect(int sprite,int rate,downwell::MotionBinary64 direction,int depth) {
        trace+=sprite+rate+direction.high+direction.low+depth;
    }
    bool playerDead(){return false;}
    int playStyle(){return 0;}
    void forcedJump(int x,int y){trace+=x+y;}
    void shake(int a,int b){trace+=a+b;}
    void stompSound(int sound){trace+=sound;}
    void recharge(){++trace;}
    bool explodingKnees(){return true;}
    bool createBlast(int object,int x,int y){trace+=object+x+y;return true;}
    unsigned choose(unsigned count){return trace%count;}
    void explosionSound(int sound){trace+=sound;}
    downwell::MotionBinary64 playerX(){return {};}
    downwell::MotionBinary64 playerY(){return {};}
    void emitSmoke(downwell::MotionBinary64 x,downwell::MotionBinary64 y,int angle,int speed){trace+=x.low+y.low+angle+speed;}
};
}
extern "C" unsigned skeletonDecisionsCompile(downwell::SkeletonDecisions* state,
    bool stopped, bool below, bool left, int movement) {
    SkeletonDecisionEvents ops;
    if (!state->stepDecisions(stopped,below,left,movement,ops) || !state->finishPresentation())
        return 0xffffffffu;
    return ops.trace;
}
extern "C" unsigned skeletonActorCompile(downwell::SkeletonActor* state,
    bool stopped, bool below, bool left, int movement) {
    SkeletonDecisionEvents ops;
    if (!state->step(stopped,below,left,movement,ops)) return 0xffffffffu;
    return ops.trace;
}
extern "C" unsigned skeletonCoordinatesCompile(downwell::SkeletonActor* state,
    bool stopped,bool mainRoom,downwell::MotionBinary64 px,downwell::MotionBinary64 py,
    downwell::MotionBinary64 viewY) {
    SkeletonDecisionEvents ops;
    if(!state->stepAtPlayer(stopped,px,py,ops) || !state->endStep(mainRoom,viewY,ops))
        return 0xffffffffu;
    return ops.trace;
}
extern "C" bool skeletonCreateCompile(downwell::SkeletonActor* state,int x,int y) {
    return state->create(x,y);
}
extern "C" unsigned skeletonBulletCompile(downwell::SkeletonActor* state,int damage,
    downwell::MotionBinary64 direction) {
    SkeletonDecisionEvents events;
    if(!state->hitByMachineGun(damage,direction,events)) return 0xffffffffu;
    return events.trace;
}
extern "C" unsigned skeletonStompCompile(downwell::SkeletonActor* state) {
    SkeletonDecisionEvents events;
    if(!state->stompPlayer(events)) return 0xffffffffu;
    return events.trace;
}
namespace {
struct SkeletonPlayerCompile {
    downwell::MotionBinary64 y,speed;
    bool onGround;
    unsigned trace=0;
    downwell::MotionBinary64 globalY(){return y;}
    downwell::MotionBinary64 verticalSpeed(){return speed;}
    bool grounded(){return onGround;}
    void stomp(){++trace;speed={};}
    void damage(int a,int b,int c){trace+=a+b+c;}
};
}
extern "C" unsigned skeletonContactCompile(downwell::SkeletonActor* state,
    downwell::ContactActor player,downwell::MotionBinary64 y,
    downwell::MotionBinary64 speed,bool grounded) {
    SkeletonPlayerCompile contact{y,speed,grounded};
    if(!state->meetsPlayer(player)) return 0;
    if(!state->collidePlayerAt(contact)) return 0xffffffffu;
    return contact.trace;
}
namespace {
struct BoneStoreCompileRandom {
    unsigned word;
    unsigned nextU32() { return word++; }
    unsigned chooseIndexSmall(unsigned count) { return nextU32()%count; }
};
struct BoneStoreCompileSound { unsigned value=0; void operator()(int n) { value=n; } };
}
extern "C" unsigned boneStoreCompile(downwell::BoneProjectileStore<8>* store,
    unsigned randomWord,bool separated,bool* shot) {
    BoneStoreCompileRandom random{randomWord}; BoneStoreCompileSound sound;
    downwell::SkeletonBoneCreation<downwell::BoneProjectileStore<8>,BoneStoreCompileRandom,BoneStoreCompileSound>
        creation{*store,random,sound,*shot};
    if (!creation.run(100,200,separated,1)) return 0;
    return creation.child;
}
extern "C" unsigned bonePhaseCompile(downwell::BoneProjectileStore<8>* store,
    downwell::BoneProjectileStore<8>::Phase phase) {
    unsigned count=0;
    store->visitPhase(phase,[&](std::uint32_t,downwell::BoneProjectile&){++count;});
    return count;
}
