#include "platform/amiga/AmigaSceneRuntime.h"
#include "platform/amiga/AmigaPortRuntime.h"
#include "platform/amiga/AmigaTerrain.h"
#include "core/BuilderPatternPack.h"
#include "core/ObjectPack.h"
#include "core/Replay.h"
#include "core/RoomPack.h"
#include "core/StartupRuntime.h"
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

namespace {
int parityFrame = 0;

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message
                  << " (parity frame " << parityFrame << ")\n";
        std::exit(1);
    }
}

void pressFire(AmigaSceneRuntime& runtime) {
    downwell::Input fire;
    fire.fire = true;
    runtime.step(fire);
    runtime.step(downwell::Input{});
}

std::vector<std::uint8_t> readFile(const char* path) {
    std::ifstream input(path, std::ios::binary);
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(input),
                                     std::istreambuf_iterator<char>());
}

void requireNear(float actual, float expected, float tolerance,
                 const char* message) {
    if (std::fabs(actual - expected) > tolerance) {
        std::cerr << "FAILED: " << message << " (Amiga " << actual
                  << ", reference " << expected << ", parity frame "
                  << parityFrame << ")\n";
        std::exit(1);
    }
}

void compareMenu(const AmigaSceneRuntime& amiga,
                 const downwell::StartupRuntime& reference) {
    require(reference.inMenu(), "reference remains in menu during comparison");
    require(amiga.scene() == AmigaScene::menu,
            "Amiga remains in menu during comparison");
    requireNear(static_cast<float>(amiga.menuPlayerX()),
                reference.menuPlayerX(), 1.0F,
                "menu player X follows decoded runtime");
    requireNear(static_cast<float>(amiga.menuPlayerY()),
                reference.menuPlayerY(), 1.0F,
                "menu player Y follows decoded runtime");
    requireNear(static_cast<float>(amiga.menuCameraX()),
                reference.menuCameraViewX() + 80.0F, 4.0F,
                "menu camera X follows decoded runtime");
    requireNear(static_cast<float>(amiga.menuCameraY()),
                reference.menuCameraViewY() + 142.0F, 4.0F,
                "menu camera Y follows decoded runtime");
    require(amiga.menuPlayerNapping() == reference.menuPlayerNapping(),
            "menu sleeping state follows decoded runtime");
    require(amiga.menuPlayerVisible() == reference.menuPlayerVisible(),
            "menu visibility follows decoded runtime");
    require(amiga.menuPlayerSprite() == reference.menuPlayerSpriteId(),
            "menu player sprite follows decoded runtime");
    unsigned referenceFrames = 1;
    if (reference.menuPlayerSpriteId() == 12 ||
        reference.menuPlayerSpriteId() == 27)
        referenceFrames = 8;
    else if (reference.menuPlayerSpriteId() == 31)
        referenceFrames = 4;
    else if (reference.menuPlayerSpriteId() == 37)
        referenceFrames = 5;
    const unsigned referenceFrame =
        static_cast<unsigned>(reference.menuPlayerImageIndex()) %
        referenceFrames;
    if (amiga.menuPlayerFrame() != referenceFrame) {
        std::cerr << "FAILED: menu player frame follows decoded runtime "
                  << "(Amiga " << amiga.menuPlayerFrame()
                  << ", reference " << referenceFrame
                  << ", reference index " << reference.menuPlayerImageIndex()
                  << ", parity frame " << parityFrame << ")\n";
        std::exit(1);
    }
    require(amiga.menuTeleportActive() == reference.menuTeleportActive(),
            "menu teleport follows decoded runtime");
    const downwell::InstanceRecord* title = reference.instance(105192);
    require(title != nullptr, "decoded menu title instance remains available");
    require(amiga.menuTitleFrame() ==
                static_cast<int>(title->imageIndex) &&
            amiga.menuTitleY() == title->y,
            "menu title animation follows decoded runtime");
    require(amiga.menuEnterSequenceActive() == reference.enterSequenceActive(),
            "well-entry sequence follows decoded runtime");
    if (amiga.menuEnterSequenceActive())
        requireNear(static_cast<float>(amiga.menuEnterSequenceY()),
                    reference.enterSequenceY(), 1.0F,
                    "well-entry vertical motion follows decoded runtime");
}

