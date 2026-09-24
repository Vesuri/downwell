#include "core/PortReplay.h"
#include "core/Replay.h"
#include "core/GemVerification.h"
#include "core/PlayerDeathVerification.h"
#include "core/MachineGunVerification.h"
#include "platform/amiga/AmigaPortRuntime.h"

#include <cinttypes>
#include <cstdio>

int main() {
    const downwell::ReplayReader replay(downwell::portReplayBytes,
                                        downwell::portReplaySize);
    if (!replay.valid())
        return 1;

    AmigaPortRuntime runtime(300);
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
    return 0;
}
