// DOWNWELL_REPLAY_VERIFY code of DownwellAmiga.cpp, moved verbatim (WR.2). Each part
// is included at its original place inside that verifier's
// conditional; DOWNWELL_VERIFY_PART selects it. No include guard.
#if DOWNWELL_VERIFY_PART == 5
#include "AmigaMemoryCopyVerification.h"
#include "AmigaCoverageWordVerification.h"
#include <cstring>
#include "core/GemVerification.h"
#include "core/MotionBinary64Verification.h"
#include "core/SmallUnsignedRemainderVerification.h"
#include "core/Positive63RemainderVerification.h"
#include "core/Well512Verification.h"
#include "core/PlayerDeathVerification.h"
#include "core/MachineGunVerification.h"
#include "core/RunnerSpatialVerification.h"
#include "core/CorpseVerification.h"
#include "core/DamageNumberVerification.h"
#include "core/PlayerPreciseVerification.h"
#include "core/ReplayFrameTrace.h"
extern "C" {
#ifdef DOWNWELL_REPLAY_FRAME_TRACE
volatile std::uint32_t gReplayTrace[downwell::ReplayFrameTrace::capacity][downwell::ReplayFrameTrace::words];
#endif
volatile ULONG gReplayComplete;
volatile unsigned gMemoryCopyVerifiedCases;
volatile unsigned gCoverageWordVerifiedCases;
volatile ULONG gReplayFrames;
volatile ULONG gReplayDigestHigh;
volatile ULONG gReplayDigestLow;
volatile ULONG gGemReplayDigestHigh;
volatile ULONG gGemReplayDigestLow;
volatile ULONG gGemReplayCoverage;
volatile ULONG gDeathReplayDigestHigh;
volatile ULONG gDeathReplayDigestLow;
volatile ULONG gDeathReplayCoverage;
volatile ULONG gWeaponReplayDigestHigh;
volatile ULONG gWeaponReplayDigestLow;
volatile ULONG gWeaponReplayCoverage;
volatile ULONG gMotionReplayDigest, gMotionReplayUpdates, gMotionReplayFailures;
volatile ULONG gSmallRemainderFailures;
volatile ULONG gPositive63RemainderFailures;
volatile ULONG gWell512Failures;
volatile ULONG gMotionBenchDigest, gMotionBenchFailures, gMotionBenchFields;
volatile ULONG gBoneReplayDigest, gBoneReplayUpdates, gBoneReplayFailures;
volatile ULONG gBoneBenchDigest, gBoneBenchFailures, gBoneBenchFields;
volatile ULONG gBoneCreateDigest, gBoneCreateCount, gBoneCreateFailures;
volatile ULONG gBoneAnimationDigest, gBoneAnimationCount, gBoneAnimationFailures;
volatile ULONG gSpatialChecks, gSpatialDigest, gSpatialFailures;
volatile ULONG gCorpseChecks, gCorpseDigest, gCorpseFailures;
volatile ULONG gDamageNumberChecks, gDamageNumberDigest, gDamageNumberFailures;
volatile ULONG gPlayerPreciseChecks, gPlayerPreciseDigest, gPlayerPreciseFailures;
volatile ULONG gReplayStackFailureStage;

// Identify the first workload that consumes the guard without resetting it.
// Stage 1 includes startup/gameplay; subsequent stages follow the checks below.
__attribute__((noinline)) void downwellReplayCheckStack(ULONG stage) {
    if (gReplayStackFailureStage || !gDownwellStackGuardReady) return;
    const volatile ULONG* guard = (const volatile ULONG*)SysBase->ThisTask->tc_SPLower;
    for (unsigned word = 0; word < 64; ++word) {
        if (guard[word] != 0x57ac6a4d) {
            gReplayStackFailureStage = stage;
            return;
        }
    }
}

__attribute__((noinline, used)) void downwellReplayComplete(void) {
    __asm__ volatile("" ::: "memory");
}
}
#endif
#if DOWNWELL_VERIFY_PART == 17
    if (!amigaRendererBuildHardwareSprite(
            activeGamePack, activeGamePackEnd, 31, 0,
            downwellPlayfieldX + 80,
#ifdef DOWNWELL_REPLAY_VERIFY
            gameplayPlayerGroundY,
#else
            gameplayPlayerStartY - port.cameraOffsetY(),
#endif
            0,
            memory.playerSprite(0), playerSpriteHeight) ||
        !amigaRendererBuildHardwareSprite(
            activeGamePack, activeGamePackEnd, 37, 0,
            downwellPlayfieldX + 80,
#ifdef DOWNWELL_REPLAY_VERIFY
            gameplayPlayerGroundY,
#else
            gameplayPlayerStartY - port.cameraOffsetY(),
#endif
            0,
            memory.playerSprite(1), playerSpriteHeight)) {
        platform.restore();
        FreeMem(startupCopperWords, 4 * sizeof(UWORD));
        memory.release();
        gDownwellStartupFailure = 11;
        downwellStartupFailed();
        return 20; // AmigaDOS RETURN_FAIL; diagnostic reason remains available.
    }