void compareStyle(const AmigaSceneRuntime& amiga,
                  const downwell::StartupRuntime& reference) {
    require(reference.inPlayMenu(),
            "reference remains in style select during comparison");
    require(amiga.scene() == AmigaScene::styleSelect,
            "Amiga remains in style select during comparison");
    requireNear(static_cast<float>(amiga.styleYOffset()),
                reference.playMenuYOffset(), 1.0F,
                "style entrance easing follows decoded runtime");
    requireNear(static_cast<float>(amiga.styleTileY()),
                reference.playMenuTileY(), 1.0F,
                "style tile scroll follows decoded runtime");
    requireNear(static_cast<float>(amiga.styleDitherY()),
                reference.playMenuDitherY(), 1.0F,
                "style dither scroll follows decoded runtime");
    requireNear(static_cast<float>(amiga.styleSmoothX()),
                reference.playMenuSmoothX(), 1.0F,
                "style carousel easing follows decoded runtime");
    require(amiga.styleCursor() == reference.playMenuCursor(),
            "style cursor follows decoded runtime");
    require(amiga.styleSelected() == reference.playMenuSelected(),
            "style selection follows decoded runtime");
    requireNear(static_cast<float>(amiga.stylePlayerDescent()),
                reference.playMenuPlayerDescent(), 1.0F,
                "style player descent follows decoded runtime");
    requireNear(static_cast<float>(amiga.styleVerticalDitherY()),
                reference.playMenuVerticalDitherY(), 1.0F,
                "style exit dither follows decoded runtime");
}

void compareInitialGameplay(const AmigaPortRuntime& amiga,
                            const downwell::StartupRuntime& reference,
                            float playerYTolerance = 4.0F,
                            float cameraYTolerance = 2.0F) {
    require(reference.inMainRoom(),
            "reference remains in gameplay during comparison");
    requireNear(static_cast<float>(240 + amiga.player().offsetX()),
                reference.mainPlayerX(), 1.0F,
                "gameplay player X follows decoded runtime");
    requireNear(static_cast<float>(176 + amiga.player().offsetY()),
                reference.mainPlayerY(), playerYTolerance,
                "gameplay player Y follows decoded runtime");
    requireNear(static_cast<float>(160 + amiga.cameraOffsetX()),
                reference.mainCameraViewX(), 1.0F,
                "gameplay camera X follows decoded runtime");
    requireNear(static_cast<float>(34 + amiga.cameraOffsetY()),
                reference.mainCameraViewY(), cameraYTolerance,
                "gameplay camera Y follows decoded runtime");
    require(amiga.player().facing() == reference.mainPlayerFacing(),
            "gameplay facing follows decoded runtime");
    if (amiga.player().grounded() != reference.mainPlayerGrounded()) {
        std::cerr << "FAILED: gameplay grounded state follows decoded runtime "
                  << "(Amiga " << amiga.player().grounded()
                  << " at " << 176 + amiga.player().offsetY()
                  << ", reference " << reference.mainPlayerGrounded()
                  << " at " << reference.mainPlayerY()
                  << ", parity frame " << parityFrame << ")\n";
        std::exit(1);
    }
    if (amiga.player().ammo() != reference.mainAmmo()) {
        std::cerr << "FAILED: gameplay ammo follows decoded runtime "
                  << "(Amiga " << amiga.player().ammo() << ", reference "
                  << reference.mainAmmo() << ", parity frame "
                  << parityFrame << ")\n";
        std::exit(1);
    }
    if (amiga.health() != reference.mainPlayerHealth()) {
        std::cerr << "FAILED: gameplay health follows decoded runtime "
                  << "(Amiga " << amiga.health() << ", reference "
                  << reference.mainPlayerHealth() << ", parity frame "
                  << parityFrame << ")\n";
        std::exit(1);
    }
}

