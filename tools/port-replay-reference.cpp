#include "core/PortReplay.h"
#include "core/Replay.h"
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
    std::printf("%" PRIu32 " %08" PRIx32 " %08" PRIx32 "\n",
                replay.frameCount(), (std::uint32_t)(digest >> 32),
                (std::uint32_t)digest);
    return 0;
}
