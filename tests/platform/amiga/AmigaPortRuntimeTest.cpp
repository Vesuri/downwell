#include "platform/amiga/AmigaPortRuntime.h"
#include "core/PortReplay.h"
#include "core/Replay.h"

#include <cstdlib>
#include <iostream>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

} // namespace

int main() {
    AmigaPortRuntime runtime(300);
    const AmigaPortFrame jump = runtime.step(4);
    require(!jump.scrolled && !runtime.player().grounded() &&
                runtime.player().jumps() == 1,
            "first replay frame jumps without scrolling");
    const AmigaPortFrame held = runtime.step(4);
    require(held.scrolled && runtime.ringOrigin() == 301 &&
                runtime.player().shots() == 0,
            "second frame advances the ring without repeat firing");
    runtime.step(0);
    const AmigaPortFrame shot = runtime.step(4);
    require(shot.bulletChanged && shot.ammoChanged &&
                runtime.bulletActive() && runtime.player().ammo() == 7,
            "released airborne press creates the bounded bullet state");
    require(runtime.checkpoint() != 0 && runtime.trajectoryDigest() != 0,
            "port runtime exposes final and rolling checkpoints");

    AmigaPortRuntime fallingRuntime(300, false, 1201 - 176, true);
    int accumulatedScroll = 0;
    for (unsigned frame = 0;
         frame < 300 && !fallingRuntime.player().grounded(); ++frame) {
        const AmigaPortFrame fallingFrame = fallingRuntime.step(0);
        accumulatedScroll += fallingFrame.scrollDelta;
    }
    require(fallingRuntime.player().grounded() &&
                fallingRuntime.player().offsetY() == 1201 - 176,
            "camera-follow gameplay falls to the recovered entrance height");
    require(accumulatedScroll == fallingRuntime.cameraOffsetY() &&
                fallingRuntime.cameraOffsetY() > 900,
            "camera motion exactly accounts for circular-buffer scrolling");
    require(fallingRuntime.ringOrigin() ==
                static_cast<unsigned>((300 + accumulatedScroll) % 320),
            "ring origin follows the signed accumulated camera displacement");

    const downwell::ReplayReader replay(downwell::portReplayBytes,
                                        downwell::portReplaySize);
    require(replay.valid() && replay.frameCount() == 64,
            "cross-target replay tape validates");
    AmigaPortRuntime replayRuntime(300);
    for (std::uint32_t frame = 0; frame < replay.frameCount(); ++frame) {
        downwell::Input replayInput;
        require(replay.frame(frame, replayInput),
                "cross-target replay frame is available");
        replayRuntime.step(downwell::packReplayInput(replayInput));
    }
    require(replayRuntime.trajectoryDigest() == 0xa5bc906f1a9c5f89ULL,
            "cross-target trajectory digest remains pinned");
    std::cout << "Amiga port runtime tests passed\n";
    return 0;
}