void compareGameplayShot(const AmigaPortRuntime& amiga,
                         const downwell::StartupRuntime& reference) {
    require(amiga.player().jumps() == reference.mainJumps() &&
                amiga.player().shots() == reference.mainShotsFired(),
            "gameplay jump and shot counts follow decoded runtime");
    require(amiga.bulletCount() == reference.mainBulletCount(),
            "gameplay bullet count follows decoded runtime");
    if (amiga.bulletCount() != 0) {
        const AmigaPortRuntime::BulletState& bullet = amiga.bulletAt(0);
        requireNear(static_cast<float>(240 + bullet.x),
                    reference.mainBulletAt(0).x, 1.0F,
                    "gameplay bullet X follows decoded runtime");
        requireNear(static_cast<float>(176 + bullet.y),
                    reference.mainBulletAt(0).y, 3.0F,
                    "gameplay bullet Y follows decoded runtime");
    }
}

void testDecodedSceneParity(const char* roomPath, const char* objectPath,
                            const char* builderPatternPath) {
    const std::vector<std::uint8_t> roomBytes = readFile(roomPath);
    const std::vector<std::uint8_t> objectBytes = readFile(objectPath);
    const std::vector<std::uint8_t> patternBytes = readFile(builderPatternPath);
    const downwell::RoomPack rooms(roomBytes.data(), roomBytes.size());
    const downwell::ObjectPack objects(objectBytes.data(), objectBytes.size());
    const downwell::BuilderPatternPack patterns(patternBytes.data(),
                                                patternBytes.size());
    require(rooms.valid() && objects.valid() && patterns.valid(),
            "generated recovery packs are valid for scene parity");

    downwell::StartupRuntime reference(
        downwell::PlatformProfile::nonDesktop, &rooms, &objects,
        downwell::StartupRuntime::defaultRandomSeed, &patterns);
    reference.step(false);
    require(reference.inMenu(), "decoded runtime enters menu");

    AmigaSceneRuntime amiga;
    downwell::Input fire;
    fire.fire = true;
    for (int stage = 0; stage < 5; ++stage) {
        amiga.step(fire);
        amiga.step(downwell::Input{});
    }
    amiga.step(fire);
    require(amiga.scene() == AmigaScene::menu,
            "Amiga runtime enters menu on the sixth press");

    downwell::Input released;
    for (int tick = 0; tick < 30; ++tick) {
        parityFrame = tick;
        amiga.step(released);
        reference.step(released, false);
        compareMenu(amiga, reference);
    }

    downwell::Input right;
    right.right = true;
    for (int tick = 0; tick < 500 && reference.inMenu(); ++tick) {
        parityFrame = 30 + tick;
        amiga.step(right);
        reference.step(right, false);
        if (!reference.inMenu() || amiga.scene() != AmigaScene::menu)
            break;
        compareMenu(amiga, reference);
    }
    if (reference.inMenu() || amiga.scene() != AmigaScene::styleSelect) {
        std::cerr << "FAILED: both runtimes reach style select on the same input "
                  << "tape (parity frame " << parityFrame
                  << ", reference menu " << reference.inMenu()
                  << ", Amiga scene " << static_cast<int>(amiga.scene())
                  << ")\n";
        std::exit(1);
    }

    for (int tick = 0; tick < 100; ++tick) {
        parityFrame = 366 + tick;
        amiga.step(released);
        reference.step(released, false);
        compareStyle(amiga, reference);
    }

    amiga.step(right);
    reference.step(right, false);
    compareStyle(amiga, reference);
    require(amiga.styleCursor() == 1,
            "both runtimes move to the locked second style");

    amiga.step(fire);
    reference.step(fire, true);
    compareStyle(amiga, reference);
    require(!amiga.styleSelected(),
            "both runtimes reject the locked style");

    downwell::Input left;
    left.left = true;
    amiga.step(left);
    reference.step(left, false);
    compareStyle(amiga, reference);
    require(amiga.styleCursor() == 0,
            "both runtimes return to the default style");

    amiga.step(released);
    reference.step(released, false);
    compareStyle(amiga, reference);
    amiga.step(fire);
    reference.step(fire, true);
    compareStyle(amiga, reference);
    require(amiga.styleSelected(),
            "both runtimes confirm the default style");

    for (int tick = 0; tick < 63; ++tick) {
        parityFrame = 470 + tick;
        amiga.step(released);
        reference.step(released, false);
        compareStyle(amiga, reference);
    }
    amiga.step(released);
    reference.step(released, false);
    require(amiga.scene() == AmigaScene::gameplay && reference.inMainRoom(),
            "both runtimes enter gameplay on the same frame");

    AmigaPortRuntime gameplay(300, false, 1201 - 176, true);
    downwell::PackedRoom terrainRoom;
    AmigaTerrainFragment terrain[amigaInitialTerrainFragmentCount];
    AmigaTerrainEnemy generatedEnemies[AmigaPortRuntime::enemyCapacity];
    unsigned generatedEnemyCount = 0;
    downwell::GameMakerRandom gameplayRandom(2);
    int breakableX[amigaTerrainBreakableCapacity];
    int breakableY[amigaTerrainBreakableCapacity];
    require(rooms.findRoom(11, terrainRoom) &&
                amigaBuildInitialAreaOneTerrain(
                    patterns, terrain, amigaInitialTerrainFragmentCount,
                    generatedEnemies, AmigaPortRuntime::enemyCapacity,
                    generatedEnemyCount, &gameplayRandom),
            "native gameplay terrain is recovered for parity");
    const unsigned breakableCount = amigaCollectTerrainBreakables(
        patterns, terrain, amigaInitialTerrainFragmentCount,
        breakableX, breakableY, amigaTerrainBreakableCapacity);
    require(breakableCount == 124,
            "all recovered area-one breakables fit native storage");
    AmigaTerrainCollisionContext terrainContext = {
        &rooms, &terrainRoom, &patterns, terrain,
        amigaInitialTerrainFragmentCount, &gameplay
    };
    gameplay.configureBreakables(breakableX, breakableY, breakableCount);
    gameplay.configureLandingResolver(amigaResolveTerrainLanding,
                                      &terrainContext);
    gameplay.configureSolidResolver(amigaResolveTerrainSolid,
                                    &terrainContext);
    gameplay.configureEnemyRandom(gameplayRandom);
    require(generatedEnemyCount > 3 &&
                generatedEnemyCount <= AmigaPortRuntime::enemyCapacity,
            "complete recovered enemy population fits native storage");
    for (unsigned enemyIndex = 0;
         enemyIndex < generatedEnemyCount; ++enemyIndex) {
        downwell::PackedObject definition;
        const AmigaTerrainEnemy& enemy = generatedEnemies[enemyIndex];
        require(objects.findObject(enemy.objectId, definition) &&
                    gameplay.configureEnemy(
                        enemyIndex, enemy.objectId, definition.spriteId,
                        enemy.x, enemy.y, enemy.speedX, enemy.speedY,
                        amigaEnemyHealth(enemy.objectId),
                        amigaEnemyReward(enemy.objectId),
                        enemy.motionState, enemy.auxiliaryState,
                        enemy.alarm0, enemy.alarm1,
                        enemy.direction, enemy.moveSpeed,
                        enemy.objectId != 116),
                "generated native enemy is configured from recovered data");
    }
    compareInitialGameplay(gameplay, reference);
    for (int tick = 0; tick < 220; ++tick) {
        parityFrame = 534 + tick;
        reference.step(released, false);
        gameplay.step(0);
        compareInitialGameplay(gameplay, reference);
        if (gameplay.player().grounded() && reference.mainPlayerGrounded())
            break;
    }
    require(gameplay.player().grounded() && reference.mainPlayerGrounded(),
            "both gameplay runtimes land on the recovered entrance floor");
    require(gameplay.player().spriteId() == reference.mainPlayerSpriteId() &&
                gameplay.player().spriteFrame() ==
                    static_cast<unsigned>(reference.mainPlayerImageIndex()),
            "landed gameplay pose follows decoded runtime");

    downwell::Input gameplayFire;
    gameplayFire.fire = true;
    gameplay.step(downwell::packReplayInput(gameplayFire));
    reference.step(gameplayFire, true);
    compareInitialGameplay(gameplay, reference);
    compareGameplayShot(gameplay, reference);
    gameplay.step(0);
    reference.step(released, false);
    compareInitialGameplay(gameplay, reference);
    compareGameplayShot(gameplay, reference);
    gameplay.step(downwell::packReplayInput(gameplayFire));
    reference.step(gameplayFire, true);
    compareInitialGameplay(gameplay, reference);
    compareGameplayShot(gameplay, reference);
    require(gameplay.player().ammo() == 7,
            "first airborne gunboot shot consumes one charge in both runtimes");

    for (int tick = 0; tick < 180; ++tick) {
        parityFrame = 755 + tick;
        gameplay.step(0);
        reference.step(released, false);
        if (gameplay.player().grounded() &&
            reference.mainPlayerGrounded())
            break;
        if (!gameplay.player().grounded() &&
            !reference.mainPlayerGrounded()) {
            compareInitialGameplay(gameplay, reference, 5.0F, 4.0F);
            compareGameplayShot(gameplay, reference);
        }
    }
    require(gameplay.player().grounded() && reference.mainPlayerGrounded(),
            "both runtimes land on recovered terrain after gunboot fire");
    compareInitialGameplay(gameplay, reference, 5.0F, 4.0F);
    compareGameplayShot(gameplay, reference);
    require(gameplay.player().ammo() == 8 && reference.mainAmmo() == 8,
            "landing recharges both runtimes to eight shots");

    const int groundedStartX = gameplay.player().offsetX();
    for (int tick = 0; tick < 4; ++tick) {
        parityFrame += 1;
        gameplay.step(downwell::packReplayInput(left));
        reference.step(left, false);
        compareInitialGameplay(gameplay, reference, 5.0F, 4.0F);
    }
    require(gameplay.player().offsetX() < groundedStartX,
            "native player traverses solid ground without floor snagging");
}
}

