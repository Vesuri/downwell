#include "core/PortReplay.h"
#include "core/Replay.h"
#include "core/GemVerification.h"
#include "core/MotionBinary64Verification.h"
#include "core/PlayerDeathVerification.h"
#include "core/MachineGunVerification.h"
#include "core/GameplayWorld.h"
#include "core/RunnerSpatialVerification.h"

#include <cinttypes>
#include <cstdio>

int main() {
    const downwell::ReplayReader replay(downwell::portReplayBytes,
                                        downwell::portReplaySize);
    if (!replay.valid())
        return 1;

    GameplayWorld runtime(300);
    for (std::uint32_t frame = 0; frame < replay.frameCount(); ++frame) {
        downwell::Input input;
        if (!replay.frame(frame, input))
            return 2;
        runtime.step(downwell::packReplayInput(input));
    }

    const std::uint64_t digest = runtime.trajectoryDigest();
    const auto gems = downwell::verifyGemSimulation();
    if (gems.coverage != downwell::gemVerificationCoverage) return 3;
    const auto death = downwell::verifyPlayerDeath();
    if (death.coverage != downwell::playerDeathVerificationCoverage) return 4;
    const auto weapon = downwell::verifyMachineGun();
    if (weapon.coverage != downwell::machineGunVerificationCoverage) return 5;
    std::printf("%" PRIu32 " %08" PRIx32 " %08" PRIx32 " 400 %08" PRIx32 " %08" PRIx32 " %02x 480 %08" PRIx32 " %08" PRIx32 " %03x 64 %08" PRIx32 " %08" PRIx32 " %02x\n",
                replay.frameCount(), (std::uint32_t)(digest >> 32),
                (std::uint32_t)digest, (std::uint32_t)(gems.digest >> 32),
                (std::uint32_t)gems.digest, gems.coverage,
                (std::uint32_t)(death.digest >> 32), (std::uint32_t)death.digest, death.coverage,
                (std::uint32_t)(weapon.digest >> 32), (std::uint32_t)weapon.digest, weapon.coverage);
    const auto motion = downwell::verifyMotionBinary64();
    const auto bench = downwell::benchmarkMotionBinary64();
    if (motion.failures || bench.failures) return 6;
    std::printf("MOTION %u %08x %u %08x %u\n", motion.updates, motion.digest, motion.failures, bench.digest, bench.failures);
    const auto bones = downwell::verifyBoneMotion(), boneBench = downwell::verifyBoneMotion(true);
    if (bones.failures || boneBench.failures) return 7;
    std::printf("BONE %u %08x %u %08x %u\n", bones.updates, bones.digest, bones.failures, boneBench.digest, boneBench.failures);
    const auto createdBones = downwell::verifyBoneCreate();
    if (createdBones.failures) return 8;
    std::printf("BONE_CREATE %u %08x %u\n",createdBones.updates,createdBones.digest,createdBones.failures);
    const auto animatedBones=downwell::verifyBoneAnimation();
    if(animatedBones.failures) return 9;
    std::printf("BONE_ANIMATION %u %08x %u\n",animatedBones.updates,animatedBones.digest,animatedBones.failures);
    const auto spatial=downwell::verifyRunnerSpatialMutations();
    if(spatial.failures) return 10;
    std::printf("SPATIAL %u %08x %u\n",spatial.checks,spatial.digest,spatial.failures);
    return 0;
}
