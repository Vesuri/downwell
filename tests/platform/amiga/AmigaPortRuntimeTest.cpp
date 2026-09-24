#include "platform/amiga/AmigaPortRuntime.h"
#include "core/GemVerification.h"
#include "platform/amiga/AmigaActorChannels.h"
#include "core/PortReplay.h"
#include "core/Replay.h"
#include "core/GameMakerRounding.h"
#include "core/EnemyPresentation.h"

#include <cstdlib>
#include <cmath>
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
    return y >= 7 || x >= 8;
}

struct GenerationOrderProbe {
    unsigned calls = 0;
    downwell::GameMakerRandom expected{2};
    static bool step(void* opaque, AmigaPortRuntime& runtime,
                     downwell::GameMakerRandom& random, int bottom) {
        auto& probe = *static_cast<GenerationOrderProbe*>(opaque);
        ++probe.calls;
        require(bottom >= 318, "generation receives world-space viewport bottom");
        if (probe.calls == 1) {
            require(bottom == 323,
                    "builder sees entry camera before the camera Step advances it");
            probe.expected.irandomSmall(180); // Existing turtle alarm fires first.
            require(random.drawCount() == probe.expected.drawCount(),
                    "existing enemy alarm RNG precedes generation");
            require(random.nextU32() == probe.expected.nextU32(),
                    "generation shares the actual enemy RNG stream");
            require(runtime.configureEnemy(1, 112, 169, 80, 0, 256, 0,
                        5, 14, 0, 1, -1, 1, 0, 0, true, true, true),
                    "generation creates a live grounded turtle");
            require(runtime.enemyAt(1).grounded && runtime.enemyAt(1).awake,
                    "Create flags survive native configuration");
            return true;
        }
        return false;
    }
};

} // namespace

