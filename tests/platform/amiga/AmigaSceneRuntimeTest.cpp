#include "platform/amiga/AmigaSceneRuntime.h"
#include "platform/amiga/AmigaPortRuntime.h"
#include "platform/amiga/AmigaTerrain.h"
#include "core/BuilderPatternPack.h"
#include "core/ObjectPack.h"
#include "core/EnemyAlarmEvents.h"
#include "core/Replay.h"
#include "core/RoomPack.h"
#include "core/StartupRuntime.h"
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
#include <array>
#include <algorithm>

namespace {
int parityFrame = 0;
bool sawExtendedEnemyAnimation = false;
bool sawPairedFrogAir = false, sawPairedFrogGround = false;
bool sawPairedFrogHit = false;
bool sawPairedCurrencyAnimation = false;

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message
                  << " (parity frame " << parityFrame << ")\n";
        std::exit(1);
    }
}

struct LiveSceneWorld {
    AmigaTerrainFragment fragments[19];
    AmigaTerrainIndex index;
    AmigaAreaOneTerrainBuilder builder{fragments, 19};
    AmigaTerrainCollisionContext collision;
    const downwell::BuilderPatternPack& patterns;
    const downwell::ObjectPack& objects;
    AmigaPortRuntime* runtime = nullptr;

    LiveSceneWorld(const downwell::BuilderPatternPack& pack,
                   const downwell::ObjectPack& definitions)
        : collision{nullptr, nullptr, &pack, nullptr, 0, nullptr},
          patterns(pack), objects(definitions) {}

    static bool create(void* opaque, const downwell::DeferredCreateInstance& command) {
        auto& world = *static_cast<LiveSceneWorld*>(opaque);
        if (!amigaAddTerrainObject(world.index, command.objectId, command.x, command.y)) return false;
        if (command.objectId == 92) {
            const int x = command.x - 240, y = command.y - 176;
            return world.runtime->appendBreakables(&x, &y, 1);
        }
        if (amigaEnemyHealth(command.objectId)) {
            downwell::PackedObject object;
            return world.objects.findObject(command.objectId, object) &&
                world.runtime->configureEnemy(world.runtime->enemyCount(), command.objectId,
                    object.spriteId, command.x - 240, command.y - 176,
                    command.integerVelocityX, command.integerVelocityY,
                    amigaEnemyHealth(command.objectId), amigaEnemyReward(command.objectId),
                    command.motionState, command.auxiliaryState,
                    command.alarms.get(0), command.alarms.get(1),
                    command.integerDirection, command.integerMoveSpeed,
                    command.active, command.grounded, true);
        }
        return true;
    }
    static bool generate(void* opaque, AmigaPortRuntime&,
                         downwell::GameMakerRandom& random, int bottom) {
        auto& world = *static_cast<LiveSceneWorld*>(opaque);
        downwell::AreaOneBuilderStep result;
        return world.builder.stepVisible(world.patterns, random, bottom, result);
    }
    void bind(AmigaPortRuntime& port, const downwell::RoomPack& rooms,
              const downwell::PackedRoom& room) {
        runtime = &port;
        require(amigaBuildTerrainIndex(collision, index), "live scene index initializes empty");
        for (unsigned i = 0; i < room.instanceCount; ++i) {
            downwell::PackedRoomInstance instance;
            require(rooms.roomInstance(room, i, instance), "live room instance decodes");
            if (instance.objectId == 60)
                require(amigaAddTerrainObject(index, 60, instance.x, instance.y, true),
                        "initial room walls enter the live index");
        }
        collision.index = &index;
        collision.runtime = &port;
        const int emptyBlock = 0;
        port.configureBreakables(&emptyBlock, &emptyBlock, 0);
        downwell::GameMakerRandom random;
        builder.resetInitialCheckpoint(random);
        builder.setCreateObserver(create, this);
        port.configureEnemyRandom(random);
        port.configureGenerationStep(generate, this);
        port.configureLandingResolver(amigaResolveTerrainLanding, &collision);
        port.configureSolidResolver(amigaResolveTerrainSolid, &collision);
    }
};

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

