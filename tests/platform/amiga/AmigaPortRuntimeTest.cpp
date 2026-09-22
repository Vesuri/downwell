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

bool shortRightWall(void*, int x, int y) {
    return x >= 8 && y < 16;
}

bool crawlerCorner(void*, int x, int y) {
    return y >= 7 || x >= 2;
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
                                  64, 0, 5, 14,
                                  0, 1, -1, 1),
            "generated ground turtle can enter native actor storage");
    for (unsigned frame = 0; frame < 4; ++frame)
        turtle.step(0);
    require(turtle.enemyAt(0).preciseX > 40L * 256L &&
                turtle.enemyAt(0).active &&
                turtle.enemyAt(0).auxiliaryState == -1 &&
                turtle.enemyAt(0).alarm1 >= 117 &&
                turtle.enemyAt(0).alarm1 <= 297,
            "ground turtle walks and reschedules its recovered animation alarm");

    AmigaPortRuntime frog(300);
    frog.configureSolidResolver(flatGround, 0);
    require(frog.configureEnemy(0, 113, 171, 40, 1,
                                0, 0, 35, 20,
                                0, 0, -1, 1),
            "generated frog can retain its recovered alarm state");
    frog.step(0);
    require(frog.enemyAt(0).grounded &&
                frog.enemyAt(0).auxiliaryState == 0 &&
                frog.enemyAt(0).alarm1 == 60,
            "frog alarm observes prior ground state before its Step event");
    for (unsigned frame = 0; frame < 60; ++frame)
        frog.step(0);
    require(frog.enemyAt(0).grounded &&
                frog.enemyAt(0).auxiliaryState == 1 &&
                frog.enemyAt(0).motionState == -2 &&
                frog.enemyAt(0).alarm0 == 60,
            "frog alarm enters its grounded crouch toward the player");
    for (unsigned frame = 0; frame < 60; ++frame)
        frog.step(0);
    require(!frog.enemyAt(0).grounded &&
                frog.enemyAt(0).auxiliaryState == 0 &&
                frog.enemyAt(0).speedY < 0 &&
                frog.enemyAt(0).preciseX < 40L * 256L,
            "frog alarm launches its recovered directional jump");

    AmigaPortRuntime snail(300);
    snail.configureSolidResolver(shortRightWall, 0);
    require(snail.configureEnemy(0, 117, 177, 0, 0,
                                 0, 26, 20, 24),
            "generated wall snail can enter native actor storage");
    snail.step(0);
    require(snail.enemyAt(0).active &&
                snail.enemyAt(0).motionState == 1 &&
                snail.enemyAt(0).facing == 1,
            "wall snail discovers and records its recovered wall side");
    for (unsigned frame = 0;
         frame < 120 && snail.enemyAt(0).speedY > 0; ++frame)
        snail.step(0);
    require(snail.enemyAt(0).speedY < 0,
            "wall snail reverses at the end of its supporting wall");

    AmigaPortRuntime detachedSnail(300);
    detachedSnail.configureSolidResolver(noSolid, 0);
    require(detachedSnail.configureEnemy(0, 117, 177, 0, 0,
                                         0, 26, 20, 24),
            "detached wall snail configuration succeeds");
    detachedSnail.step(0);
    require(!detachedSnail.enemyAt(0).active,
            "wall snail retires when neither recovered wall side exists");

    AmigaPortRuntime crawler(300);
    crawler.configureSolidResolver(crawlerCorner, 0);
    require(crawler.configureEnemy(0, 119, 181, 0, 0,
                                   0, 0, 3, 6,
                                   1, 0, 2, -1),
            "generated crawler can retain its recovered direction and alarm");
    for (unsigned frame = 0;
         frame < 30 && !crawler.enemyAt(0).grounded; ++frame)
        crawler.step(0);
    require(crawler.enemyAt(0).grounded,
            "crawler falls onto terrain before beginning surface motion");
    for (unsigned frame = 0; frame < 6; ++frame)
        crawler.step(0);
    require(crawler.enemyAt(0).preciseX == 256L &&
                crawler.enemyAt(0).preciseY < 256L &&
                crawler.enemyAt(0).alarm0 > 0,
            "crawler advances along the floor then turns up the wall");

    AmigaPortRuntime bat(300);
    bat.configureSolidResolver(noSolid, 0);
    require(bat.configureEnemy(0, 116, 175, 40, 0,
                               0, 0, 15, 12,
                               0, 0, -1, -1, 180, 320, false),
            "generated bat retains direction and deferred activation");
    bat.step(0);
    require(bat.enemyAt(0).active && !bat.enemyAt(0).activationPending &&
                bat.enemyAt(0).preciseX == 40L * 256L,
            "visible dormant bat activates after the current simulation phase");
    bat.step(0);
    require(bat.enemyAt(0).preciseX < 40L * 256L &&
                bat.enemyAt(0).moveSpeed >= 256 &&
                bat.enemyAt(0).moveSpeed <= 320,
            "visible bat activates and steers toward the player in fixed point");

    AmigaPortRuntime distantBat(300);
    distantBat.configureSolidResolver(noSolid, 0);
    require(distantBat.configureEnemy(0, 116, 175, 40, 500,
                                      0, 0, 15, 12,
                                      0, 0, -1, -1, 180, 320, false),
            "distant bat configuration succeeds");
    distantBat.step(0);
    require(!distantBat.enemyAt(0).active &&
                distantBat.enemyAt(0).activationPending,
            "off-screen bat remains dormant until the camera reaches it");

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