int main() {
    for (unsigned rate : {0u, 2u, 3u, 4u, 5u}) {
        unsigned phase = 0;
        unsigned char remainder = 0;
        for (unsigned tick = 1; tick <= 2000; ++tick) {
            downwell::advanceEnemyImageQ8(phase, remainder, rate);
            require(phase == tick * rate * 64u / 5u &&
                        remainder == tick * rate * 64u % 5u,
                    "enemy decimal animation rates do not accumulate Q8 drift");
        }
    }
    const long ties[] = {0, 2, 2, 4};
    for (int i = 0; i < 4; ++i) {
        require(downwell::roundGameMakerQ8(i * 256 + 128) == ties[i] &&
                    downwell::roundGameMakerQ8(-i * 256 - 128) == -ties[i],
                "GML exact half coordinates round to even on both sides of zero");
    }
    for (long q = -1024 * 256; q <= 1024 * 256; ++q) {
        require(downwell::floorQ8Coordinate(q) == (long)std::floor(q / 256.0),
                "collision pixel lookup floors negative and positive fractions");
        const long rounded = downwell::roundGameMakerQ8(q);
        require(rounded == downwell::roundGameMakerCoordinate(q / 256.0),
                "fixed-point coordinate rounding matches portable oracle");
        require(downwell::roundGameMakerQ8(q + 240 * 256) - 240 == rounded &&
                    downwell::roundGameMakerQ8(q + 176 * 256) - 176 == rounded,
                "even world origins preserve rounded relative coordinates");
    }
    AmigaPortRuntime roundingActors(300, true, 0, true);
    roundingActors.configureSolidResolver(noSolid, nullptr);
    require(roundingActors.configureEnemy(0, 111, 163, 40, 20, 192, 0, 5, 6) &&
                roundingActors.configureEnemy(1, 111, 163, -40, 20, -192, 0, 5, 6),
            "configure fractional motion on both sides of the native origin");
    roundingActors.step(0);
    require(roundingActors.enemyAt(0).x == 41 && roundingActors.enemyAt(1).x == -41 &&
                roundingActors.enemyAt(0).y == 20 && roundingActors.enemyAt(1).y == 20,
            "enemy Step rounds final coordinates instead of truncating toward origin");

    AmigaPortRuntime eventOrder(300, true, 0, true);
    eventOrder.configureSolidResolver(noSolid, nullptr);
    require(eventOrder.configureEnemy(0, 112, 169, 40, 0, 64, 0,
                5, 14, 0, 1, -1, 1), "configure existing alarm source");
    GenerationOrderProbe generation;
    eventOrder.configureGenerationStep(GenerationOrderProbe::step, &generation);
    eventOrder.step(0);
    require(eventOrder.generationSucceeded() && generation.calls == 1 &&
                eventOrder.enemyAt(0).alarm1 >= 120 &&
                eventOrder.enemyAt(1).alarm1 == 1 && eventOrder.enemyAt(1).x == 81 &&
                eventOrder.enemyAt(1).imagePhase == 38,
            "new enemy moves and animates immediately without aging its Create alarm");
    eventOrder.step(0);
    require(!eventOrder.generationSucceeded() && generation.calls == 2 &&
                eventOrder.enemyAt(1).alarm1 >= 120,
            "new alarm first fires on the following tick; generation failure is visible");
    eventOrder.step(0);
    require(generation.calls == 2, "failed generation cannot replay a partial event");
    AmigaPortRuntime lifecycle(300);
    require(lifecycle.configureEnemy(0, 111, 163, 0, -303, 0, 0, 5, 6),
            "configure enemy beyond upper cleanup boundary");
    require(lifecycle.configureEnemy(1, 111, 163, 0, -302, 0, 0, 5, 6),
            "configure enemy exactly on upper cleanup boundary");
    require(lifecycle.configureEnemy(2, 111, 163, 0, 4000, 0, 0, 5, 6),
            "configure future prebuilt enemy");
    lifecycle.step(0);
    require(!lifecycle.enemyAt(0).active && !lifecycle.enemyAt(0).activationPending,
            "enemy above viewTop minus 160 is retired, not queued for reactivation");
    require(lifecycle.enemyAt(1).active,
            "cleanup comparison is strictly less than the boundary");
    require(lifecycle.enemyAt(2).active && !lifecycle.enemyAt(2).awake,
            "future prebuilt enemies are not mistaken for out-of-view live instances");
    lifecycle.step(0);
    require(!lifecycle.enemyAt(0).active && !lifecycle.enemyAt(0).awake,
            "retired enemy stays retired on subsequent ticks");
    for (unsigned limit = 0; limit <= 8; ++limit) {
        for (unsigned pattern = 0; pattern < 256; ++pattern) {
            AmigaActorChannels channels(limit);
            unsigned expected = 0;
            bool fellBack = false;
            for (unsigned actor = 0; actor < 8; ++actor) {
                const unsigned parts = 1 + ((pattern >> actor) & 1);
                const int first = channels.take(parts);
                if (expected + parts > limit) fellBack = true;
                if (fellBack)
                    require(first == -1, "behind a BOB, smaller actors cannot jump above it in DMA");
                else {
                    require(first == static_cast<int>(expected),
                            "front-to-back actors get increasing DMA priority indices");
                    expected += parts;
                }
                require(channels.used() == expected, "multipart actor allocation is atomic");
            }
        }
    }
    static_assert(AmigaPortRuntime::bulletCapacity == 16,
                  "bullet simulation capacity matches SDL, not DMA channels");
    static_assert(AmigaPortRuntime::gemCapacity == 64,
                  "gem simulation capacity matches SDL");
    AmigaPortRuntime crowdedRewards(300);
    require(crowdedRewards.configureEnemy(0, 111, 163, 0, 5,
                                          0, 0, 5, 140),
            "configure reward exceeding the former twelve-gem limit");
    crowdedRewards.step(0);
    require(crowdedRewards.gemCount() == 14,
            "all physical reward gems survive beyond hardware sprite limits");
    unsigned rewardTotal = 0;
    for (unsigned i = 0; i < AmigaPortRuntime::gemCapacity; ++i)
        if (crowdedRewards.gemAt(i).active)
            rewardTotal += crowdedRewards.gemAt(i).amount;
    require(rewardTotal == 140, "crowded rewards do not silently lose currency");
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
    require(burst.bulletAt(0).x == -1 && burst.bulletAt(1).x == 1,
            "original default gun alternates left and right boot emission offsets");
    AmigaPortRuntime movingShot(300, false, 1000);
    for (unsigned frame = 0; frame < 12; ++frame) movingShot.step(2);
    const int muzzleX = downwell::roundGameMakerQ8(movingShot.player().offsetXFixed());
    const int muzzleY = downwell::roundGameMakerQ8(movingShot.player().offsetYFixed());
    movingShot.step(2 | 4);
    require(movingShot.bulletAt(0).x == muzzleX - 1 &&
                movingShot.bulletAt(0).y == muzzleY + 4 + 8,
            "shot originates at pre-movement rounded instance coordinates before its own Step");

    AmigaPortRuntime destructible(300);
    AmigaPortRuntime headBox(0);
    const int headX[] = {0}, headY[] = {-24};
    headBox.configureBreakables(headX, headY, 1);
    headBox.configureSolidResolver([](void* opaque, int x, int y) {
        const auto& runtime = *static_cast<AmigaPortRuntime*>(opaque);
        return runtime.breakableActiveAt(0, -24) &&
            x >= -8 && x <= 7 && y >= -32 && y <= -17;
    }, &headBox);
    headBox.step(4);
    headBox.step(4);
    const AmigaPortFrame headFrame = headBox.step(4);
    require(headBox.blocksDestroyed() == 1 &&
            !headBox.breakableActiveAt(0, -24) &&
            headFrame.destroyedBlockCount == 1 &&
            headFrame.blockX[0] == 0 && headFrame.blockY[0] == -24 &&
            headBox.player().offsetYFixed() == -11 * 256 &&
            !headBox.player().grounded() && headBox.player().shots() == 0,
            "head contact breaks the box, moves down immediately, and invalidates terrain once");
    require(headBox.step(4).destroyedBlockCount == 0 &&
            headBox.blocksDestroyed() == 1,
            "a destroyed ceiling box cannot issue repeated cache invalidations");

    AmigaPortRuntime wallShot(300, false, 1000);
    wallShot.configureSolidResolver(flatGround, nullptr);
    wallShot.step(4);
    require(wallShot.player().shots() == 1 && !wallShot.bulletActive() &&
                wallShot.blocksDestroyed() == 0,
            "ordinary solid walls absorb the default bullet without a breakable record");

    AmigaPortRuntime wallEnemyShot(300, false, 1000);
    wallEnemyShot.configureSolidResolver(flatGround, nullptr);
    wallEnemyShot.configureEnemy(0, 111, 163, -1, 12, 0, 0, 5, 6,
                                 0, 0, -1, -1, 0, 0, true, false, true);
    wallEnemyShot.step(4);
    require(wallEnemyShot.enemiesDefeated() == 1,
            "source enemy-line intersection retains precedence over the solid wall check");

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
    const int newBlockX[] = {32, 48}, newBlockY[] = {64, 64};
    require(destructible.appendBreakables(newBlockX, newBlockY, 2) &&
                destructible.blocksDestroyed() == 1 &&
                !destructible.breakableActiveAt(-1, 32) &&
                destructible.breakableActiveAt(32, 64) &&
                destructible.breakableActiveAt(48, 64),
            "new terrain appends without resurrecting destroyed blocks or resetting statistics");
    require(!destructible.appendBreakables(newBlockX, newBlockY, 512) &&
                !destructible.appendBreakables(nullptr, newBlockY, 1) &&
                destructible.appendBreakables(nullptr, nullptr, 0) &&
                destructible.blocksDestroyed() == 1 && destructible.breakableActiveAt(32, 64),
            "append overflow and missing arrays fail without mutation; empty batch is valid");
    AmigaPortRuntime indexedBlocks(300);
    AmigaPortRuntime manyBlocks(300);
    int manyX[300], manyY[300];
    for (int i = 0; i < 300; ++i) { manyX[i] = (i % 32) * 16; manyY[i] = (i / 32) * 16; }
    manyBlocks.configureBreakables(manyX, manyY, 300);
    for (int i = 0; i < 300; ++i)
        require(manyBlocks.breakableActiveAt(manyX[i], manyY[i]),
                "indexed breakable IDs above 255 do not wrap or disappear");
    AmigaPortRuntime initiallyEmpty(300);
    require(initiallyEmpty.appendBreakables(newBlockX, newBlockY, 2) &&
                initiallyEmpty.breakableActiveAt(32, 64) && initiallyEmpty.breakableActiveAt(48, 64),
            "first live batch can append to an empty runtime without prior configuration");
    const int firstX[] = {0}, firstY[] = {32};
    indexedBlocks.configureBreakables(firstX, firstY, 1);
    require(indexedBlocks.appendBreakables(newBlockX, newBlockY, 2) &&
                indexedBlocks.breakableActiveAt(0, 32) && indexedBlocks.breakableActiveAt(32, 64) &&
                indexedBlocks.breakableActiveAt(48, 64) && !indexedBlocks.breakableActiveAt(16, 64),
            "aligned batches extend the indexed collision path without losing prior blocks");

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
    require(combat.gemCount() == 3 &&
                !combat.gemAt(0).attracted &&
                (combat.gemAt(0).speedX != 0 ||
                 combat.gemAt(0).speedY != 0),
            "gun-kill rewards launch freely with recovered random velocity");

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
                turtle.enemyAt(0).alarm1 <= 297 &&
                turtle.enemyAt(0).spriteId == 188,
            "ground turtle walks and reschedules its recovered animation alarm");

    AmigaPortRuntime frog(300);
    AmigaPortRuntime simultaneousFrog(300);
    simultaneousFrog.configureSolidResolver(flatGround, nullptr);
    require(simultaneousFrog.configureEnemy(0, 113, 166, 40, 1, 0, 0, 35, 20,
                2, 1, 1, 1, 0, 0, true, true, true),
            "configure both frog alarms expiring together");
    simultaneousFrog.step(0);
    require(simultaneousFrog.enemyAt(0).auxiliaryState == 1 &&
                simultaneousFrog.enemyAt(0).motionState == -2 &&
                simultaneousFrog.enemyAt(0).frogHorizontal.velocity == -65536L &&
                simultaneousFrog.enemyAt(0).alarm0 == 60 &&
                simultaneousFrog.enemyAt(0).speedY < 0,
            "alarm zero jump precedes alarm one preparation without prematurely clearing support");
    AmigaPortRuntime aimingFrog(300);
    aimingFrog.configureSolidResolver(flatGround, nullptr);
    require(aimingFrog.configureEnemy(0, 113, 166, 0, 1, 0, 0, 35, 20,
                0, 0, -1, 1, 0, 0, true, true, true),
            "configure grounded frog at player's pre-movement X");
    aimingFrog.step(2); // right: current player X becomes positive this tick
    require(aimingFrog.player().offsetXFixed() > 0 &&
                aimingFrog.enemyAt(0).motionState == -2,
            "frog alarm targets pre-movement equality to the left, not current player X");
    AmigaPortRuntime edgeFrog(300);
    edgeFrog.configureSolidResolver(flatGround, nullptr);
    require(edgeFrog.configureEnemy(0, 113, 166, 80, 1, 0, 0, 35, 20,
                0, 0, -1, 1, 0, 0, true, true, true),
            "configure grounded frog exactly at camera right edge");
    edgeFrog.step(0);
    require(edgeFrog.enemyAt(0).auxiliaryState == 0 &&
                edgeFrog.enemyAt(0).alarm1 == 60 && edgeFrog.enemyAt(0).alarm0 == -1,
            "out-of-view frog retries preparation instead of starting its jump countdown");
    frog.configureSolidResolver(flatGround, 0);
    require(frog.configureEnemy(0, 113, 171, 40, 1,
                                0, 0, 35, 20,
                                0, 0, -1, 1),
            "generated frog can retain its recovered alarm state");
    frog.step(0);
    require(frog.enemyAt(0).grounded &&
                frog.enemyAt(0).auxiliaryState == 0 &&
                frog.enemyAt(0).alarm1 == 60 &&
                frog.enemyAt(0).spriteId == 166,
            "frog alarm observes prior ground state before its Step event");
    for (unsigned frame = 0; frame < 60; ++frame)
        frog.step(0);
    require(frog.enemyAt(0).grounded &&
                frog.enemyAt(0).auxiliaryState == 1 &&
                frog.enemyAt(0).motionState == -2 &&
                frog.enemyAt(0).alarm0 == 60 &&
                frog.enemyAt(0).spriteId == 170,
            "frog alarm enters its grounded crouch toward the player");
    for (unsigned frame = 0; frame < 60; ++frame)
        frog.step(0);
    require(!frog.enemyAt(0).grounded &&
                frog.enemyAt(0).auxiliaryState == 0 &&
                frog.enemyAt(0).speedY < 0 &&
                frog.enemyAt(0).preciseX < 40L * 256L &&
                frog.enemyAt(0).spriteId == 168 &&
                frog.enemyAt(0).imageFrame == 0,
            "frog alarm launches its recovered directional jump");

    AmigaPortRuntime fallingSnake(300);
    fallingSnake.configureSolidResolver(noSolid, nullptr);
    require(fallingSnake.configureEnemy(0, 111, 163, 40, 0, 0, 0, 5, 6),
            "configure snake for exact decimal integration");
    for (long tick = 1; tick <= 30; ++tick) {
        fallingSnake.step(0);
        const auto& state = fallingSnake.enemyAt(0);
        require(state.speedY == tick * 256 / 5 &&
                    state.preciseY == tick * (tick + 1) * 128 / 5 &&
                    state.gravityPositionRemainder == tick * (tick + 1) * 128 % 5,
                "0.2 gravity retains both velocity and integrated position fractions");
    }

    AmigaPortRuntime fallingFrog(300);
    AmigaPortRuntime hitFrog(300, false, 10000);
    hitFrog.configureSolidResolver(noSolid, nullptr);
    require(hitFrog.configureEnemy(0, 113, 166, -1, 16, 512, 0, 35, 20,
                2, 1, 30, -1, 0, 0, true, false, true), "configure live bullet-hit frog");
    hitFrog.step(4);
    require(hitFrog.enemyAt(0).health == 25 && hitFrog.enemyAt(0).frogHit.pending &&
                hitFrog.enemyAt(0).frogHit.remaining == 0,
            "bullet damage queues a surviving frog's reaction for its next Step");
    hitFrog.step(0);
    require(hitFrog.enemyAt(0).frogHit.remaining == 5 && hitFrog.enemyAt(0).spriteId == 173 &&
                hitFrog.enemyAt(0).imageFrame == 0 && hitFrog.enemyAt(0).speedY == 0 &&
                hitFrog.enemyAt(0).frogHorizontal.velocity == 104858 &&
                hitFrog.enemyAt(0).auxiliaryState == 0 && hitFrog.enemyAt(0).alarm0 == 0 &&
                hitFrog.enemyAt(0).alarm1 == 60,
            "frog hit selects damage art, resets frame/vertical speed and cancels preparation");
    for (unsigned i = 0; i < 5; ++i) hitFrog.step(0);
    require(hitFrog.enemyAt(0).frogHit.remaining == 0 && hitFrog.enemyAt(0).spriteId == 168,
            "frog returns to airborne art when the stun alarm expires");
    AmigaPortRuntime offscreenFrog(300);
    offscreenFrog.configureSolidResolver(noSolid, nullptr);
    require(offscreenFrog.configureEnemy(0, 113, 166, 40, 400,
                512, 0, 35, 20, 0, 0, -1, -1), "configure frog below motion view");
    for (unsigned i = 0; i < 60; ++i) offscreenFrog.step(0);
    require(offscreenFrog.enemyAt(0).preciseX == 40 * 256L &&
                offscreenFrog.enemyAt(0).preciseY == 400 * 256L &&
                offscreenFrog.enemyAt(0).speedY == 7 * 256L,
            "native offscreen frog accumulates gravity without moving, even before legacy wake threshold");
    fallingFrog.configureSolidResolver(noSolid, nullptr);
    require(fallingFrog.configureEnemy(0, 113, 171, 40, 1,
                0, 0, 35, 20, 0, 0, -1, -1), "configure free-falling frog");
    for (long tick = 1; tick <= 40; ++tick) {
        fallingFrog.step(0);
        const auto& state = fallingFrog.enemyAt(0);
        require(state.speedY == tick * 192 / 5 && state.gravityRemainder == tick * 2 % 5,
                "frog gravity retains exact fifth-unit velocity remainder");
        const long displacementFifths = tick * (tick + 1) * 96;
        require(state.preciseY == 256 + displacementFifths / 5 &&
                state.gravityPositionRemainder == displacementFifths % 5,
                "frog position integrates the gravity remainder rather than discarding it");
        const double expectedY = 1.0 + 0.15 * tick * (tick + 1) / 2.0;
        require(std::fabs(state.preciseY / 256.0 - expectedY) < tick / 256.0,
                "frog ballistic position stays within one Q8 unit per integration step");
    }

    for (unsigned tick = 0; tick < 20; ++tick) fallingFrog.step(0);
    require(fallingFrog.enemyAt(0).speedY == 7 * 256 &&
                fallingFrog.enemyAt(0).gravityRemainder == 0,
            "terminal velocity clears gravity remainder rather than accumulating hidden speed");

    AmigaPortRuntime unsupportedFrog(300);
    AmigaPortRuntime fractionalLanding(300);
    AmigaPortRuntime movingLanding(300);
    movingLanding.configureSolidResolver(flatGround, nullptr);
    require(movingLanding.configureEnemy(0, 113, 166, 40, 1,
                512, 0, 35, 20, 0, 0, -1, -1), "configure moving frog reaching support");
    movingLanding.step(0);
    require(movingLanding.enemyAt(0).grounded && movingLanding.enemyAt(0).preciseX == 40 * 256L,
            "source integration suppresses horizontal displacement on the landing frame");
    for (int direction : {-1, 1}) {
        AmigaPortRuntime edgeProbe(300);
        edgeProbe.configureSolidResolver([](void* opaque, int x, int) {
            return *static_cast<int*>(opaque) < 0 ? x <= -9 : x >= 8;
        }, &direction);
        require(edgeProbe.configureEnemy(0, 113, 166, 0, 40,
                    direction * 512, 0, 35, 20, 0, 0, -1, -1),
                "configure frog one pixel away from a side wall after movement");
        edgeProbe.step(0);
        require(edgeProbe.enemyAt(0).preciseX == direction * 512 &&
                    edgeProbe.enemyAt(0).speedX == direction * 512,
                "recovered frog leading edge does not rebound one pixel before contact");
    }
    fractionalLanding.configureSolidResolver(flatGround, nullptr);
    require(fractionalLanding.configureEnemy(0, 113, 166, 40, 0,
                0, 0, 35, 20, 0, 0, -1, -1), "configure fractional frog landing");
    for (unsigned i = 0; i < 4; ++i) fractionalLanding.step(0);
    require(fractionalLanding.enemyAt(0).grounded &&
                fractionalLanding.enemyAt(0).gravityPositionRemainder == 2,
            "whole-pixel provisional landing movement preserves the position fraction");
    const long restingY = fractionalLanding.enemyAt(0).preciseY;
    fractionalLanding.step(0);
    require(fractionalLanding.enemyAt(0).preciseY == restingY &&
                fractionalLanding.enemyAt(0).gravityPositionRemainder == 2,
            "resting on support preserves complete position, not just Q8 whole");
    unsupportedFrog.configureSolidResolver(flatGround, nullptr);
    require(unsupportedFrog.configureEnemy(0, 113, 171, 40, 1,
                0, 0, 35, 20, 0, 0, -1, 60), "configure frog on removable support");
    unsupportedFrog.step(0);
    require(unsupportedFrog.enemyAt(0).grounded &&
                unsupportedFrog.enemyAt(0).gravityRemainder == 0,
            "frog landing clears the gravity remainder");
    const long supportedY = unsupportedFrog.enemyAt(0).preciseY;
    unsupportedFrog.configureSolidResolver(noSolid, nullptr);
    unsupportedFrog.step(0);
    require(!unsupportedFrog.enemyAt(0).grounded &&
                unsupportedFrog.enemyAt(0).preciseY > supportedY &&
                unsupportedFrog.enemyAt(0).alarm1 > 0,
            "frog falls immediately when support disappears, without waiting for jump alarm");
    require(!unsupportedFrog.enemyAt(0).frogLanded,
            "an airborne Step clears the original landed latch");
    unsupportedFrog.configureSolidResolver(flatGround, nullptr);
    unsupportedFrog.step(0);
    require(unsupportedFrog.enemyAt(0).grounded &&
                unsupportedFrog.enemyAt(0).frogLanded &&
                unsupportedFrog.enemyAt(0).alarm1 == 60,
            "landing restarts the original sixty-frame preparation alarm");
    unsupportedFrog.step(0);
    require(unsupportedFrog.enemyAt(0).alarm1 == 59,
            "remaining grounded does not continually restart the alarm");

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
    require(snail.enemyAt(0).drawFrame() == snail.enemyAt(0).imageFrame + 256,
            "descending wall snail selects offline vertically flipped artwork");
    for (unsigned frame = 0;
         frame < 120 && snail.enemyAt(0).speedY > 0; ++frame)
        snail.step(0);
    require(snail.enemyAt(0).speedY < 0,
            "wall snail reverses at the end of its supporting wall");
    require(snail.enemyAt(0).drawFrame() == snail.enemyAt(0).imageFrame,
            "ascending wall snail restores unflipped artwork");

    AmigaPortRuntime detachedSnail(300);
    AmigaPortRuntime hitSnail(300, false, 1000);
    hitSnail.configureSolidResolver(shortRightWall, nullptr);
    require(hitSnail.configureEnemy(0, 117, 177, -1, 12, 0, -26, 20, 24,
                                    0, 0, -1, -1, 0, 0, true, false, true),
            "configure snail that survives a usual-gun bullet");
    hitSnail.step(4);
    require(hitSnail.enemyAt(0).health == 10 && hitSnail.enemyAt(0).snailHit.pending &&
            hitSnail.enemyAt(0).spriteId == 177, "surviving hit queues the snail reaction");
    hitSnail.step(0);
    require(hitSnail.enemyAt(0).spriteId == 178 && hitSnail.enemyAt(0).snailHit.alarm2 == 4 &&
            hitSnail.enemyAt(0).alarm0 == 0 && hitSnail.enemyAt(0).alarm1 >= 50 &&
            hitSnail.enemyAt(0).alarm1 < 80, "snail hit selects damage art and source alarm changes");
    for (unsigned i = 0; i < 4; ++i) hitSnail.step(0);
    require(hitSnail.enemyAt(0).spriteId == 177 && !hitSnail.enemyAt(0).snailHit.stunned,
            "snail returns to normal artwork after the hit alarm");
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
    for (unsigned frame = 0; frame < 12; ++frame)
        crawler.step(0);
    require(crawler.enemyAt(0).preciseX == 3 * 256L &&
                crawler.enemyAt(0).preciseY < 2 * 256L &&
                crawler.enemyAt(0).alarm0 > 0,
            "crawler advances along the floor then turns up the wall");
    require(crawler.enemyAt(0).drawFrame() == crawler.enemyAt(0).imageFrame + 256,
            "upward crawler chooses its host-generated ninety-degree frame");

    AmigaPortRuntime bat(300);
    bat.configureSolidResolver(noSolid, 0);
    require(bat.configureEnemy(0, 116, 175, 40, 0,
                               0, 0, 15, 12,
                               0, 0, -1, -1, 180, 320, false),
            "generated bat retains direction and deferred activation");
    for (unsigned frame = 0; frame < 10; ++frame) bat.step(0);
    require(!bat.enemyAt(0).active && bat.enemyAt(0).activationPending,
            "activation frontier advances only sixteen pixels per Begin Step");
    bat.step(0);
    require(bat.enemyAt(0).active && !bat.enemyAt(0).activationPending &&
                bat.enemyAt(0).preciseX < 40L * 256L,
            "world-y 176 enemy activates on frontier step eleven before movement");
    bat.step(0);
    require(bat.enemyAt(0).preciseX < 40L * 256L &&
                bat.enemyAt(0).moveSpeed >= 256 &&
                bat.enemyAt(0).moveSpeed <= 320,
            "visible bat activates and steers toward the player in fixed point");

    AmigaPortRuntime animatedBat(300);
    animatedBat.configureSolidResolver(noSolid, 0);
    require(animatedBat.configureEnemy(0, 116, 153, 40, 0,
                                        0, 0, 15, 12,
                                        0, 0, -1, -1,
                                        180, 320, true),
            "active bat is available for recovered animation timing");
    for (unsigned frame = 0; frame < 15; ++frame)
        animatedBat.step(0);
    require(animatedBat.enemyAt(0).spriteId == 153 &&
                animatedBat.enemyAt(0).imageFrame == 3,
            "0.2-speed bat reaches frame three after exactly fifteen updates");

    AmigaPortRuntime walkingTurtle(300);
    walkingTurtle.configureSolidResolver(flatGround, 0);
    require(walkingTurtle.configureEnemy(0, 112, 186, 40, 0,
                                          64, 0, 5, 14,
                                          0, 1, -1, 100,
                                          0, 0, true),
            "walking turtle is available for recovered animation timing");
    for (unsigned frame = 0; frame < 34; ++frame)
        walkingTurtle.step(0);
    require(walkingTurtle.enemyAt(0).spriteId == 188 &&
                walkingTurtle.enemyAt(0).imageFrame == 5,
            "six-frame walking-turtle cycle retains its final two frames");

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
                stomp.gemAt(0).attracted &&
                stomp.gemAt(1).amount == 2 &&
                stomp.gemAt(2).amount == 2,
            "six-value snake reward splits into three recovered small gems");
    for (unsigned frame = 0;
         frame < 120 && stomp.gemCount() != 0; ++frame)
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
                contact.invulnerability() == 90 &&
                contact.player().speedXFixed() == 0 &&
                contact.playerVisible(),
            "side contact removes one health and starts invulnerability");
    contactFrame = contact.step(0);
    require(contact.health() == 3 && !contactFrame.healthChanged &&
                contact.invulnerability() == 89 &&
                !contact.playerVisible(),
            "first damaged Step suppresses repeated damage and hides player");
    contactFrame = contact.step(0);
    require(contact.invulnerability() == 88 && contact.playerVisible(),
            "recovered dFlash alternates player visibility every damaged Step");

    while (!contact.dead() && contact.tick() < 400)
        contact.step(0);
    require(contact.dead() && contact.health() == 0 &&
                contact.deathFrames() == 0,
            "four recovered contacts enter death with clamped zero health");
    const unsigned deathShots = contact.player().shots();
    const int deathCameraY = contact.cameraOffsetY();
    for (unsigned frame = 0; frame < 99; ++frame)
        contact.step(4);
    require(contact.player().shots() == deathShots &&
                contact.cameraOffsetY() == deathCameraY &&
                !contact.deathMenuReady(),
            "death disables control and freezes the camera during animation");
    contact.step(4);
    require(contact.deathFrames() == 100 && contact.deathMenuReady(),
            "recovered 90 plus 10 frame death alarms reach the menu gate");

    AmigaPortRuntime sideCamera(300, true, 0, true);
    sideCamera.configureSolidResolver(noSolid, 0);
    downwell::Input walkRight;
    walkRight.right = true;
    AmigaPortFrame sideFrame{};
    for (unsigned frame = 0;
         frame < 120 && sideCamera.cameraOffsetX() == 0; ++frame)
        sideFrame = sideCamera.step(downwell::packReplayInput(walkRight));
    require(sideCamera.player().offsetXFixed() > 80L * 256 &&
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
    std::cout << "Native trajectory digest: " << std::hex
              << replayRuntime.trajectoryDigest() << std::dec << '\n';
    require(replayRuntime.trajectoryDigest() == 0x9f0d3943e7900d2bULL,
            "cross-target trajectory digest remains pinned");
    const auto gems = downwell::verifyGemSimulation();
    require(gems.coverage == 63 && gems.digest == 0x9543220ed0d8e596ULL,
            "400-update gem cross-CPU fixture retains all branches and pinned full-state digest");
    std::cout << "Amiga port runtime tests passed\n";
    return 0;
}