int main(int argc, char** argv) {
    AmigaSceneRuntime runtime;
    require(runtime.scene() == AmigaScene::splash &&
            runtime.splashStage() == 1, "startup begins at splash stage one");
    for (int stage = 2; stage <= 6; ++stage) {
        pressFire(runtime);
        require(runtime.splashStage() == stage,
                "fire edge advances one splash stage");
    }
    pressFire(runtime);
    require(runtime.scene() == AmigaScene::menu && runtime.menuPlayerNapping(),
            "sixth advance enters the sleeping menu scene");

    for (int tick = 0; tick < 29; ++tick)
        runtime.step(downwell::Input{});
    downwell::Input right;
    right.right = true;
    runtime.step(right);
    require(!runtime.menuPlayerNapping(),
            "the first enabled direction input wakes the player");
    for (int tick = 0; tick < 10; ++tick)
        runtime.step(right);
    require(runtime.menuPlayerX() == 123,
            "menu acceleration reaches the recovered ten-tick position");
    for (int tick = 0; tick < 600 && runtime.scene() == AmigaScene::menu; ++tick)
        runtime.step(right);
    require(runtime.scene() == AmigaScene::styleSelect,
            "walking into the well reaches style select");
    require(runtime.menuTeleportActive() && !runtime.menuPlayerVisible() &&
            runtime.menuTitleFrame() == 9,
            "well transition hides the player and completes the title");

    downwell::Input released;
    for (int tick = 0; tick < 120 && runtime.styleYOffset() != 0; ++tick)
        runtime.step(released);
    require(runtime.styleYOffset() == 0, "style-select entrance settles");
    pressFire(runtime);
    require(runtime.styleSelected(), "default style can be selected");
    for (int tick = 0; tick < 100 && runtime.scene() != AmigaScene::gameplay; ++tick)
        runtime.step(released);
    require(runtime.scene() == AmigaScene::gameplay,
            "style descent enters gameplay");
    if (argc >= 4)
        testDecodedSceneParity(argv[1], argv[2], argv[3]);
    std::cout << "Amiga scene runtime tests passed\n";
    return 0;
}