void verifyFrogMotion(const AmigaPortRuntime& amiga,
                      const downwell::StartupRuntime& reference) {
    if (!std::getenv("DOWNWELL_VERIFY_FROG_MOTION")) return;
    unsigned ordinal = 0;
    for (unsigned i = 0; i < reference.activeInstanceCount(); ++i) {
        const auto& enemy = reference.instanceAt(i);
        if (!amigaEnemyHealth(enemy.objectId)) continue;
        require(ordinal < amiga.enemyCount(), "frog diagnostic retains creation-order pairing");
        const auto& native = amiga.enemyAt(ordinal++);
        if (enemy.objectId == 113 && enemy.active && native.active)
            require(native.frogPresentation.desiredRate == enemy.frogPresentation.desiredRate &&
                        native.imageSpeed == static_cast<unsigned>(std::lround(enemy.imageSpeed * 20)),
                    "frog imgSp and applied image_speed agree independently across adapters");
        require(native.active == enemy.active &&
                    native.activationPending == enemy.activationPending,
                "enemy activation/retirement agrees, including inactive instances");
        if (enemy.active && native.active)
            require(native.alarm0 == enemy.alarms.get(0) && native.alarm1 == enemy.alarms.get(1),
                    "live enemy alarm slots match exactly across compact and full stores");
        if (enemy.objectId == 116 && enemy.active && native.active)
            require(native.batActivity.initialized == enemy.batActivity.initialized &&
                    native.batActivity.flying == enemy.batActivity.flying &&
                    native.batActivity.hang == enemy.batActivity.hang &&
                    native.batHit.pending == enemy.batHit.pending &&
                    native.batHit.ready == enemy.batHit.ready &&
                    native.batHit.stunned == enemy.batHit.stunned &&
                    native.batHit.direction == enemy.batHit.direction,
                    "surviving bat hit state matches across both adapters");
        if (enemy.objectId == 116 && enemy.active && native.active) {
            const bool equal = native.preciseX == std::lround((enemy.preciseX - 240) * 256) &&
                    native.preciseY == std::lround((enemy.preciseY - 176) * 256) &&
                    native.speedX == std::lround(enemy.velocityX * 256) &&
                    native.speedY == std::lround(enemy.velocityY * 256) &&
                    native.direction == static_cast<int>(enemy.direction) &&
                    native.facing == (enemy.scaleX < 0 ? -1 : 1);
            if (!equal) std::cerr << "BAT ordinal=" << ordinal << " native="
                << native.preciseX << ',' << native.preciseY << ',' << native.speedX << ',' << native.speedY
                << ',' << native.direction << ',' << native.facing << " SDL="
                << std::lround((enemy.preciseX - 240) * 256) << ',' << std::lround((enemy.preciseY - 176) * 256)
                << ',' << std::lround(enemy.velocityX * 256) << ',' << std::lround(enemy.velocityY * 256)
                << ',' << enemy.direction << ',' << enemy.scaleX << " flying=" << native.batActivity.flying
                << " hang=" << native.batActivity.hang << " RNG=" << amiga.randomDrawCount()
                << ',' << reference.randomDrawCount() << " target="
                << amiga.player().frameStartXFixed() << ',' << amiga.player().frameStartYFixed()
                << '/' << reference.mainEnemyTargetX() << ',' << reference.mainEnemyTargetY() << '\n';
            require(equal, "bat collision and flight state match exactly across adapters");
        }
        if ((enemy.objectId == 111 || enemy.objectId == 112) && enemy.active && native.active) {
            const auto& a = native.walker;
            const auto& b = enemy.walker;
            require(a.x == b.x - 240L * 1280 && a.y == b.y - 176L * 1280 &&
                    a.vx == b.vx && a.vy == b.vy && a.facing == b.facing &&
                    a.grounded == b.grounded && a.publishedX == b.publishedX - 240 &&
                    a.publishedY == b.publishedY - 176,
                    "shared walking-enemy state matches exactly across adapters");
        }
        if (enemy.objectId == 117 && enemy.active && native.active) {
            const auto& a = native.snail;
            const auto& b = enemy.snail;
            require(a.x == b.x - 240L * 1280 && a.y == b.y - 176L * 1280 &&
                    a.vy == b.vy && a.facing == b.facing && a.verticalFlip == b.verticalFlip &&
                    a.attached == b.attached,
                    "shared snail motion and orientation match exactly across adapters");
            require(native.snailHit.pending == enemy.snailHit.pending &&
                    native.snailHit.ready == enemy.snailHit.ready &&
                    native.snailHit.stunned == enemy.snailHit.stunned &&
                    native.snailHit.alarm2 == enemy.snailHit.alarm2,
                    "shared snail hit response agrees across adapters");
        }
        if (enemy.objectId == 119 && enemy.active && native.active) {
            const auto& a = native.crawler;
            const auto& b = enemy.crawler;
            require(a.x == b.x - 240L * 1280 && a.y == b.y - 176L * 1280 &&
                    a.vx == b.vx && a.vy == b.vy && a.direction == b.direction &&
                    a.latched == b.latched && native.alarm0 == enemy.alarms.get(0),
                    "shared crawler state and alarm match exactly across adapters");
        }
        if (enemy.objectId != 113 || !enemy.active || !native.active) continue;
        if (std::getenv("DOWNWELL_TRACE_FROG_MOTION") && amiga.tick() >= 474)
            std::cerr << "FROG RAW tick=" << amiga.tick()
                      << " nativeY=" << native.preciseY << '+' << unsigned(native.gravityPositionRemainder) << "/5"
                      << " nativeVY=" << native.speedY << '+' << unsigned(native.gravityRemainder) << "/5"
                      << " SDL Y=" << (enemy.preciseY - 176.0F) * 256.0F
                      << " VY=" << enemy.velocityY * 256.0F
                      << " ground=" << native.grounded << ',' << enemy.grounded
                      << " alarm=" << native.alarm0 << ',' << enemy.alarms.get(0) << '\n';
        require(native.objectId == 113, "frog diagnostic pairs the same object type");
        const long referenceX = static_cast<long>(std::round((enemy.preciseX - 240.0F) * 256.0F));
        const long referenceY = static_cast<long>(std::round((enemy.preciseY - 176.0F) * 256.0F));
        const long referenceVX = static_cast<long>(std::round(enemy.velocityX * 256.0F));
        const long referenceVY = static_cast<long>(std::round(enemy.velocityY * 256.0F));
        const long nativeY = static_cast<long>(std::round(native.preciseY + native.gravityPositionRemainder / 5.0));
        const long nativeVY = static_cast<long>(std::round(native.speedY + native.gravityRemainder / 5.0));
        const bool matching = native.preciseX == referenceX && nativeY == referenceY &&
            native.speedX == referenceVX && nativeVY == referenceVY &&
            native.frogHorizontal.position == enemy.frogHorizontal.position &&
            native.frogHorizontal.velocity == enemy.frogHorizontal.velocity &&
            native.preciseY * 5 + native.gravityPositionRemainder == enemy.frogVertical.position &&
            native.speedY * 5 + native.gravityRemainder == enemy.frogVertical.velocity &&
            native.grounded == enemy.frogVertical.grounded &&
            native.frogHit.pending == enemy.frogHit.pending &&
            native.frogHit.ready == enemy.frogHit.ready &&
            native.frogHit.remaining == enemy.frogHit.remaining;
        if (!matching)
            std::cerr << "FROG MOTION tick=" << amiga.tick() << " actor=" << ordinal - 1
                      << " Q8 native=" << native.preciseX << ',' << nativeY << ',' << native.speedX << ',' << nativeVY
                      << " SDL=" << referenceX << ',' << referenceY << ',' << referenceVX << ',' << referenceVY << '\n';
        require(matching, "frog adapters must agree at Q8 precision, not within a one-pixel allowance");
        if (enemy.frogHit.remaining != 0) {
            sawPairedFrogHit = true;
            require(native.spriteId == 173 && enemy.spriteId == 173,
                    "both adapters select the source damage sprite during hit stun");
        }
    }
}

