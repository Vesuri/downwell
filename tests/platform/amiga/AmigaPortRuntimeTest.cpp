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

bool noSolid(void*, int, int) {
    return false;
}

bool flatGround(void*, int, int y) {
    return y >= 9;
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

    AmigaPortRuntime burst(300, false, 1000);
    burst.step(4);
    for (unsigned frame = 0; frame < 7; ++frame)
        burst.step(4);
    require(burst.player().shots() == 2 && burst.bulletCount() == 2 &&
                burst.bulletAt(0).active && burst.bulletAt(1).active &&
                burst.bulletAt(0).y > burst.bulletAt(1).y,
            "held gunboots retain simultaneous seven-tick shots");

    AmigaPortRuntime destructible(300);
    const int blockX[] = {-1};
    const int blockY[] = {32};
    destructible.configureBreakables(blockX, blockY, 1);
    destructible.step(4);
    destructible.step(0);
    destructible.step(4);
    AmigaPortFrame broken{};
    for (unsigned frame = 0;
         frame < 16 && destructible.blocksDestroyed() == 0; ++frame)
        broken = destructible.step(0);
    require(destructible.blocksDestroyed() == 1 &&
                broken.destroyedBlockCount == 1 &&
                !destructible.breakableActiveAt(-1, 32) &&
                !destructible.bulletActive(),
            "gunboot bullets destroy configured recovered blocks");

    AmigaPortRuntime combat(300);
    require(combat.configureEnemy(0, 111, 163, -1, 12,
                                  0, 0, 5, 6),
            "bounded native enemy configuration succeeds");
    combat.step(4);
    combat.step(0);
    combat.step(4);
    for (unsigned frame = 0;
         frame < 20 && combat.enemiesDefeated() == 0; ++frame)
        combat.step(0);
    require(combat.enemiesDefeated() == 1 &&
                !combat.enemyAt(0).active,
            "usual gun bullet defeats the recovered five-health snake");

    AmigaPortRuntime turtle(300);
    turtle.configureSolidResolver(flatGround, 0);
    require(turtle.configureEnemy(0, 112, 169, 40, 0,
                                  64, 0, 5, 14),
            "generated ground turtle can enter native actor storage");
    for (unsigned frame = 0; frame < 4; ++frame)
        turtle.step(0);
    require(turtle.enemyAt(0).preciseX > 40L * 256L &&
                turtle.enemyAt(0).active,
            "ground turtle uses its recovered quarter-pixel walking speed");

    AmigaPortRuntime stomp(300);
    require(stomp.configureEnemy(0, 111, 163, 0, 5,
                                 0, 0, 5, 6),
            "stomp enemy configuration succeeds");
    stomp.step(0);
    require(stomp.enemiesDefeated() == 1 &&
                !stomp.enemyAt(0).active &&
                !stomp.player().grounded() &&
                stomp.player().ammo() == 8 && stomp.gemCount() == 3,
            "descending contact stomps a snake, bounces and refills gunboots");
    require(stomp.gemAt(0).amount == 2 && !stomp.gemAt(0).large &&
                stomp.gemAt(1).amount == 2 &&
                stomp.gemAt(2).amount == 2,
            "six-value snake reward splits into three recovered small gems");
    for (unsigned frame = 0;
         frame < 120 && stomp.currency() == 0; ++frame)
        stomp.step(0);
    require(stomp.currency() == 6 && stomp.gemCount() == 0,
            "attracted stomp reward reaches the player and enters currency");
    require(stomp.gemStreak() == 6 && stomp.gemStreakTimer() == 300,
            "collected physical gems feed the recovered streak timer");
    AmigaPortFrame expiredStreak{};
    for (unsigned frame = 0;
         frame < 301 && stomp.gemStreak() != 0; ++frame)
        expiredStreak = stomp.step(0);
    require(stomp.gemStreak() == 0 && stomp.gemStreakTimer() == 0 &&
                expiredStreak.streakChanged,
            "native streak expires after the recovered 300-frame hold");

    AmigaPortRuntime contact(300);
    require(contact.configureEnemy(0, 117, 177, 0, 0,
                                   0, 0, 20, 24),
            "contact enemy configuration succeeds");
    AmigaPortFrame contactFrame = contact.step(0);
    require(contact.health() == 3 && contactFrame.healthChanged &&
                contact.invulnerability() == 90,
            "side contact removes one health and starts invulnerability");
    contactFrame = contact.step(0);
    require(contact.health() == 3 && !contactFrame.healthChanged &&
                contact.invulnerability() == 89,
            "invulnerability suppresses repeated overlap damage");

    AmigaPortRuntime sideCamera(300, true, 0, true);
    sideCamera.configureSolidResolver(noSolid, 0);
    downwell::Input walkRight;
    walkRight.right = true;
    AmigaPortFrame sideFrame{};
    for (unsigned frame = 0;
         frame < 120 && sideCamera.cameraOffsetX() == 0; ++frame)
        sideFrame = sideCamera.step(downwell::packReplayInput(walkRight));
    require(sideCamera.player().offsetX() > 80 &&
                sideCamera.cameraOffsetX() == 160 &&
                sideFrame.cameraXChanged && sideFrame.cameraXDelta == 160,
            "crossing the recovered shaft edge snaps to the right-room camera");

    AmigaPortRuntime fallingRuntime(300, false, 1201 - 176, true);
    int accumulatedScroll = fallingRuntime.cameraOffsetY();
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
    require(replayRuntime.trajectoryDigest() == 0x95d0aaea1be7256fULL,
            "cross-target trajectory digest remains pinned");
    std::cout << "Amiga port runtime tests passed\n";
    return 0;
}