#endif
#if DOWNWELL_VERIFY_PART == 36
#ifdef DOWNWELL_REPLAY_FRAME_TRACE
        if(port.tick()&&port.tick()<=downwell::ReplayFrameTrace::capacity)
            downwell::ReplayFrameTrace::capture(port,gReplayTrace[port.tick()-1]);
#endif
        if (port.tick() == verificationReplay.frameCount()) {
            gReplayFrames = port.tick();
            gReplayDigestHigh = (ULONG)(lastPortCheckpoint >> 32);
            gReplayDigestLow = (ULONG)lastPortCheckpoint;
            gSmallRemainderFailures = downwell::verifySmallUnsignedRemainder();
            gPositive63RemainderFailures = downwell::verifyPositive63Remainder();
            gWell512Failures = downwell::verifyWell512();
            downwellReplayCheckStack(1);
            const auto gems = downwell::verifyGemSimulation();
            downwellReplayCheckStack(2);
            gGemReplayDigestHigh = (ULONG)(gems.digest >> 32);
            gGemReplayDigestLow = (ULONG)gems.digest;
            gGemReplayCoverage = gems.coverage;
            const auto death = downwell::verifyPlayerDeath();
            downwellReplayCheckStack(3);
            gDeathReplayDigestHigh = (ULONG)(death.digest >> 32);
            gDeathReplayDigestLow = (ULONG)death.digest;
            gDeathReplayCoverage = death.coverage;
            const auto weapon = downwell::verifyMachineGun();
            downwellReplayCheckStack(4);
            gWeaponReplayDigestHigh = (ULONG)(weapon.digest >> 32);
            gWeaponReplayDigestLow = (ULONG)weapon.digest;
            gWeaponReplayCoverage = weapon.coverage;
            const auto motion = downwell::verifyMotionBinary64();
            downwellReplayCheckStack(5);
            gMotionReplayDigest = motion.digest; gMotionReplayUpdates = motion.updates;
            gMotionReplayFailures = motion.failures;
            const UWORD motionStart = AmigaPlatform::vblankCount();
            const auto motionBench = downwell::benchmarkMotionBinary64();
            downwellReplayCheckStack(6);
            gMotionBenchFields = UWORD(AmigaPlatform::vblankCount() - motionStart);
            gMotionBenchDigest = motionBench.digest; gMotionBenchFailures = motionBench.failures;
            const auto bones = downwell::verifyBoneMotion();
            downwellReplayCheckStack(7);
            gBoneReplayDigest = bones.digest; gBoneReplayUpdates = bones.updates; gBoneReplayFailures = bones.failures;
            const UWORD boneStart = AmigaPlatform::vblankCount();
            const auto boneBench = downwell::verifyBoneMotion(true);
            downwellReplayCheckStack(8);
            gBoneBenchFields = UWORD(AmigaPlatform::vblankCount() - boneStart);
            gBoneBenchDigest = boneBench.digest; gBoneBenchFailures = boneBench.failures;
            const auto createdBones = downwell::verifyBoneCreate();
            downwellReplayCheckStack(9);
            gBoneCreateDigest = createdBones.digest; gBoneCreateCount = createdBones.updates;
            gBoneCreateFailures = createdBones.failures;
            const auto animatedBones=downwell::verifyBoneAnimation();
            downwellReplayCheckStack(10);
            gBoneAnimationDigest=animatedBones.digest; gBoneAnimationCount=animatedBones.updates;
            gBoneAnimationFailures=animatedBones.failures;
            const auto spatial=downwell::verifyRunnerSpatialMutations();
            downwellReplayCheckStack(11);
            gSpatialChecks=spatial.checks;gSpatialDigest=spatial.digest;gSpatialFailures=spatial.failures;
            const auto corpses=downwell::verifyCorpses();
            downwellReplayCheckStack(12);
            gCorpseChecks=corpses.checks;gCorpseDigest=corpses.digest;gCorpseFailures=corpses.failures;
            const auto numbers=downwell::verifyDamageNumbers();
            downwellReplayCheckStack(13);
            gDamageNumberChecks=numbers.checks;gDamageNumberDigest=numbers.digest;gDamageNumberFailures=numbers.failures;
            const auto precise=downwell::verifyPlayerPrecise();
            downwellReplayCheckStack(14);
            gPlayerPreciseChecks=precise.checks;gPlayerPreciseDigest=precise.digest;gPlayerPreciseFailures=precise.failures;
            gReplayComplete = 1;
            downwellReplayComplete();
            for (;;) platform.waitVblank();
        }
#endif