void compareInitialGameplay(const AmigaPortRuntime& amiga,
                            const downwell::StartupRuntime& reference,
                            float playerYTolerance = 0.0F,
                            float cameraYTolerance = 2.0F,
                            bool allowGroundingPhase = false) {
    require(reference.inMainRoom(),
            "reference remains in gameplay during comparison");
    verifyFrogMotion(amiga, reference);
    require(amiga.generationSucceeded(),
            "live terrain generation succeeds throughout the comparison tape");
    unsigned referenceEnemyCount = 0;
    for (unsigned i = 0; i < reference.activeInstanceCount(); ++i)
        if (amigaEnemyHealth(reference.instanceAt(i).objectId)) ++referenceEnemyCount;
    require(referenceEnemyCount == amiga.enemyCount(),
            "all enemy births occur on matching Step frames throughout this tape");
    if (std::getenv("DOWNWELL_TRACE_BIRTHS")) {
        static unsigned nativeSeen = 0, referenceSeen = 0;
        const unsigned count = referenceEnemyCount;
        if (nativeSeen != amiga.enemyCount() || referenceSeen != count) {
            std::cout << "ACTOR BIRTH tick=" << amiga.tick()
                      << " native=" << amiga.enemyCount() << " SDL=" << count
                      << " cameras=" << 34 + amiga.cameraOffsetY() << ','
                      << reference.mainCameraViewY() << '\n';
            nativeSeen = amiga.enemyCount();
            referenceSeen = count;
        }
    }
    // Counts above must match on every sampled frame; compare actors in
    // recovered creation order, never substitute a different ordinal.
    unsigned comparedOrdinal = 0;
    for (unsigned i = 0; i < reference.activeInstanceCount(); ++i) {
        const auto& enemy = reference.instanceAt(i);
        if (!amigaEnemyHealth(enemy.objectId)) continue;
        if (enemy.active && enemy.imageIndex >= 2.0F)
            sawExtendedEnemyAnimation = true;
        if (comparedOrdinal < amiga.enemyCount()) {
            const auto& native = amiga.enemyAt(comparedOrdinal);
            if (std::getenv("DOWNWELL_TRACE_ANIMATION")) {
                static bool reported[120] = {};
                if (!reported[enemy.objectId] && enemy.active && native.active &&
                    (native.spriteId != enemy.spriteId ||
                     native.imageFrame != static_cast<unsigned>(enemy.imageIndex))) {
                    reported[enemy.objectId] = true;
                    std::cout << "ANIMATION tick=" << amiga.tick() << " object=" << enemy.objectId
                              << " native=" << native.spriteId << ':' << native.imageFrame
                              << " SDL=" << enemy.spriteId << ':' << enemy.imageIndex
                              << " grounded=" << native.grounded << ',' << enemy.grounded
                              << " y=" << 176 + native.preciseY / 256.0F << ',' << enemy.preciseY << '\n';
                }
            }
            require(native.objectId == enemy.objectId,
                    "paired actor identity matches throughout live comparison");
            if (enemy.objectId == 116 && enemy.active && native.active) {
                require(240.0F + native.preciseX / 256.0F == enemy.preciseX &&
                        176.0F + native.preciseY / 256.0F == enemy.preciseY &&
                        native.direction == static_cast<int>(enemy.direction) &&
                        native.speedX / 256.0F == enemy.velocityX &&
                        native.speedY / 256.0F == enemy.velocityY,
                        "active bat adapters preserve exact shared state on the comparison tape");
            }
            if (enemy.active && native.active)
                require(native.grounded == enemy.grounded && native.spriteId == enemy.spriteId &&
                            native.imageFrame == static_cast<unsigned>(enemy.imageIndex),
                        "all paired active actors match support, sprite and animation frame");
            if (enemy.objectId == 113 && enemy.active && native.active) {
                require(native.frogLanded == enemy.frogLanded,
                        "frog landing latch matches on the live tape");
                require(native.grounded == enemy.grounded && native.spriteId == enemy.spriteId &&
                            native.imageFrame == static_cast<unsigned>(enemy.imageIndex),
                        "frog support, sprite and animation frame match on the live tape");
                sawPairedFrogGround = sawPairedFrogGround || enemy.grounded;
                sawPairedFrogAir = sawPairedFrogAir || !enemy.grounded;
            }
            requireNear(240 + native.preciseX / 256.0F, enemy.preciseX, 1.0F,
                        "paired actor horizontal trajectory agrees across targets");
            requireNear(176 + native.preciseY / 256.0F, enemy.preciseY, 1.0F,
                        "paired actor vertical trajectory agrees within Q8 integration error");
            if (enemy.objectId == 113 && enemy.active && enemy.frogHit.remaining != 0)
                require(enemy.spriteId == 173, "frog hit stun takes priority over flight presentation");
            else if (enemy.objectId == 113 && enemy.active && !enemy.grounded)
                require(enemy.spriteId == 168 &&
                            enemy.imageIndex == (enemy.velocityY < 0 ? 0.0F : 1.0F),
                        "SDL airborne frog uses the recovered rise/fall pose");
        }
        ++comparedOrdinal;
    }
    if (std::getenv("DOWNWELL_TRACE_ACTORS")) {
        static bool reported = false;
        unsigned ordinal = 0;
        for (unsigned i = 0; !reported && i < reference.activeInstanceCount(); ++i) {
            const auto& e = reference.instanceAt(i);
            if (!amigaEnemyHealth(e.objectId)) continue;
            if (ordinal < amiga.enemyCount()) {
                const auto& a = amiga.enemyAt(ordinal);
                const float x = 240 + a.preciseX / 256.0F, y = 176 + a.preciseY / 256.0F;
                if (std::fabs(x - e.preciseX) > 1.0F || std::fabs(y - e.preciseY) > 1.0F) {
                    std::cout << "FIRST ACTOR DIFFERENCE tick=" << amiga.tick() << " ordinal=" << ordinal
                        << " native=" << x << ',' << y << " velocity=" << a.speedX << ',' << a.speedY
                        << " ground=" << a.grounded << " SDL=" << e.preciseX << ',' << e.preciseY
                        << " velocity=" << e.velocityX << ',' << e.velocityY << " ground=" << e.grounded << '\n';
                    reported = true;
                }
            }
            ++ordinal;
        }
    }
    requireNear(240.0F + amiga.player().offsetXFixed() / 256.0F,
                reference.mainPlayerX(), 0.0F,
                "both backends execute identical fixed-point horizontal player state");
    requireNear(176.0F + amiga.player().offsetYFixed() / 256.0F,
                reference.mainPlayerY(), playerYTolerance,
                "gameplay player Y follows decoded runtime");
    requireNear(static_cast<float>(160 + amiga.cameraOffsetX()),
                reference.mainCameraViewX(), 1.0F,
                "gameplay camera X follows decoded runtime");
    const float amigaCameraY = static_cast<float>(34 + amiga.cameraOffsetY());
    if (std::fabs(amigaCameraY - reference.mainCameraViewY()) >
        cameraYTolerance) {
        std::cerr << "FAILED: gameplay camera Y follows decoded runtime "
                  << "(Amiga camera " << amigaCameraY << ", reference camera "
                  << reference.mainCameraViewY() << ", Amiga player "
                  << 176 + amiga.player().offsetY() << ", reference player "
                  << reference.mainPlayerY() << ", parity frame "
                  << parityFrame << ")\n";
        std::exit(1);
    }
    require(amiga.player().facing() == reference.mainPlayerFacing(),
            "gameplay facing follows decoded runtime");
    if (amiga.player().grounded() != reference.mainPlayerGrounded() &&
        !(allowGroundingPhase &&
          std::fabs(static_cast<float>(176 + amiga.player().offsetY()) -
                    reference.mainPlayerY()) <= playerYTolerance)) {
        std::cerr << "FAILED: gameplay grounded state follows decoded runtime "
                  << "(Amiga " << amiga.player().grounded()
                  << " at " << 240 + amiga.player().offsetX() << ","
                  << 176 + amiga.player().offsetY()
                  << ", reference " << reference.mainPlayerGrounded()
                  << " at " << reference.mainPlayerX() << ","
                  << reference.mainPlayerY()
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
    for (int object : {111, 112, 113, 116, 117, 119}) {
        unsigned mask = 0;
        for (int alarm = 0; alarm < 12; ++alarm) {
            downwell::PackedObjectEvent event;
            if (objects.findEvent(object, 2, alarm, event)) mask |= 1u << alarm;
        }
        require(mask == downwell::enemyAlarmEvents(object),
                "compact native enemy alarm masks match recovered inherited event metadata");
    }
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

    AmigaPortRuntime forecast(300, false, 1201 - 176, true);
    downwell::PackedRoom terrainRoom;
    AmigaTerrainFragment terrain[amigaInitialTerrainFragmentCount];
    AmigaTerrainEnemy generatedEnemies[AmigaPortRuntime::enemyCapacity];
    AmigaTerrainDecoration
        generatedDecorations[amigaTerrainDecorationCapacity];
    unsigned generatedEnemyCount = 0;
    unsigned generatedDecorationCount = 0;
    downwell::GameMakerRandom gameplayRandom(2);
    int breakableX[amigaTerrainBreakableCapacity];
    int breakableY[amigaTerrainBreakableCapacity];
    require(rooms.findRoom(11, terrainRoom) &&
                amigaBuildInitialAreaOneTerrain(
                    patterns, terrain, amigaInitialTerrainFragmentCount,
                    generatedEnemies, AmigaPortRuntime::enemyCapacity,
                    generatedEnemyCount, &gameplayRandom,
                    generatedDecorations, amigaTerrainDecorationCapacity,
                    &generatedDecorationCount),
            "native forecast terrain is recovered for parity");
    bool sawPlant = false;
    bool sawSpecial = false;
    for (unsigned index = 0; index < generatedDecorationCount; ++index) {
        const AmigaTerrainDecoration& decoration =
            generatedDecorations[index];
        if (decoration.objectId == 289) {
            sawPlant = true;
            require(decoration.spriteId == 550 &&
                        decoration.frame >= 0 && decoration.frame <= 16,
                    "native plants retain recovered sprite frames");
        } else {
            sawSpecial = true;
            require(decoration.objectId == 293 ||
                        decoration.objectId == 296 ||
                        decoration.objectId == 109 ||
                        decoration.objectId == 306,
                    "native decoration list contains SDL-visible objects");
        }
    }
    require(generatedDecorationCount > 0 &&
                generatedDecorationCount <= amigaTerrainDecorationCapacity &&
                sawPlant && sawSpecial,
            "complete recovered decoration population fits native storage");
    const unsigned breakableCount = amigaCollectTerrainBreakables(
        patterns, terrain, amigaInitialTerrainFragmentCount,
        breakableX, breakableY, amigaTerrainBreakableCapacity);
    require(breakableCount == 124,
            "all recovered area-one breakables fit native storage");
    AmigaTerrainCollisionContext terrainContext = {
        &rooms, &terrainRoom, &patterns, terrain,
        amigaInitialTerrainFragmentCount, &forecast
    };
    forecast.configureBreakables(breakableX, breakableY, breakableCount);
    static AmigaTerrainIndex index;
    require(amigaBuildTerrainIndex(terrainContext, index), "terrain fits spatial index");
    auto indexedContext = terrainContext;
    indexedContext.index = &index;
    for (int y = -16; y < 4200; y += 7)
        for (int x = -24; x < 520; x += 7)
            require(amigaResolveTerrainSolid(&terrainContext, x - 240, y - 176) ==
                    amigaResolveTerrainSolid(&indexedContext, x - 240, y - 176),
                    "indexed solid queries match serialized oracle");
    for (int y = -16; y < 4200; y += 31)
        for (int x = -24; x < 520; x += 11)
            require(amigaResolveTerrainLanding(&terrainContext, x - 240, y - 176) ==
                    amigaResolveTerrainLanding(&indexedContext, x - 240, y - 176),
                    "indexed landing queries match serialized oracle");
    terrainContext.index = &index;
    forecast.configureLandingResolver(amigaResolveTerrainLanding,
                                      &terrainContext);
    forecast.configureSolidResolver(amigaResolveTerrainSolid,
                                    &terrainContext);
    forecast.configureEnemyRandom(gameplayRandom);
    require(generatedEnemyCount > 3 &&
                generatedEnemyCount <= AmigaPortRuntime::enemyCapacity,
            "complete recovered enemy population fits native storage");
    for (unsigned enemyIndex = 0;
         enemyIndex < generatedEnemyCount; ++enemyIndex) {
        downwell::PackedObject definition;
        const AmigaTerrainEnemy& enemy = generatedEnemies[enemyIndex];
        require(objects.findObject(enemy.objectId, definition) &&
                    forecast.configureEnemy(
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
    AmigaPortRuntime gameplay(300, false, 1201 - 176, true);
    gameplay.initializeEntryInput(false);
    int builderOrder = -1, cameraOrder = -1;
    for (unsigned i = 0; i < reference.activeInstanceCount(); ++i) {
        downwell::PackedObjectEvent event;
        if (!objects.findEvent(reference.instanceAt(i).objectId, 3, 0, event)) continue;
        if (event.codeId == 1003) builderOrder = static_cast<int>(i);
        if (event.codeId == 999) cameraOrder = static_cast<int>(i);
    }
    require(builderOrder >= 0 && cameraOrder > builderOrder,
            "recovered rmMain ordinary Step order places builder before camera");
    LiveSceneWorld live(patterns, objects);
    live.bind(gameplay, rooms, terrainRoom);
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
    require(gameplay.player().spriteId(gameplay.deathSequence().movementDead()) == reference.mainPlayerSpriteId() &&
                gameplay.player().spriteFrame(gameplay.deathSequence().movementDead()) ==
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

    /*
     * Keep walking off the recovered entrance platform.  This extends the
     * parity tape beyond the first safe landing into real camera scrolling
     * and the first generated actor activation frontier.
     */
    bool sawAwakeEnemy = false;
    const int postLandingCameraY = gameplay.cameraOffsetY();
    for (int tick = 0; tick < 600; ++tick) {
        parityFrame += 1;
        gameplay.step(downwell::packReplayInput(right));
        reference.step(right, false);
        compareInitialGameplay(gameplay, reference, 10.0F, 10.0F, true);
        for (unsigned enemy = 0; enemy < gameplay.enemyCount(); ++enemy)
            sawAwakeEnemy = sawAwakeEnemy || gameplay.enemyAt(enemy).awake;
        if (sawAwakeEnemy &&
            gameplay.cameraOffsetY() >= postLandingCameraY + 256)
            break;
    }
    require(gameplay.cameraOffsetY() >= postLandingCameraY + 256,
            "post-landing tape scrolls at least one circular-buffer page");
    require(sawAwakeEnemy,
            "post-landing tape reaches generated native enemy simulation");
    unsigned retiredCompared = 0, enemyOrdinal = 0;
    for (unsigned i = 0; i < reference.activeInstanceCount(); ++i) {
        const auto& enemy = reference.instanceAt(i);
        if (enemy.objectId != 111 && enemy.objectId != 112 && enemy.objectId != 113 &&
            enemy.objectId != 116 && enemy.objectId != 117 && enemy.objectId != 119) continue;
        require(enemyOrdinal < gameplay.enemyCount() &&
                gameplay.enemyAt(enemyOrdinal).objectId == enemy.objectId,
                "generated enemy prefix preserves creation order across targets");
        const auto& native = gameplay.enemyAt(enemyOrdinal++);
        if (!enemy.active && enemy.health > 0 &&
            enemy.y < reference.mainCameraViewY() - 160) {
            require(!enemy.activationPending,
                    "retired SDL enemies are not eligible for later region activation");
            require(!native.active && !native.awake && !native.activationPending,
                    "SDL-retired enemies no longer remain live in Amiga");
            ++retiredCompared;
        }
    }
    require(retiredCompared >= 2, "comparison exercises two out-of-view enemy retirements");
    require(sawExtendedEnemyAnimation,
            "SDL live tape actually displays enemy frames beyond the old two-frame cap");
    require(sawPairedFrogAir && sawPairedFrogGround,
            "frog presentation comparison covers both flight and landing");
    require(enemyOrdinal == gameplay.enemyCount(),
            "live generation creates exactly the SDL enemy prefix, not a forecast population");
    require(live.builder.fragmentCount() > 0 &&
                live.builder.fragmentCount() < amigaInitialTerrainFragmentCount,
            "comparison exercises incremental terrain rather than a complete forecast");
    if (std::getenv("DOWNWELL_TRACE_COMBAT") || std::getenv("DOWNWELL_VERIFY_COMBAT")) {
        bool reported = false;
        bool playerReported = false;
        unsigned pairedDeadFrames = 0;
        const unsigned combatTicks = std::getenv("DOWNWELL_COMBAT_TICKS")
            ? static_cast<unsigned>(std::atoi(std::getenv("DOWNWELL_COMBAT_TICKS"))) : 600;
        require(combatTicks >= 600 && combatTicks <= 6000,
                "combat continuation is bounded to 600..6000 updates");
        for (unsigned tick = 0; tick < combatTicks; ++tick) {
            if (tick == 0) {
                std::cout << "COMBAT START tick=" << gameplay.tick()
                          << " native=" << 240 + gameplay.player().offsetX() << ',' << 176 + gameplay.player().offsetYFixed() / 256.0F
                          << " SDL=" << reference.mainPlayerX() << ',' << reference.mainPlayerY()
                          << " ground=" << gameplay.player().grounded() << ',' << reference.mainPlayerGrounded() << '\n';
                for (unsigned i = 0; i < reference.activeInstanceCount(); ++i) {
                    const auto& wall = reference.instanceAt(i);
                    if (wall.active && (wall.objectId == 60 || wall.objectId == 92 || wall.objectId == 93) &&
                        std::fabs(wall.x - reference.mainPlayerX()) < 24 &&
                        std::fabs(wall.y - reference.mainPlayerY()) < 32)
                        std::cout << "NEARBY SOLID " << wall.objectId << ' ' << wall.x << ',' << wall.y << '\n';
                }
            }
            downwell::Input input;
            input.left = (tick / 90) % 2 == 0;
            input.right = !input.left;
            // Offset the firing cycle to retain a surviving-frog hit after
            // replacing oversized provisional boxes with recovered masks.
            const int firePhase = std::getenv("DOWNWELL_COMBAT_FIRE_PHASE")
                ? std::atoi(std::getenv("DOWNWELL_COMBAT_FIRE_PHASE")) : 1;
            input.fire = (tick + firePhase) % 10 < 5;
            const bool bothWereDead = gameplay.deathSequence().movementDead() && reference.mainDeathSequence().movementDead();
            const auto beforeNativeTime = gameplay.runTimeTicks();
            const auto beforeHostTime = reference.runTimeTicks();
            const bool nativePublishedDeath = gameplay.deathSequence().active;
            const bool hostPublishedDeath = reference.mainDeathSequence().active;
            const unsigned gemsBeforeNative = gameplay.gameGemTotal();
            const unsigned gemsBeforeHost = reference.mainGameGemTotal();
            const bool nativeWasRecorded = gameplay.runGemStatistics().recorded;
            const bool hostWasRecorded = reference.runGemStatistics().recorded;
            const unsigned nativeShots = gameplay.player().shots(), hostShots = reference.mainShotsFired();
            const unsigned nativeJumps = gameplay.player().jumps(), hostJumps = reference.mainJumps();
            gameplay.step(downwell::packReplayInput(input));
            reference.step(input, input.fire && (tick + firePhase) % 10 == 0);
            const auto& nativeDeath = gameplay.deathSequence();
            const auto& hostDeath = reference.mainDeathSequence();
            require(gameplay.runTimeTicks() == reference.runTimeTicks() &&
                        gameplay.runTimeTicks() == beforeNativeTime + (nativePublishedDeath ? 0u : 1u) &&
                        reference.runTimeTicks() == beforeHostTime + (hostPublishedDeath ? 0u : 1u),
                    "shared run clock agrees and uses pre-publication death on every update");
            const auto& nativeStats = gameplay.runGemStatistics();
            const auto& hostStats = reference.runGemStatistics();
            const auto& nativeResults = gameplay.deathResults();
            const auto& hostResults = reference.deathResults();
            downwell::StateDigest nativeResultsDigest, hostResultsDigest;
            nativeResults.appendState(nativeResultsDigest);
            hostResults.appendState(hostResultsDigest);
            require(nativeResultsDigest.value() == hostResultsDigest.value(),
                    "complete results state and emitted effects agree on every combat update");
            require(nativeResults.active == nativeDeath.menuReady && hostResults.active == hostDeath.menuReady &&
                        !nativeResults.failed && !hostResults.failed &&
                        nativeResults.menu.resultShown == hostResults.menu.resultShown &&
                        nativeResults.menu.cursor == hostResults.menu.cursor &&
                        nativeResults.menu.sequenceOver == hostResults.menu.sequenceOver &&
                        nativeResults.menu.inputAlarm == hostResults.menu.inputAlarm &&
                        nativeResults.menu.revealAlarm == hostResults.menu.revealAlarm &&
                        nativeResults.progress.animatedTotal == hostResults.progress.animatedTotal &&
                        nativeResults.frame.displayedTotal == hostResults.frame.displayedTotal &&
                        nativeResults.inputEffects.action == hostResults.inputEffects.action,
                    "live results controllers agree throughout death and continued input");
            if (!nativeWasRecorded && nativeStats.recorded)
                require(nativeStats.savedRunGems == gemsBeforeNative,
                        "native stats snapshot precedes this update's gem collection");
            if (!hostWasRecorded && hostStats.recorded)
                require(hostStats.savedRunGems == gemsBeforeHost,
                        "host stats snapshot precedes this update's gem collection");
            require(nativeStats.recorded == nativeDeath.started && hostStats.recorded == hostDeath.started &&
                        nativeStats.lifetime == hostStats.lifetime && nativeStats.record == hostStats.record &&
                        nativeStats.savedRunGems == hostStats.savedRunGems,
                    "death-entry gem statistics agree across adapters and latch only on movement entry");
            require(gameplay.playerVisible() == reference.mainPlayerVisible(),
                    "both backends expose the same damaged-player visibility on every combat update");
            require(nativeDeath.active == hostDeath.active && nativeDeath.frames == hostDeath.frames &&
                        nativeDeath.animation == hostDeath.animation && nativeDeath.alarm == hostDeath.alarm &&
                        nativeDeath.skipPresses == hostDeath.skipPresses && nativeDeath.menuReady == hostDeath.menuReady,
                    "death sequence state agrees across adapters on every combat update");
            if (bothWereDead) {
                ++pairedDeadFrames;
                require(gameplay.dead() && gameplay.health() == 0 && reference.mainPlayerHealth() == 0 &&
                            gameplay.player().shots() == nativeShots && reference.mainShotsFired() == hostShots &&
                            gameplay.player().jumps() == nativeJumps && reference.mainJumps() == hostJumps,
                        "depleted-health players cannot fire or jump during continued input");
            }
            if (std::getenv("DOWNWELL_TRACE_SNAKE") && gameplay.enemyCount() > 4 && gameplay.tick() >= 585 && gameplay.tick() <= 601) {
                const auto& native = gameplay.enemyAt(4);
                unsigned ordinal = 0;
                for (unsigned i = 0; i < reference.activeInstanceCount(); ++i) {
                    const auto& enemy = reference.instanceAt(i);
                    if (enemy.objectId != 111 && enemy.objectId != 112 && enemy.objectId != 113 &&
                        enemy.objectId != 116 && enemy.objectId != 117 && enemy.objectId != 119) continue;
                    if (ordinal++ != 4) continue;
                    std::cout << "SNAKE " << gameplay.tick()
                              << " x=" << 240 + native.preciseX / 256.0F << ',' << enemy.preciseX
                              << " y=" << 176 + native.preciseY / 256.0F << ',' << enemy.preciseY
                              << " vx=" << native.speedX / 256.0F << ',' << enemy.velocityX
                              << " ground=" << native.grounded << ',' << enemy.grounded << '\n';
                }
                for (unsigned i = 0; i < gameplay.bulletCapacity; ++i) {
                    const auto& bullet = gameplay.bulletAt(i);
                    if (bullet.active) std::cout << "BULLET N " << bullet.x + 240 << ',' << bullet.y + 176 << '\n';
                }
                for (unsigned i = 0; i < reference.mainBulletCount(); ++i) {
                    const auto& bullet = reference.mainBulletAt(i);
                    std::cout << "BULLET S " << bullet.x << ',' << bullet.y << '\n';
                }
            }
            if (std::getenv("DOWNWELL_TRACE_PLAYER_STEPS") && gameplay.tick() >= 550 && gameplay.tick() <= 605)
                std::cout << "PLAYER STEP " << gameplay.tick()
                          << " x=" << 240 + gameplay.player().offsetX() << ',' << reference.mainPlayerX()
                          << " y=" << 176 + gameplay.player().offsetYFixed() / 256.0F << ',' << reference.mainPlayerY()
                          << " ground=" << gameplay.player().grounded() << ',' << reference.mainPlayerGrounded()
                          << " shots=" << gameplay.player().shots() << ',' << reference.mainShotsFired() << '\n';
            require(gameplay.generationSucceeded(), "combat continuation retains successful live generation");
            require(gameplay.bulletCount() == reference.mainBulletCount(),
                    "combat continuation retains the same live bullet count");
            unsigned bulletOrdinal = 0;
            for (unsigned i = 0; i < gameplay.bulletCapacity; ++i) {
                const auto& bullet = gameplay.bulletAt(i);
                if (!bullet.active) continue;
                const auto& hostBullet = reference.mainBulletAt(bulletOrdinal++);
                requireNear(bullet.x + 240, hostBullet.x, 1.0F,
                            "combat bullet X matches in creation order");
                requireNear(bullet.y + 176, hostBullet.y, 3.0F,
                            "combat bullet Y matches in creation order");
            }
            verifyFrogMotion(gameplay, reference);
            if (std::getenv("DOWNWELL_TRACE_GEM_MOTION")) {
                {
                    for (unsigned i = 0; i < gameplay.enemyCount(); ++i) {
                        const auto& e = gameplay.enemyAt(i);
                        if (e.active) std::cout << "GE N " << gameplay.tick() << ' ' << i << ' ' << e.objectId
                            << ' ' << e.x << ',' << e.y << ' ' << e.preciseX << ',' << e.preciseY << '\n';
                    }
                    for (unsigned i = 0; i < reference.activeInstanceCount(); ++i) {
                        const auto& e = reference.instanceAt(i);
                        if (e.active && (e.objectId == 111 || e.objectId == 112 || e.objectId == 113 || e.objectId == 116 || e.objectId == 117 || e.objectId == 119))
                            std::cout << "GE S " << gameplay.tick() << ' ' << i << ' ' << e.objectId
                                << ' ' << e.x - 240 << ',' << e.y - 176 << ' '
                                << (e.preciseX - 240) * 256 << ',' << (e.preciseY - 176) * 256 << '\n';
                    }
                }
                std::cout << "GPLAYER " << gameplay.tick() << ' '
                          << gameplay.player().offsetXFixed() << ',' << gameplay.player().offsetYFixed()
                          << ' ' << (reference.mainPlayerX() - 240) * 256 << ','
                          << (reference.mainPlayerY() - 176) * 256 << '\n';
                for (unsigned i = 0; i < gameplay.gemCapacity; ++i) {
                    const auto& gem = gameplay.gemAt(i);
                    if (gem.active) std::cout << "GEM N " << gameplay.tick() << ' ' << gem.age
                        << ' ' << gem.motion.x << ',' << gem.motion.y << ' '
                        << gem.motion.vx << ',' << gem.motion.vy << ' ' << gem.attracted << '\n';
                }
                for (unsigned i = 0; i < reference.mainCurrencyCount(); ++i) {
                    const auto& gem = reference.mainCurrencyAt(i);
                    std::cout << "GEM S " << gameplay.tick() << ' ' << gem.age
                        << ' ' << gem.motion.x << ',' << gem.motion.y << ' '
                        << gem.motion.vx << ',' << gem.motion.vy << ' ' << gem.attracted << '\n';
                }
            }
            require(gameplay.currency() == reference.mainCurrencyTotal() &&
                        gameplay.gameGemTotal() == reference.mainGameGemTotal() &&
                        gameplay.gemStreak() == reference.mainGemStreak() &&
                        gameplay.gemStreakTimer() == reference.mainGemStreakTimer(),
                    "shared currency, run total, streak and timer agree on every combat frame");
            // Both adapters retain live gems in birth order. Compare that
            // order and authoritative movement, not a sorted multiset.
            std::vector<std::array<long, 9>> nativeGems, referenceGems;
            for (unsigned i = 0; i < gameplay.gemCapacity; ++i) {
                const auto& gem = gameplay.gemAt(i);
                if (gem.active) nativeGems.push_back({static_cast<long>(gem.age), gem.amount,
                    gem.motion.animation.rateTenths, static_cast<long>(gem.motion.animation.phaseTenths),
                    gem.motion.x, gem.motion.y, gem.motion.vx, gem.motion.vy, gem.motion.attracted});
            }
            for (unsigned i = 0; i < reference.mainCurrencyCount(); ++i) {
                const auto& gem = reference.mainCurrencyAt(i);
                referenceGems.push_back({static_cast<long>(gem.age), gem.amount,
                    gem.motion.animation.rateTenths, static_cast<long>(gem.motion.animation.phaseTenths),
                    gem.motion.x, gem.motion.y, gem.motion.vx, gem.motion.vy, gem.motion.attracted});
            }
            if (nativeGems != referenceGems)
                std::cerr << "COMBAT GEM DIFFERENCE tick=" << gameplay.tick()
                          << " firePhase=" << firePhase
                          << " count=" << nativeGems.size() << ',' << referenceGems.size()
                          << " currency=" << gameplay.currency() << ',' << reference.mainCurrencyTotal()
                          << " kills=" << gameplay.enemiesDefeated() << ',' << reference.mainEnemiesDefeated()
                          << '\n';
            require(nativeGems == referenceGems,
                    "live currency birth order, motion and animation agree exactly across adapters");
            sawPairedCurrencyAnimation = sawPairedCurrencyAnimation || !nativeGems.empty();
            if (gameplay.player().spriteId(gameplay.deathSequence().movementDead()) != reference.mainPlayerSpriteId() ||
                    gameplay.player().spriteFrame(gameplay.deathSequence().movementDead()) != static_cast<unsigned>(reference.mainPlayerImageIndex()))
                std::cerr << "PRESENTATION tick=" << gameplay.tick()
                          << " health=" << gameplay.health() << ',' << reference.mainPlayerHealth()
                          << " sprite=" << gameplay.player().spriteId(gameplay.deathSequence().movementDead()) << ',' << reference.mainPlayerSpriteId()
                          << " frame=" << gameplay.player().spriteFrame(gameplay.deathSequence().movementDead()) << ',' << reference.mainPlayerImageIndex()
                          << " y=" << 176 + gameplay.player().offsetYFixed() / 256.0F << ',' << reference.mainPlayerY()
                          << '\n';
            require(gameplay.player().spriteId(gameplay.deathSequence().movementDead()) == reference.mainPlayerSpriteId() &&
                    gameplay.player().spriteFrame(gameplay.deathSequence().movementDead()) ==
                        static_cast<unsigned>(reference.mainPlayerImageIndex()),
                    "combat player presentation agrees across adapters, including forced spins");
            if (!playerReported && std::fabs(176 + gameplay.player().offsetYFixed() / 256.0F - reference.mainPlayerY()) > 3.0F) {
                playerReported = true;
                std::cout << "COMBAT PLAYER tick=" << gameplay.tick()
                          << " y=" << 176 + gameplay.player().offsetYFixed() / 256.0F << ',' << reference.mainPlayerY()
                          << " ground=" << gameplay.player().grounded() << ',' << reference.mainPlayerGrounded()
                          << " jumps=" << gameplay.player().jumps() << ',' << reference.mainJumps()
                          << " shots=" << gameplay.player().shots() << ',' << reference.mainShotsFired()
                          << " ammo=" << gameplay.player().ammo() << ',' << reference.mainAmmo() << '\n';
            }
            if (!reported && (gameplay.health() != reference.mainPlayerHealth() ||
                    gameplay.enemiesDefeated() != reference.mainEnemiesDefeated() ||
                    gameplay.currency() != reference.mainCurrencyTotal())) {
                std::cout << "COMBAT DIFFERENCE tick=" << gameplay.tick()
                          << " health=" << gameplay.health() << ',' << reference.mainPlayerHealth()
                          << " kills=" << gameplay.enemiesDefeated() << ',' << reference.mainEnemiesDefeated()
                          << " currency=" << gameplay.currency() << ',' << reference.mainCurrencyTotal()
                          << " player=" << 240 + gameplay.player().offsetX() << ',' << 176 + gameplay.player().offsetY()
                          << " SDL=" << reference.mainPlayerX() << ',' << reference.mainPlayerY() << '\n';
                reported = true;
                for (unsigned i = 0; i < gameplay.enemyCount(); ++i) {
                    const auto& enemy = gameplay.enemyAt(i);
                    std::cout << "COMBAT NATIVE ENEMY " << i << ' ' << enemy.objectId
                              << " active=" << enemy.active << " hp=" << enemy.health
                              << " position=" << 240 + enemy.x << ',' << 176 + enemy.y
                              << " precise=" << 240 + enemy.preciseX / 256.0F << ','
                              << 176 + enemy.preciseY / 256.0F << '\n';
                }
                for (unsigned i = 0; i < reference.activeInstanceCount(); ++i) {
                    const auto& enemy = reference.instanceAt(i);
                    if (enemy.objectId == 111 || enemy.objectId == 112 || enemy.objectId == 113 ||
                        enemy.objectId == 116 || enemy.objectId == 117 || enemy.objectId == 119)
                        std::cout << "COMBAT SDL ENEMY " << enemy.objectId
                                  << " active=" << enemy.active << " hp=" << enemy.health
                                  << " position=" << enemy.x << ',' << enemy.y
                                  << " precise=" << enemy.preciseX << ',' << enemy.preciseY << '\n';
                }
            }
        }
        std::cout << "COMBAT END health=" << gameplay.health() << ',' << reference.mainPlayerHealth()
                  << " kills=" << gameplay.enemiesDefeated() << ',' << reference.mainEnemiesDefeated()
                  << " currency=" << gameplay.currency() << ',' << reference.mainCurrencyTotal() << '\n';
        require(sawPairedCurrencyAnimation, "combat tape actually covers live currency animation");
        require(gameplay.enemiesDefeated() > 0 && reference.mainEnemiesDefeated() > 0 &&
                    gameplay.currency() > 0 && reference.mainCurrencyTotal() > 0,
                "combat diagnostic actually exercises kills and reward collection on both targets");
        if (std::getenv("DOWNWELL_VERIFY_COMBAT"))
            require(!reported && !playerReported,
                    "extended combat tape must match movement, health, kills and currency");
        if (std::getenv("DOWNWELL_VERIFY_FROG_MOTION"))
            require(sawPairedFrogHit, "strict combat tape exercises a surviving frog hit on both adapters");
        if (std::getenv("DOWNWELL_VERIFY_DEATH"))
            require(pairedDeadFrames >= 120,
                    "death-state regression includes at least 120 paired depleted-health updates");
    }
    if (std::getenv("DOWNWELL_COMPARE_ACTORS")) {
        for (unsigned i = 0; i < gameplay.enemyCount(); ++i) {
            const auto& e = gameplay.enemyAt(i);
            std::cout << "AMIGA " << i << ' ' << e.objectId << ' ' << e.active << ' '
                      << e.awake << ' ' << e.x + 240 << ' ' << e.y + 176 << ' '
                      << e.health << ' ' << e.imageFrame << '\n';
        }
        unsigned ordinal = 0;
        for (unsigned i = 0; i < reference.activeInstanceCount(); ++i) {
            const auto& e = reference.instanceAt(i);
            if (e.objectId != 111 && e.objectId != 112 && e.objectId != 113 &&
                e.objectId != 116 && e.objectId != 117 && e.objectId != 119) continue;
            std::cout << "SDL " << ordinal++ << ' ' << e.objectId << ' ' << e.active << ' '
                      << e.x << ' ' << e.y << ' ' << e.health << ' ' << e.imageIndex << '\n';
        }
    }
}
}

int main(int argc, char** argv) {
    {
        AmigaSceneRuntime fade;
        for (int stage = 2; stage <= 5; ++stage)
            pressFire(fade);
        for (int tick = 0; tick < 59; ++tick)
            fade.step(downwell::Input{});
        require(fade.splashStage() == 6 && fade.splashDitherFrame() == 0,
                "native splash reaches the opaque dither endpoint");
        fade.step(downwell::Input{});
        require(fade.splashDitherFrame() == 1,
                "native splash reverses the recovered dither sequence");
    }
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
