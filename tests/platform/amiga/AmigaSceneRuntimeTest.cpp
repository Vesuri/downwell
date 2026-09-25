#include "platform/amiga/AmigaSceneRuntime.h"
#include "platform/amiga/AmigaPortRuntime.h"
#include "platform/amiga/AmigaTerrain.h"
#include "core/BuilderPatternPack.h"
#include "core/ObjectPack.h"
#include "core/EnemyAlarmEvents.h"
#include "core/Replay.h"
#include "core/RoomPack.h"
#include "core/StartupRuntime.h"
#include "core/GameMakerRounding.h"
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
        return world.runtime->createGeneratedInstance(command, world.objects,
            [&world](const downwell::DeferredCreateInstance& command) {
                return amigaAddTerrainObject(world.index, command.objectId, command.x, command.y);
            });
    }
    static bool generate(void* opaque, AmigaPortRuntime&,
                         downwell::GameMakerRandom& random, int bottom) {
        auto& world = *static_cast<LiveSceneWorld*>(opaque);
        downwell::AreaOneBuilderStep result;
        return world.builder.stepVisible(world.patterns, random, bottom, result);
    }
    void bind(AmigaPortRuntime& port, const downwell::RoomPack& rooms,
              const downwell::PackedRoom& room, bool continuing = false,
              const downwell::StartupRuntime* entry = nullptr) {
        runtime = &port;
        collision.index = nullptr; collision.runtime = nullptr; collision.fragmentCount = 0;
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
        if (entry) {
            require(builder.resetFromRoomEntry(entry->mainBuilderState()),
                    "live room entry imports generator before any terrain emission");
            port.configureEnemyRandom(entry->randomState());
        } else if (continuing) {
            require(port.initializeAreaOneBuilder(builder, -16, 704), "native continuation initializes the current level on its owned RNG");
        } else {
            downwell::GameMakerRandom random;
            builder.resetInitialCheckpoint(random);
            port.configureEnemyRandom(random);
        }
        builder.setCreateObserver(create, this);
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
                    native.batHit.direction == enemy.batHit.direction &&
                    native.batHit.pendingFraction == enemy.batHit.pendingFraction &&
                    native.batHit.fraction == enemy.batHit.fraction,
                    "surviving bat hit state matches across both adapters");
        if (enemy.objectId == 116 && enemy.active && native.active) {
            const bool equal = native.preciseX == std::lround((enemy.preciseX - 240) * 256) &&
                    native.preciseY == std::lround((enemy.preciseY - 176) * 256) &&
                    native.speedX == std::lround(enemy.velocityX * 256) &&
                    native.speedY == std::lround(enemy.velocityY * 256) &&
                    native.direction == static_cast<int>(enemy.direction) &&
                    native.batDirectionFraction == enemy.batDirectionFraction &&
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
            require(native.drawFrame() / 256 == (enemy.scaleY < 0 ? 1u : 0u),
                    "native offline snail frame uses the same stored vertical orientation as SDL");
            require(native.imageScaleY() == enemy.scaleY,
                    "native bullet/player contacts use SDL's stored snail image scale");
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
            require(a.imageDirection == b.imageDirection &&
                    enemy.rotation == a.imageDirection * 90 &&
                    native.drawFrame() / 256 == a.imageDirection % 4,
                    "both crawler renderers consume the Step-published angle");
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
                            const downwell::StartupRuntime& reference) {
    require(reference.inMainRoom(),
            "reference remains in gameplay during comparison");
    require(amiga.sideCameraDigest() == reference.sideCameraDigest(),
            "camera easing, side-lock state and room registrations match exactly");
    require(amiga.levelExitPresent() == reference.levelExitPresent(),
            "both adapters retain the same exit-field lifetime");
    if (amiga.levelExitPresent())
        require(amiga.goalField().radius.equal(reference.goalField().radius),
                "both live adapters publish the same once-per-update field radius");
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
            require(native.active == enemy.active && native.health == enemy.health,
                    "paired enemies agree on activation and remaining health");
            require(native.alarm0 == enemy.alarms.get(0) && native.alarm1 == enemy.alarms.get(1),
                    "paired enemies retain identical compact alarm countdowns");
            if (enemy.objectId == 116 && enemy.active && native.active) {
                require(240.0F + native.preciseX / 256.0F == enemy.preciseX &&
                        176.0F + native.preciseY / 256.0F == enemy.preciseY &&
                        native.direction == static_cast<int>(enemy.direction) &&
                        native.speedX / 256.0F == enemy.velocityX &&
                        native.speedY / 256.0F == enemy.velocityY,
                        "active bat adapters preserve exact shared state on the comparison tape");
            }
            if (enemy.active && native.active &&
                (native.grounded != enemy.grounded || native.spriteId != enemy.spriteId ||
                 native.imageFrame != static_cast<unsigned>(enemy.imageIndex)))
                std::cerr << "Actor presentation mismatch id=" << enemy.objectId << " tick=" << amiga.tick()
                    << " grounded=" << native.grounded << ',' << enemy.grounded
                    << " sprite=" << native.spriteId << ',' << enemy.spriteId
                    << " frame=" << native.imageFrame << ',' << enemy.imageIndex << '\n';
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
            // Native public subpixel coordinates project fifth-Q8/Q16 state
            // into Q8. Permit only that projection error, never a whole pixel.
            requireNear(240 + native.preciseX / 256.0F, enemy.preciseX, 1.0F / 256,
                        "paired actor horizontal trajectory agrees across targets");
            requireNear(176 + native.preciseY / 256.0F, enemy.preciseY, 1.0F / 256,
                        "paired actor vertical trajectory agrees within Q8 projection error");
            require(240 + native.x == enemy.x && 176 + native.y == enemy.y,
                    "paired actors publish identical pixel positions");
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
                reference.mainPlayerY(), 0.0F,
                "gameplay player Y follows decoded runtime");
    requireNear(static_cast<float>(160 + amiga.cameraOffsetX()),
                reference.mainCameraViewX(), 0.0F,
                "gameplay camera X follows decoded runtime");
    const float amigaCameraY = static_cast<float>(34 + amiga.cameraOffsetY());
    if (amigaCameraY != reference.mainCameraViewY()) {
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
    if (amiga.player().grounded() != reference.mainPlayerGrounded()) {
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
    unsigned ordinal = 0;
    for (unsigned slot = 0; slot < amiga.bulletCapacity; ++slot) {
        const auto& bullet = amiga.bulletAt(slot);
        if (!bullet.active) continue;
        const auto& host = reference.mainBulletAt(ordinal++);
        const auto& a = bullet.trajectory;
        const auto& b = host.trajectory;
        require(a.x.pixel + 240 == b.x.pixel && a.y.pixel + 176 == b.y.pixel &&
                a.x.fraction == b.x.fraction && a.y.fraction == b.y.fraction &&
                a.velocityX == b.velocityX && a.velocityY == b.velocityY &&
                a.direction == b.direction && a.zeroSpread == b.zeroSpread &&
                bullet.gemHigh == host.gemHigh && bullet.animation.index.equal(host.animation.index),
                "all live gameplay shots match fractional motion, birth state and animation in creation order");
        require(240 + bullet.x == host.x && 176 + bullet.y == host.y,
                "all live gameplay shots publish identical pixel positions");
    }
}

void testGeneratedWorldHandoff(const downwell::ObjectPack& objects) {
    {
        GameplayWorld presentation(0);
        downwell::GameMakerRandom oracle(presentation.randomState());
        presentation.registerLevelExit(4000);
        downwell::GoalTimeField::nextRadius(oracle); // Synchronous Create.
        for (unsigned frame = 0; frame < 100; ++frame) {
            const auto radius = downwell::GoalTimeField::nextRadius(oracle);
            presentation.step(0);
            require(presentation.goalField().radius.equal(radius) &&
                    presentation.randomDrawCount() == oracle.drawCount() &&
                    presentation.randomState().stateIndex() == oracle.stateIndex(),
                    "live world consumes exactly one exit-field Draw value per update");
            for (unsigned word = 0; word < 16; ++word)
                require(presentation.randomState().stateWord(word) == oracle.stateWord(word),
                        "live field Draw preserves the exact RNG stream");
        }
    }
    GameplayWorld world(0), direct(0);
    unsigned terrainCalls = 0;
    const auto terrain = [&](const downwell::DeferredCreateInstance&) { ++terrainCalls; return true; };
    downwell::DeferredCreateInstance rejected{111, 248, 192};
    require(!world.createGeneratedInstance(rejected, objects,
                [](const downwell::DeferredCreateInstance&) { return false; }) && world.enemyCount() == 0,
            "terrain-registration failure prevents actor creation");
    const downwell::ObjectPack empty(nullptr, 0);
    require(!world.createGeneratedInstance(rejected, empty, terrain) && world.enemyCount() == 0,
            "missing actor metadata fails without registering a partial actor");
    for (int id : {111, 112, 113, 116, 117, 119}) {
        downwell::DeferredCreateInstance command{id, 248, 192};
        command.integerVelocityX = 64; command.integerVelocityY = -26;
        command.velocityX = 99; command.velocityY = 99; // Must use target-neutral integer payload.
        command.integerDirection = 27; command.integerMoveSpeed = 320;
        command.motionState = -1; command.auxiliaryState = 1;
        command.alarms.set(0, 3); command.alarms.set(1, 17);
        command.active = id != 116; command.grounded = id == 111 || id == 112;
        const unsigned slot = world.enemyCount();
        require(world.createGeneratedInstance(command, objects, terrain), "core handoff accepts supported enemy birth");
        downwell::PackedObject definition;
        require(objects.findObject(id, definition), "generated enemy metadata exists");
        const auto& actor = world.enemyAt(slot);
        const auto profile = downwell::enemyCreateProfile(id);
        require(actor.objectId == id && actor.spriteId == definition.spriteId && actor.x == 8 && actor.y == 16 &&
                actor.health == profile.health && actor.reward == profile.reward && actor.imageSpeed == profile.animationRate &&
                actor.speedX == 64 && actor.speedY == -26 && actor.direction == 27 && actor.moveSpeed == 320 &&
                actor.motionState == -1 && actor.auxiliaryState == 1 && actor.alarm0 == 3 && actor.alarm1 == 17 &&
                actor.active == command.active && actor.grounded == command.grounded && actor.awake &&
                actor.activationPending == !command.active,
                "core birth preserves coordinates, metadata, motion, alarms and activation state");
    }
    require(world.randomDrawCount() == direct.randomDrawCount(), "actor handoff does not repeat builder RNG draws");
    require(world.createGeneratedInstance({92, 248, 192}, objects, terrain) && world.breakableActiveAt(8, 16),
            "breakable births use the same world-origin mapping");
    const unsigned beforeShop = terrainCalls;
    require(world.createGeneratedInstance({62, 385, 1024}, objects, terrain) &&
            direct.registerSideCameraLocker(385, 1024) && terrainCalls == beforeShop &&
            world.sideCameraDigest() == direct.sideCameraDigest(),
            "locker Create registers its normalized position synchronously without terrain or RNG work");
    require(world.createGeneratedInstance({277, 384, 1024}, objects, terrain) &&
            direct.registerShop(384, 1024) && terrainCalls == beforeShop,
            "shop Create is synchronous and bypasses plain terrain registration");
    require(world.createGeneratedInstance({306, 480, 2000}, objects,
                [&](const downwell::DeferredCreateInstance&) { return world.levelExitPresent(); }),
            "exit Create precedes its terrain callback");
    direct.registerLevelExit(2000);
    require(world.randomDrawCount() == direct.randomDrawCount() &&
            world.randomState().stateIndex() == direct.randomState().stateIndex(), "handoff retains synchronous Create RNG consumption");
    for (unsigned i = 0; i < 16; ++i)
        require(world.randomState().stateWord(i) == direct.randomState().stateWord(i), "handoff retains all Create RNG words");
    while (world.enemyCount() < world.enemyCapacity)
        require(world.createGeneratedInstance(rejected, objects, terrain), "birth fills remaining actor slots");
    require(!world.createGeneratedInstance(rejected, objects, terrain) && world.enemyCount() == world.enemyCapacity,
            "full world reports failed generation rather than dropping a birth silently");
}

void testDecodedSceneParity(const char* roomPath, const char* objectPath,
                            const char* builderPatternPath, bool shopFixture = false) {
    const std::vector<std::uint8_t> roomBytes = readFile(roomPath);
    const std::vector<std::uint8_t> objectBytes = readFile(objectPath);
    const std::vector<std::uint8_t> patternBytes = readFile(builderPatternPath);
    const downwell::RoomPack rooms(roomBytes.data(), roomBytes.size());
    const downwell::ObjectPack objects(objectBytes.data(), objectBytes.size());
    testGeneratedWorldHandoff(objects);
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

    {
        downwell::StartupRuntime restarted(downwell::PlatformProfile::desktop,
            &rooms, &objects, 2, &patterns);
        restarted.skipToGameForTesting();
        for (int visit = 0; visit < 3; ++visit) {
            downwell::Input moving;
            moving.right = true;
            for (int frame = 0; frame < 180; ++frame) restarted.step(moving, false);
            const auto oldTick = restarted.tick();
            require(restarted.restartFromResults(downwell::DeathMenuSequence::Action::Retry, 42),
                    "host retry recreates main-room instances");
            downwell::GameMakerRandom roomRandom(42);
            AmigaTerrainFragment restartFragments[19];
            AmigaAreaOneTerrainBuilder restartBuilder(restartFragments, 19);
            restartBuilder.resetRoom(-16, 704, roomRandom);
            require(roomRandom.drawCount() == restarted.randomDrawCount(),
                    "fresh native builder consumes the same room-creation RNG draws as decoded Retry");
            require(restarted.inMainRoom() && restarted.tick() == oldTick &&
                    restarted.mainPlayerHealth() == 4 && restarted.mainAmmo() == 8 &&
                    restarted.mainBulletCount() == 0 && restarted.mainCurrencyCount() == 0 &&
                    restarted.runTimeTicks() == 0 && !restarted.deathResults().active &&
                    restarted.runStage().area == 1 && restarted.runStage().level == 1 &&
                    restarted.controllerPersistent(),
                    "host restart clears gameplay state without resetting lifetime clock/controller");
            require(restarted.restartFromResults(downwell::DeathMenuSequence::Action::StyleSelect, 42) &&
                    restarted.inPlayMenu() && !restarted.playMenuSelected() &&
                    restarted.playMenuCursor() == 0 && restarted.runStage().area == 0,
                    "host style return recreates unconfirmed style room");
            require(restarted.restartFromResults(downwell::DeathMenuSequence::Action::Surface, 42) &&
                    restarted.inMenu() && restarted.menuPlayerNapping() &&
                    restarted.showSplash() == 0 && restarted.runStage().area == 0,
                    "host surface return recreates garden without publisher sequence");
            require(restarted.restartFromResults(downwell::DeathMenuSequence::Action::Retry, 42),
                    "host returns from garden directly to fresh gameplay");
        }
    }

    // Exercise a real room-entry handoff, not only the seed-2 capture used by
    // the native intro. Create and garden RNG consumption belongs to startup;
    // importing it must neither reconstruct nor consume those draws again.
    for (const std::uint32_t seed : {2U, 42U, 1234567U, 99U}) {
        downwell::StartupRuntime entry(
            downwell::PlatformProfile::nonDesktop, &rooms, &objects, seed, &patterns);
        entry.skipToGameForTesting();
        AmigaPortRuntime imported(300, false, 1201 - 176, true);
        imported.initializeEntryInput(false);
        downwell::PackedRoom room;
        require(rooms.findRoom(11, room), "handoff room metadata exists");
        LiveSceneWorld liveEntry(patterns, objects);
        liveEntry.bind(imported, rooms, room, false, &entry);
        bool sawLock = false, sawRelease = false;
        if (seed == 99) {
            // Synthetic central locker isolates Collision timing from side
            // room traversal. Native camera gate separately walks both sides.
            require(imported.registerSideCameraLocker(240, 240) &&
                entry.registerSideCameraLocker(240, 240), "paired locker registration");
        }
        for (unsigned tick = 0; tick < 240; ++tick) {
            parityFrame = static_cast<int>(tick);
            compareInitialGameplay(imported, entry);
            if (imported.cameraSideLocked()) sawLock = true;
            if (sawLock && !imported.cameraSideLocked()) sawRelease = true;
            require(imported.randomDrawCount() == entry.randomDrawCount() &&
                    imported.randomState().stateIndex() == entry.randomState().stateIndex(),
                    "room-entry RNG cursor remains exact across seeds");
            for (unsigned word = 0; word < 16; ++word)
                require(imported.randomState().stateWord(word) == entry.randomState().stateWord(word),
                        "room-entry RNG words remain exact across seeds");
            imported.step(0);
            entry.step(downwell::Input{}, false);
        }
        if (seed == 99) require(sawLock && sawRelease,
            "paired collision fixture enters and releases the source camera lock");
        require(imported.player().grounded() && entry.mainPlayerGrounded(),
                "imported room entry reaches the source entrance floor across seeds");
        const auto oldCount = liveEntry.builder.fragmentCount();
        require(oldCount > 0 && !liveEntry.builder.resetFromRoomEntry(entry.mainBuilderState()) &&
                liveEntry.builder.fragmentCount() == oldCount,
                "late handoff is rejected without discarding generated terrain");
    }

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
    if (shopFixture) {
        const auto nativeDraws = gameplay.randomDrawCount();
        const auto hostDraws = reference.randomDrawCount();
        require(!gameplay.levelExitPresent() && !reference.levelExitPresent(),
            "entrance does not already contain a level exit");
        gameplay.registerLevelExit(4000);
        reference.registerLevelExit(4000);
        require(gameplay.levelExitPresent() && reference.levelExitPresent() &&
            gameplay.levelExit().x == 240 && reference.levelExit().x == 240 &&
            gameplay.levelExit().y == 4000 && reference.levelExit().y == 4000 &&
            gameplay.goalField().x == 240 && reference.goalField().x == 240 &&
            gameplay.goalField().y == 4032 && reference.goalField().y == 4032,
            "both adapters synchronously create the centered exit and offset goal field");
        require(gameplay.randomDrawCount() == nativeDraws + 1 &&
            reference.randomDrawCount() == hostDraws + 1 &&
            gameplay.goalField().radius.equal(reference.goalField().radius),
            "goal-field Create consumes one matching RNG draw before shop stock creation");
        // Isolated fixture, not a modification of ordinary 1-1 generation.
        // Approach from outside the invisible wall, while overlapping counter mask.
        const int x = 240 + gameplay.player().offsetX() + 12;
        const int y = 176 + gameplay.player().offsetY();
        auto* nativeShop = gameplay.registerShop(x, y);
        auto* hostShop = reference.registerShop(x, y);
        require(nativeShop && hostShop, "both world adapters register a shop");
        require(nativeShop->farewellVariant == hostShop->farewellVariant,
            "shop creation chooses the same persistent farewell variant on both streams");
        for (unsigned i = 0; i < 3; ++i)
            require(nativeShop->stock.items[i].upgrade == hostShop->stock.items[i].upgrade &&
                nativeShop->stock.items[i].price == hostShop->stock.items[i].price,
                "registered stock uses identical shared RNG state");
        const unsigned basePrices[] = {300, 150, 500, 250, 400, 1000};
        const unsigned memberPrices[] = {270, 135, 450, 225, 360, 900};
        for (const auto& item : nativeShop->stock.items)
            require(item.price == basePrices[item.upgrade - 100],
                    "unowned Member's Card does not discount live stock");
        require(gameplay.grantUpgrade(14) && reference.grantUpgrade(14),
                "both adapters acquire Member's Card after the first shop exists");
        auto* nativeMemberShop = gameplay.registerShop(x + 1000, y);
        auto* hostMemberShop = reference.registerShop(x + 1000, y);
        require(nativeMemberShop && hostMemberShop, "subsequent live shops can be created with the card");
        for (unsigned i = 0; i < 3; ++i) {
            const auto& a = nativeMemberShop->stock.items[i];
            const auto& b = hostMemberShop->stock.items[i];
            require(a.upgrade == b.upgrade && a.price == memberPrices[a.upgrade - 100] && b.price == a.price,
                    "new shops snapshot the source ten-percent member discount on both adapters");
            require(nativeShop->stock.items[i].price == basePrices[nativeShop->stock.items[i].upgrade - 100] &&
                hostShop->stock.items[i].price == basePrices[hostShop->stock.items[i].upgrade - 100],
                "acquisition does not reprice existing shop stock");
        }
        auto tickShop = [&](bool pressed, bool goLeft = false, bool goRight = false) {
            downwell::Input input; input.fire = pressed; input.left = goLeft; input.right = goRight;
            gameplay.step(downwell::packReplayInput(input));
            reference.step(input, pressed);
            require(gameplay.shops().noControl == reference.shops().noControl &&
                gameplay.shops().interactable == reference.shops().interactable &&
                nativeShop->menu.cursor == hostShop->menu.cursor &&
                nativeShop->menu.cursorWobbleX == hostShop->menu.cursorWobbleX &&
                nativeShop->menu.cursorPhaseQuarters == hostShop->menu.cursorPhaseQuarters &&
                nativeShop->prompt.phase.equal(hostShop->prompt.phase) &&
                nativeShop->menu.active == hostShop->menu.active &&
                gameplay.player().grounded() && reference.mainPlayerGrounded(),
                "world interaction and grounded input suppression agree across adapters");
            const auto& a = nativeShop->feedback;
            const auto& b = hostShop->feedback;
            const auto& ka = nativeShop->keeper;
            const auto& kb = hostShop->keeper;
            require(ka.sprite == kb.sprite && ka.image.equal(kb.image) && ka.speed.equal(kb.speed) &&
                ka.alarm0 == kb.alarm0 && ka.alarm1 == kb.alarm1 && ka.alarm2 == kb.alarm2,
                "merchant animation phase and alarms agree exactly across adapters");
            require(a.message == b.message && a.thankful == b.thankful && a.goodbye == b.goodbye &&
                a.confused == b.confused && a.resetKeeperImage == b.resetKeeperImage &&
                a.soundCount == b.soundCount && !a.overflow && !b.overflow,
                "both runtimes retain identical merchant feedback");
            for (unsigned i = 0; i < a.soundCount; ++i)
                require(a.sounds[i].resource == b.sounds[i].resource &&
                    a.sounds[i].priority == b.sounds[i].priority && a.sounds[i].channel == b.sounds[i].channel,
                    "merchant/UI sound requests preserve source order across adapters");
            for (unsigned i = 0; i < 3; ++i)
                require(a.itemPresent[i] == b.itemPresent[i] && a.storedItem[i] == b.storedItem[i],
                    "purchased child disappearance agrees across adapters");
        };
        tickShop(false); tickShop(true);
        require(nativeShop->menu.active && gameplay.shops().noControl,
            "live controller opens and locks movement instead of jumping");
        tickShop(true);
        require(nativeShop->stock.items[0].remaining == 1, "held opening press does not buy");
        tickShop(false); tickShop(true);
        require(nativeShop->stock.items[0].remaining == 1 && hostShop->stock.items[0].remaining == 1,
            "unfunded purchase preserves stock in both runtimes");
        // Test-only free battery isolates queued grant timing from gem collection.
        nativeShop->stock.items[0] = hostShop->stock.items[0] = {101, 1, 0};
        const int capacity = gameplay.player().ammoCapacity();
        tickShop(false); tickShop(true);
        require(gameplay.consumables().pending[1] == 1 && reference.consumables().pending[1] == 1 &&
            gameplay.player().ammoCapacity() == capacity && reference.mainAmmoCapacity() == capacity,
            "purchase queues, rather than immediately applies, battery");
        tickShop(false, true);
        require(gameplay.player().ammoCapacity() == capacity + 1 &&
            reference.mainAmmoCapacity() == capacity + 1 && nativeShop->menu.cursor == 0,
            "next controller phase consumes grant while left selects Leave");
        tickShop(true);
        require(!gameplay.shops().noControl && !nativeShop->menu.active,
            "live Leave releases movement lock");
        for (int i = 0; i < 20; ++i) tickShop(false, false, true);
        // Existing player queries use integer cells but retain subpixel position.
        require(240 + gameplay.player().offsetX() <= x - 12 && reference.mainPlayerX() < x - 11 &&
            std::fabs(reference.mainPlayerX() - (240 + gameplay.player().offsetXFixed() / 256.0F)) < 0.01F,
            "invisible counter wall blocks rightward walking equally in both runtimes");
        tickShop(false); tickShop(true);
        require(nativeShop->menu.active && nativeShop->stock.items[0].remaining == 0 &&
            hostShop->stock.items[0].remaining == 0, "live reentry preserves sold stock");
        tickShop(false, true); tickShop(true);
        const int exitY = 176 + gameplay.player().offsetY();
        gameplay.registerLevelExit(exitY);
        reference.registerLevelExit(exitY);
        gameplay.step(0); reference.step({}, false);
        require(gameplay.goalStopped() && reference.goalStopped() &&
            gameplay.levelExit().alarm0 == 5 && reference.levelExit().alarm0 == 5,
            "living exit collision stops both players and arms five full alarm phases");
        const auto nativeClock = gameplay.runTimeTicks();
        const auto hostClock = reference.runTimeTicks();
        const int stoppedY = gameplay.player().offsetY();
        for (unsigned tick = 1; tick <= 100; ++tick) {
            downwell::Input locked; locked.fire = true; locked.right = true;
            gameplay.step(downwell::packReplayInput(locked)); reference.step(locked, false);
            require(gameplay.player().offsetY() == stoppedY && reference.mainPlayerY() == exitY &&
                gameplay.runTimeTicks() == nativeClock && reference.runTimeTicks() == hostClock,
                "exit latch suppresses jump/fall and stops the run clock on both adapters");
            require(gameplay.exitDitherRequested() == (tick >= 5) &&
                reference.exitDitherRequested() == (tick >= 5) &&
                gameplay.levelResultRequested() == (tick >= 95) &&
                reference.levelResultRequested() == (tick >= 95),
                "both live adapters request dither at five and results at ninety-five ticks");
            const auto& a = gameplay.exitDither();
            const auto& b = reference.exitDither();
            require(a.active == b.active && a.x == b.x && a.y == b.y &&
                a.alarm0 == b.alarm0 && a.soundRequested == b.soundRequested,
                "exit dither world motion and sound timing agree across adapters");
            require(a.soundRequested == (tick >= 20),
                "exit dither sound follows fifteen alarm phases after tick-five creation");
        }
        for (unsigned id : {6u, 6u, 12u, 13u, 19u, 21u, 100u}) {
            require(gameplay.grantUpgrade(id) && reference.grantUpgrade(id), "shared upgrade grant accepted");
            require(gameplay.health() == reference.mainPlayerHealth() &&
                gameplay.healthMaximum() == reference.mainPlayerHealthMaximum() &&
                gameplay.player().ammoCapacity() == reference.mainAmmoCapacity() &&
                gameplay.currency() == reference.mainCurrencyTotal() &&
                gameplay.upgrades().count == reference.upgrades().count &&
                gameplay.consumables().pending[0] == reference.consumables().pending[0],
                "both runtime adapters apply identical grants, refunds and deferred consumables");
        }
        for (int completedLevel = 1; completedLevel <= 2; ++completedLevel) {
            require(gameplay.enterLevelClear(0, false) && reference.enterLevelClear(0, false),
                "both adapters consume the exit request at an explicit scene boundary");
            const auto nativeTick = gameplay.tick();
            const auto hostTick = reference.tick();
            gameplay.step(0); reference.step({}, false);
            require(gameplay.tick() == nativeTick && reference.tick() == hostTick,
                "old gameplay dispatcher cannot advance while the clear room owns the runtime");
            bool nativeHeart = false, hostHeart = false;
            for (unsigned tick = 1; tick <= 211; ++tick) {
                bool nativeConfirm = true, hostConfirm = true;
                std::vector<unsigned> nativeEvents, hostEvents;
                const bool right = tick == 61 || tick == 62;
                const bool left = tick == 75;
                require(gameplay.stepLevelClear(nativeConfirm, left, right, nativeHeart,
                    [&](downwell::LevelClearSelection::Effect event, unsigned value) {
                        nativeEvents.push_back(unsigned(event)); nativeEvents.push_back(value);
                    }) && reference.stepLevelClear(hostConfirm, left, right, hostHeart,
                    [&](downwell::LevelClearSelection::Effect event, unsigned value) {
                        hostEvents.push_back(unsigned(event)); hostEvents.push_back(value);
                    }), "both result adapters update successfully");
                const auto& a = gameplay.levelClear();
                const auto& b = reference.levelClear();
                require(nativeEvents == hostEvents && nativeConfirm == hostConfirm && nativeHeart == hostHeart &&
                    a.selection.cursor == b.selection.cursor && a.selection.next == b.selection.next &&
                    a.selection.visible == b.selection.visible && a.motion.playerY.equal(b.motion.playerY) &&
                    a.motion.cameraY.equal(b.motion.cameraY) &&
                    a.carousel.position.magnitude.equal(b.carousel.position.magnitude) &&
                    a.carousel.position.negative == b.carousel.position.negative,
                    "result event order, input, camera, player and signed carousel match");
                for (unsigned i = 0; i < 4; ++i)
                    require(a.offers.ids[i] == b.offers.ids[i], "actual runtime RNG produces matching upgrade offers");
                require(gameplay.randomState().stateIndex() == reference.randomState().stateIndex() &&
                    gameplay.randomDrawCount() == reference.randomDrawCount(), "result RNG cursor/count match");
                for (unsigned i = 0; i < 16; ++i)
                    require(gameplay.randomState().stateWord(i) == reference.randomState().stateWord(i),
                        "result full RNG state matches");
                require(gameplay.health() == reference.mainPlayerHealth() &&
                    gameplay.player().ammo() == reference.mainAmmo() &&
                    gameplay.currency() == reference.mainCurrencyTotal() &&
                    gameplay.levelClearFrame().tileY == reference.levelClearFrame().tileY &&
                    gameplay.levelClearFrame().time == reference.levelClearFrame().time &&
                    gameplay.exitDither().y == reference.exitDither().y &&
                    gameplay.nextLevelRequested() == (tick == 211) &&
                    reference.nextLevelRequested() == (tick == 211),
                    "result grants, presentation and departure requests match across adapters");
            }
            const auto carriedRandom = gameplay.randomState();
            const auto carriedClock = gameplay.runTimeTicks();
            const int carriedHealth = gameplay.health();
            const auto carriedCurrency = gameplay.currency();
            const auto carriedUpgrades = gameplay.upgrades().count;
            const auto carriedInventory = gameplay.upgrades();
            const int carriedCapacity = gameplay.player().ammoCapacity();
            downwell::Input heldEntry; heldEntry.fire = true;
            require(gameplay.prepareNextAreaOneLevel(heldEntry, 319) && reference.loadNextAreaOneLevel(heldEntry),
                "both runtime adapters accept completed departure into the next area-one level");
            require(gameplay.runStage().level == completedLevel + 1 &&
                reference.runStage().level == completedLevel + 1 &&
                !gameplay.levelClearActive() && !reference.levelClearActive() &&
                gameplay.health() == carriedHealth && reference.mainPlayerHealth() == carriedHealth &&
                gameplay.currency() == carriedCurrency && reference.mainCurrencyTotal() == carriedCurrency &&
                gameplay.upgrades().count == carriedUpgrades && reference.upgrades().count == carriedUpgrades &&
                gameplay.player().ammoCapacity() == carriedCapacity && reference.mainAmmoCapacity() == carriedCapacity &&
                gameplay.player().ammo() == carriedCapacity && reference.mainAmmo() == carriedCapacity &&
                gameplay.runTimeTicks() == carriedClock,
                "native room retirement preserves actual grants, currency, health, clock and capacity");
            for (unsigned id = 0; id < 23; ++id)
                require(gameplay.upgrades().levels[id] == carriedInventory.levels[id] &&
                    reference.upgrades().levels[id] == carriedInventory.levels[id],
                    "every owned upgrade level survives both room transitions");
            require(gameplay.bulletCount() == 0 && gameplay.enemyCount() == 0 && gameplay.gemCount() == 0 &&
                gameplay.ringOrigin() == 319 && gameplay.player().jumpShootLocked() &&
                gameplay.cameraOffsetY() == 0 && gameplay.cameraOffsetX() == 0,
                "native old actors and camera retire without a large temporary world copy");
            require(gameplay.nextMuzzleOffset() == 0 && reference.mainNextMuzzleOffset() == 0,
                    "owned Laser Sight centres both newly created player emitters");
            require(gameplay.randomDrawCount() == carriedRandom.drawCount() &&
                gameplay.randomState().stateIndex() == carriedRandom.stateIndex(),
                "native retirement neither reseeds nor consumes generation RNG");
            for (unsigned i = 0; i < 16; ++i)
                require(gameplay.randomState().stateWord(i) == carriedRandom.stateWord(i),
                    "native next-room builder receives the untouched RNG state");
            require(!gameplay.prepareNextAreaOneLevel(heldEntry, 0) && !reference.loadNextAreaOneLevel(heldEntry),
                "duplicate next-room consumption cannot skip a stage");
            live.bind(gameplay, rooms, terrainRoom, true);
            require(gameplay.randomDrawCount() == reference.randomDrawCount() &&
                gameplay.randomState().stateIndex() == reference.randomState().stateIndex(),
                "native next-room builder initialization matches SDL room Create RNG");
            for (unsigned i = 0; i < 16; ++i)
                require(gameplay.randomState().stateWord(i) == reference.randomState().stateWord(i),
                    "all RNG words match after next-room terrain reset");
            const unsigned entryShots = gameplay.player().shots();
            bool sawReentryEnemy = false;
            for (unsigned tick = 0; tick < 720; ++tick) {
                downwell::Input input;
                if (tick >= 90) {
                    input.left = (tick / 150) % 2 != 0;
                    input.right = !input.left;
                    if (!sawReentryEnemy) {
                        // This seeded entrance closes around breakable centre
                        // blocks. Clear that route before traversing the shaft.
                        const int x = 240 + gameplay.player().offsetX();
                        input.left = x > 244;
                        input.right = x < 236;
                    }
                    // Sparse taps allow descent between jumps and shots.
                    input.fire = tick % 60 == 0 || tick % 60 == 3;
                }
                gameplay.step(downwell::packReplayInput(input)); reference.step(input, false);
                parityFrame = gameplay.tick();
                compareInitialGameplay(gameplay, reference);
                compareGameplayShot(gameplay, reference);
                for (unsigned enemy = 0; enemy < gameplay.enemyCount(); ++enemy)
                    sawReentryEnemy = sawReentryEnemy || gameplay.enemyAt(enemy).active;
                require(gameplay.generationSucceeded() && reference.deferredCommandsSucceeded(),
                    "next-stage live generation succeeds on both adapters");
                require(std::fabs(reference.mainPlayerX() - (240 + gameplay.player().offsetXFixed() / 256.0F)) < 0.01F &&
                    std::fabs(reference.mainPlayerY() - (176 + gameplay.player().offsetYFixed() / 256.0F)) < 0.01F &&
                    gameplay.player().grounded() == reference.mainPlayerGrounded() &&
                    gameplay.randomDrawCount() == reference.randomDrawCount(),
                    "next-stage movement, terrain collision and generation RNG remain paired");
                require(160 + gameplay.cameraOffsetX() == reference.mainCameraRenderViewX() &&
                    34 + gameplay.cameraOffsetY() == reference.mainCameraRenderViewY(),
                    "both regenerated rooms publish identical camera views");
            }
            std::cout << "REENTRY level=" << gameplay.runStage().level
                << " enemies=" << gameplay.enemyCount() << " active=" << sawReentryEnemy
                << " shots=" << gameplay.player().shots() - entryShots
                << " player=" << 240 + gameplay.player().offsetX() << ',' << 176 + gameplay.player().offsetY() << '\n';
            require(sawReentryEnemy && gameplay.player().shots() > entryShots,
                    "each regenerated room exercises active enemies and firing, not just an idle landing");
            if (completedLevel == 1) {
                // Reuse the same runtime and world, not fresh stage-two fixtures:
                // exercise retiring and rebinding a second generated room.
                const int nextExitY = 176 + gameplay.player().offsetY();
                gameplay.registerLevelExit(nextExitY);
                reference.registerLevelExit(nextExitY);
                for (unsigned tick = 0; tick < 96; ++tick) {
                    gameplay.step(0); reference.step({}, false);
                }
                require(gameplay.levelResultRequested() && reference.levelResultRequested(),
                    "second live room reaches its own clear request");
            }
        }
        return;
    }
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
        compareInitialGameplay(gameplay, reference);
        compareGameplayShot(gameplay, reference);
        if (gameplay.player().grounded() &&
            reference.mainPlayerGrounded())
            break;
    }
    require(gameplay.player().grounded() && reference.mainPlayerGrounded(),
            "both runtimes land on recovered terrain after gunboot fire");
    compareInitialGameplay(gameplay, reference);
    compareGameplayShot(gameplay, reference);
    require(gameplay.player().ammo() == 8 && reference.mainAmmo() == 8,
            "landing recharges both runtimes to eight shots");

    const int groundedStartX = gameplay.player().offsetX();
    for (int tick = 0; tick < 4; ++tick) {
        parityFrame += 1;
        gameplay.step(downwell::packReplayInput(left));
        reference.step(left, false);
        compareInitialGameplay(gameplay, reference);
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
    bool traverseFire = false;
    for (int tick = 0; tick < 600; ++tick) {
        parityFrame += 1;
        downwell::Input traverse;
        traverse.right = (tick / 90) % 2 == 0;
        traverse.left = !traverse.right;
        traverse.fire = gameplay.player().grounded();
        gameplay.step(downwell::packReplayInput(traverse));
        reference.step(traverse, traverse.fire && !traverseFire);
        traverseFire = traverse.fire;
        compareInitialGameplay(gameplay, reference);
        for (unsigned enemy = 0; enemy < gameplay.enemyCount(); ++enemy)
            sawAwakeEnemy = sawAwakeEnemy || gameplay.enemyAt(enemy).awake;
        if (sawAwakeEnemy && sawExtendedEnemyAnimation && sawPairedFrogAir && sawPairedFrogGround &&
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
    if (!sawPairedFrogAir || !sawPairedFrogGround)
        std::cerr << "Traversal coverage frog=" << sawPairedFrogAir << ',' << sawPairedFrogGround
            << " player=" << reference.mainPlayerX() << ',' << reference.mainPlayerY()
            << " health=" << reference.mainPlayerHealth() << " camera=" << gameplay.cameraOffsetY() << '\n';
    require(sawPairedFrogAir && sawPairedFrogGround,
            "frog presentation comparison covers both flight and landing");
    require(enemyOrdinal == gameplay.enemyCount(),
            "live generation creates exactly the SDL enemy prefix, not a forecast population");
    require(live.builder.fragmentCount() > 0 &&
                live.builder.fragmentCount() < amigaInitialTerrainFragmentCount,
            "comparison exercises incremental terrain rather than a complete forecast");
    if (std::getenv("DOWNWELL_TRACE_COMBAT") || std::getenv("DOWNWELL_VERIFY_COMBAT")) {
        const bool laserFixture = std::getenv("DOWNWELL_VERIFY_LASER_SIGHT") != nullptr;
        bool sawLaserBullet = false;
        if (laserFixture) {
            const int nativeOffset = gameplay.nextMuzzleOffset(), hostOffset = reference.mainNextMuzzleOffset();
            require(gameplay.grantUpgrade(12) && reference.grantUpgrade(12), "both adapters grant Laser Sight");
            require(gameplay.nextMuzzleOffset() == nativeOffset && reference.mainNextMuzzleOffset() == hostOffset,
                    "granting Laser Sight does not rerun player-instance initialization mid-room");
        }
        const bool gemHighFixture = std::getenv("DOWNWELL_VERIFY_GEM_HIGH") != nullptr;
        bool sawHighBullet = false;
        if (gemHighFixture) {
            // Inject only an accumulated combo; real movement/landing must
            // award its gems and the next controller phase must select level 1.
            auto& nativeCombo = const_cast<downwell::RunCombo&>(gameplay.runCombo());
            auto& hostCombo = const_cast<downwell::RunCombo&>(reference.runCombo());
            nativeCombo.state.count = hostCombo.state.count = 8;
        }
        const bool candleFixture = std::getenv("DOWNWELL_VERIFY_CANDLE") != nullptr;
        const bool poweredFixture = std::getenv("DOWNWELL_VERIFY_GEM_POWERED") != nullptr;
        if (poweredFixture)
            require(gameplay.grantUpgrade(4) && reference.grantUpgrade(4),
                    "both live adapters grant Gem Powered before combat");
        const bool streakFixture = std::getenv("DOWNWELL_VERIFY_STREAK_UPGRADE") != nullptr;
        bool sawExtendedStreak = false;
        if (streakFixture)
            require(gameplay.grantUpgrade(20) && reference.grantUpgrade(20),
                    "both live adapters grant the streak extension upgrade before combat");
        bool sawCandleDamage = false;
        if (candleFixture)
            require(gameplay.grantUpgrade(7) && reference.grantUpgrade(7),
                    "both live adapters grant Candle before the combat tape");
        if (std::getenv("DOWNWELL_VERIFY_ATTRACTOR")) {
            require(gameplay.gemCount() == 0 && reference.mainCurrencyCount() == 0,
                    "Attractor combat fixture starts before any reward birth");
            require(gameplay.grantUpgrade(10) && reference.grantUpgrade(10),
                    "both live adapters grant the source Gem Attractor upgrade");
        }
        bool reported = false;
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
            // Begin Step tests the previous published position, before movement.
            const bool nativeField = gameplay.levelExitPresent() && gameplay.goalField().meetsPlayer(
                240 + downwell::roundGameMakerQ8(gameplay.player().offsetXFixed()),
                176 + downwell::roundGameMakerQ8(gameplay.player().offsetYFixed()));
            const bool hostField = reference.levelExitPresent() && reference.goalField().meetsPlayer(
                downwell::roundGameMakerCoordinate(reference.mainPlayerX()),
                downwell::roundGameMakerCoordinate(reference.mainPlayerY()));
            const bool nativeClockStopped = nativePublishedDeath || gameplay.goalStopped() || nativeField;
            const bool hostClockStopped = hostPublishedDeath || reference.goalStopped() || hostField;
            const unsigned gemsBeforeNative = gameplay.gameGemTotal();
            const unsigned gemsBeforeHost = reference.mainGameGemTotal();
            const bool nativeWasRecorded = gameplay.runGemStatistics().recorded;
            const bool hostWasRecorded = reference.runGemStatistics().recorded;
            const unsigned nativeShots = gameplay.player().shots(), hostShots = reference.mainShotsFired();
            const unsigned nativeJumps = gameplay.player().jumps(), hostJumps = reference.mainJumps();
            gameplay.step(downwell::packReplayInput(input));
            reference.step(input, input.fire && (tick + firePhase) % 10 == 0);
            if (laserFixture && tick % 30 == 0) {
                using C = downwell::LaserSightRay::Coordinate;
                const C x{static_cast<int>(reference.mainPlayerX()), 12345};
                const C y{static_cast<int>(reference.mainPlayerY()), 54321};
                const C bottom{static_cast<int>(reference.mainCameraViewY()) + 284, 0};
                const auto nativeDraws = gameplay.randomDrawCount();
                const auto hostDraws = reference.randomDrawCount();
                const auto actualNative = gameplay.laserSightEndpoint();
                const auto actualHost = reference.mainLaserSightEndpoint();
                require(actualNative.x.pixel == actualHost.x.pixel && actualNative.x.fraction == actualHost.x.fraction &&
                        actualNative.y.pixel == actualHost.y.pixel && actualNative.y.fraction == actualHost.y.fraction,
                        "actual player speed, subpixels and camera feed identical ray endpoints");
                for (std::uint32_t aim : {0u, 0x0aaaaaaau, 0xf5555556u}) {
                    const auto native = gameplay.traceLaserSight(x, y, bottom, 0, aim);
                    const auto host = reference.traceLaserSight(x, y, bottom, 0, aim);
                    require(native.x.pixel == host.x.pixel && native.x.fraction == host.x.fraction &&
                            native.y.pixel == host.y.pixel && native.y.fraction == host.y.fraction,
                            "live Laser Sight adapters agree on room-coordinate endpoints");
                }
                require(nativeDraws == gameplay.randomDrawCount() && hostDraws == reference.randomDrawCount(),
                        "live Laser Sight queries leave RNG untouched");
            }
            if (160 + gameplay.cameraOffsetX() != reference.mainCameraRenderViewX() ||
                    34 + gameplay.cameraOffsetY() != reference.mainCameraRenderViewY())
                std::cerr << "COMBAT CAMERA tick=" << gameplay.tick()
                          << " native=" << 160 + gameplay.cameraOffsetX() << ',' << 34 + gameplay.cameraOffsetY()
                          << " SDL=" << reference.mainCameraRenderViewX() << ',' << reference.mainCameraRenderViewY() << '\n';
            require(160 + gameplay.cameraOffsetX() == reference.mainCameraRenderViewX() &&
                    34 + gameplay.cameraOffsetY() == reference.mainCameraRenderViewY(),
                    "combat and death publish identical integer camera views on every update");
            const auto& nativeDeath = gameplay.deathSequence();
            const auto& hostDeath = reference.mainDeathSequence();
            require(nativeDeath.flashVisible() == hostDeath.flashVisible(),
                    "both live paths publish the same death flash interval");
            require(gameplay.ammoHudOffset() == reference.mainAmmoHudOffset(),
                    "both live HUDs share firing offset and decay on every combat update");
            require(gameplay.ammoMeterOffset() == reference.mainAmmoMeterOffset(),
                    "both live HUDs share successful recharge movement");
            require(gameplay.ammoGauge().filled == reference.mainAmmoGauge().filled,
                    "both live gauges agree on fractional fill boundaries");
            for (unsigned row = 0; row < 184; ++row)
                require(gameplay.ammoGauge().state[row] == reference.mainAmmoGauge().state[row],
                        "both live gauges share every row animation state");
            if (gameplay.player().shots() != nativeShots)
                require(gameplay.ammoHudOffset() == 2,
                        "an actual shot reaches the HUD presentation state immediately");
            require(gameplay.runStage().area == 1 && gameplay.runStage().level == 1 &&
                    reference.runStage().area == 1 && reference.runStage().level == 1,
                    "both live paths enter and retain the supported first stage");
            require(gameplay.runCombo().state.count == reference.runCombo().state.count &&
                    gameplay.runCombo().state.best == reference.runCombo().state.best &&
                    gameplay.runCombo().state.record == reference.runCombo().state.record &&
                    gameplay.runCombo().healthReserve.maximum == reference.runCombo().healthReserve.maximum &&
                    gameplay.runCombo().healthReserve.pieces == reference.runCombo().healthReserve.pieces &&
                    gameplay.player().ammoCapacity() == reference.mainAmmoCapacity(),
                    "live combo counters, records and grant reserves agree each update");
            if (gameplay.runTimeTicks() != reference.runTimeTicks() ||
                    gameplay.runTimeTicks() != beforeNativeTime + (nativeClockStopped ? 0u : 1u) ||
                    reference.runTimeTicks() != beforeHostTime + (hostClockStopped ? 0u : 1u))
                std::cerr << "Combat clock tick=" << gameplay.tick()
                    << " before=" << beforeNativeTime << ',' << beforeHostTime
                    << " after=" << gameplay.runTimeTicks() << ',' << reference.runTimeTicks()
                    << " death=" << nativePublishedDeath << ',' << hostPublishedDeath
                    << " health=" << gameplay.health() << ',' << reference.mainPlayerHealth() << '\n';
            require(gameplay.runTimeTicks() == reference.runTimeTicks() &&
                        gameplay.runTimeTicks() == beforeNativeTime + (nativeClockStopped ? 0u : 1u) &&
                        reference.runTimeTicks() == beforeHostTime + (hostClockStopped ? 0u : 1u),
                    "shared run clock agrees and respects pre-publication death, goal stop and exit field");
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
                require(bullet.gemHigh == hostBullet.gemHigh,
                        "live bullets retain matching birth-time weapon levels");
                require(bullet.animation.index.equal(hostBullet.animation.index),
                        "live bullets retain identical shared animation phase");
                const auto& a = bullet.trajectory;
                const auto& b = hostBullet.trajectory;
                const bool same = a.x.pixel + 240 == b.x.pixel && a.y.pixel + 176 == b.y.pixel &&
                    a.x.fraction == b.x.fraction && a.y.fraction == b.y.fraction &&
                    a.velocityX == b.velocityX && a.velocityY == b.velocityY && a.direction == b.direction &&
                    a.zeroSpread == b.zeroSpread;
                if (!same) std::cerr << "Projectile phase mismatch at " << gameplay.tick()
                    << " directions=" << a.direction << ',' << b.direction << '\n';
                require(same, "live bullets retain identical fractional positions, spread and velocities");
                if (a.zeroSpread) {
                    sawLaserBullet = true;
                    require(a.direction == 0xc0000000u && a.velocityX == 0 &&
                            a.velocityY == 8 * downwell::MachineGunTrajectory::unit,
                            "Laser Sight projectiles retain exact straight-down velocity");
                }
                sawHighBullet |= bullet.gemHigh;
                require(bullet.x + 240 == hostBullet.x && bullet.y + 176 == hostBullet.y,
                        "combat bullets publish identical pixels in creation order");
            }
            require(gameplay.randomDrawCount() == reference.randomDrawCount() &&
                    gameplay.randomState().stateIndex() == reference.randomState().stateIndex(),
                    "live combat retains matching RNG cursor and consumption after spread");
            for (unsigned word = 0; word < 16; ++word)
                require(gameplay.randomState().stateWord(word) == reference.randomState().stateWord(word),
                        "live combat retains all runner RNG words after spread");
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
            std::vector<std::array<long, 10>> nativeGems, referenceGems;
            for (unsigned i = 0; i < gameplay.gemCapacity; ++i) {
                const auto& gem = gameplay.gemAt(i);
                if (gem.active) nativeGems.push_back({static_cast<long>(gem.age), gem.amount,
                    gem.motion.animation.rateTenths, static_cast<long>(gem.motion.animation.phaseTenths),
                    gem.motion.x, gem.motion.y, gem.motion.vx, gem.motion.vy, gem.motion.attracted,
                    gem.motion.attractorAtBirth});
            }
            for (unsigned i = 0; i < reference.mainCurrencyCount(); ++i) {
                const auto& gem = reference.mainCurrencyAt(i);
                referenceGems.push_back({static_cast<long>(gem.age), gem.amount,
                    gem.motion.animation.rateTenths, static_cast<long>(gem.motion.animation.phaseTenths),
                    gem.motion.x, gem.motion.y, gem.motion.vx, gem.motion.vy, gem.motion.attracted,
                    gem.motion.attractorAtBirth});
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
            if (std::getenv("DOWNWELL_VERIFY_ATTRACTOR"))
                for (const auto& gem : nativeGems)
                    require(gem[9] == 1, "live reward births receive the owned Attractor configuration");
            require(gameplay.invulnerability() == static_cast<unsigned>(reference.mainPlayerInvulnerability()),
                    "live damage immunity agrees every combat update");
            require(gameplay.gemStreakTimer() == reference.mainGemStreakTimer(),
                    "live gem streak timers agree every combat update");
            require(gameplay.player().ammo() == reference.mainAmmo(),
                    "live charge agrees after all pickup effects, not just player movement");
            if (streakFixture && gameplay.gemStreakTimer() == 480) sawExtendedStreak = true;
            if (candleFixture && gameplay.invulnerability() == 240)
                sawCandleDamage = true;
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
            const bool samePlayer = 240 + gameplay.player().offsetXFixed() / 256.0F == reference.mainPlayerX() &&
                176 + gameplay.player().offsetYFixed() / 256.0F == reference.mainPlayerY() &&
                gameplay.player().grounded() == reference.mainPlayerGrounded();
            if (!samePlayer) {
                std::cout << "COMBAT PLAYER tick=" << gameplay.tick()
                          << " x=" << 240 + gameplay.player().offsetXFixed() / 256.0F << ',' << reference.mainPlayerX()
                          << " y=" << 176 + gameplay.player().offsetYFixed() / 256.0F << ',' << reference.mainPlayerY()
                          << " ground=" << gameplay.player().grounded() << ',' << reference.mainPlayerGrounded()
                          << " jumps=" << gameplay.player().jumps() << ',' << reference.mainJumps()
                          << " shots=" << gameplay.player().shots() << ',' << reference.mainShotsFired()
                          << " ammo=" << gameplay.player().ammo() << ',' << reference.mainAmmo() << '\n';
            }
            require(samePlayer, "combat and death preserve identical player position and support every update");
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
        require(!candleFixture || sawCandleDamage,
                "Candle live fixture actually takes damage and publishes the extended interval");
        require(!laserFixture || sawLaserBullet, "Laser Sight live tape actually emits upgraded shots");
        require(!streakFixture || sawExtendedStreak,
                "streak fixture actually earns gems and publishes the extended interval");
        require(!poweredFixture || gameplay.currency() > 0,
                "Gem Powered combat fixture earns gems during the firing tape");
        require(!gemHighFixture || sawHighBullet,
                "live combo fixture actually emits powered-up projectiles");
        std::cout << "COMBAT END health=" << gameplay.health() << ',' << reference.mainPlayerHealth()
                  << " kills=" << gameplay.enemiesDefeated() << ',' << reference.mainEnemiesDefeated()
                  << " currency=" << gameplay.currency() << ',' << reference.mainCurrencyTotal()
                  << " bestCombo=" << gameplay.runCombo().state.best << ',' << reference.runCombo().state.best << '\n';
        require(gameplay.runCombo().state.best > 0, "combat tape completes a nonzero combo");
        require(sawPairedCurrencyAnimation, "combat tape actually covers live currency animation");
        require(gameplay.enemiesDefeated() > 0 && reference.mainEnemiesDefeated() > 0 &&
                    gameplay.currency() > 0 && reference.mainCurrencyTotal() > 0,
                "combat diagnostic actually exercises kills and reward collection on both targets");
        if (std::getenv("DOWNWELL_VERIFY_COMBAT"))
            require(!reported,
                    "extended combat tape must match movement, health, kills and currency");
        if (std::getenv("DOWNWELL_VERIFY_FROG_MOTION"))
            require(sawPairedFrogHit, "strict combat tape exercises a surviving frog hit on both adapters");
        if (std::getenv("DOWNWELL_VERIFY_DEATH")) {
            require(pairedDeadFrames >= 120,
                    "death-state regression includes at least 120 paired depleted-health updates");
            require(gameplay.deathSequence().menuReady && reference.mainDeathSequence().menuReady &&
                    gameplay.deathResults().active && reference.deathResults().active &&
                    gameplay.runGemStatistics().recorded && reference.runGemStatistics().recorded,
                    "natural death reaches both results controllers and records both runs");
            std::cout << "DEATH COVERAGE pairedDeadFrames=" << pairedDeadFrames
                      << " results=active statistics=recorded\n";
        }
        if (std::getenv("DOWNWELL_VERIFY_SURVIVAL"))
            require(combatTicks == 6000 && pairedDeadFrames == 0 &&
                    gameplay.health() > 0 && reference.mainPlayerHealth() > 0 &&
                    !gameplay.dead() && !reference.mainDeathSequence().movementDead(),
                    "long survival tape remains alive on both adapters with corrected normal damage");
    }
    // Exercise the real controller boundaries after the existing tape, without
    // changing its inputs, digests or required combat encounters.
    const int chargeBefore = gameplay.player().ammoCapacity();
    for (unsigned id : {101u, 101u, 105u}) {
        require(gameplay.grantConsumable(id) && reference.grantConsumable(id), "both runtimes accept queued purchases");
    }
    require(gameplay.player().ammoCapacity() == chargeBefore && reference.mainAmmoCapacity() == chargeBefore,
            "purchase grant is not an immediate refill");
    for (unsigned step = 0; step < 3; ++step) {
        const auto frame = gameplay.step(0);
        reference.step(downwell::Input{}, false);
        require(gameplay.player().ammoCapacity() == reference.mainAmmoCapacity() &&
                gameplay.health() == reference.mainPlayerHealth() &&
                gameplay.healthMaximum() == reference.mainPlayerHealthMaximum(), "live consumable application matches");
        require(gameplay.consumables().effects.count == (step == 0 ? 2u : step == 1 ? 1u : 0u) &&
                gameplay.consumables().effects.count == reference.consumables().effects.count,
                "both controller paths consume one of each type per update");
        if (step < 2) require(frame.ammoChanged, "native HUD invalidates after queued charge grants");
        for (unsigned i = 0; i < 6; ++i)
            require(gameplay.consumables().pending[i] == reference.consumables().pending[i], "pending counts match");
    }
    require(gameplay.player().ammoCapacity() == chargeBefore + 2, "two batteries consumed over two updates");
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

// Input-only route: inspect terrain to choose buttons, never modify the world.
// This checks the actual room builder and collision adapters, not exit fixtures.
void testNaturalExit(const char* roomPath, const char* objectPath, const char* patternPath) {
    auto rb = readFile(roomPath), ob = readFile(objectPath), pb = readFile(patternPath);
    downwell::RoomPack rooms(rb.data(), rb.size());
    downwell::ObjectPack objects(ob.data(), ob.size());
    downwell::BuilderPatternPack patterns(pb.data(), pb.size());
    downwell::PackedRoom room;
    require(rooms.findRoom(11, room), "natural route has decoded main room");
    for (unsigned seed : {99u, 4u}) {
        downwell::StartupRuntime reference(downwell::PlatformProfile::nonDesktop,
            &rooms, &objects, seed, &patterns);
        reference.skipToGameForTesting();
        AmigaPortRuntime game(300, false, 1201 - 176, true);
        game.initializeEntryInput(false);
        LiveSceneWorld live(patterns, objects);
        live.bind(game, rooms, room, false, &reference);
        int target = 240;
        unsigned updates = 0;
        for (; updates < 2500 && !game.levelResultRequested() && !game.dead(); ++updates) {
            const int x = 240 + game.player().offsetX(), y = 176 + game.player().offsetY();
            if (updates % 16 == 0) {
                int best = -1000000;
                for (int candidate = 180; candidate <= 300; candidate += 8) {
                    int floor = amigaResolveTerrainLanding(&live.collision, candidate - 240, y - 176 + 8);
                    if (floor == downwell::gameplayNoLanding) floor = y + 400 - 176;
                    const int score = std::min(floor + 176 - y, 400) * 4 - std::abs(candidate - x);
                    if (score > best) { best = score; target = candidate; }
                }
            }
            downwell::Input input;
            input.left = x > target + 2; input.right = x < target - 2;
            input.fire = updates % 40 == 0 || updates % 40 == 3;
            game.step(downwell::packReplayInput(input)); reference.step(input, false);
            parityFrame = updates;
            compareInitialGameplay(game, reference);
        }
        require(!game.dead() && game.levelResultRequested() && reference.levelResultRequested(),
            "input-only route reaches a naturally generated exit alive on both adapters");
        require(game.enterLevelClear(0, false) && reference.enterLevelClear(0, false),
            "natural exit transfers to upgrade scene");
        bool heartA = false, heartB = false;
        unsigned clearUpdates = 0;
        for (; clearUpdates < 400 && !game.nextLevelRequested(); ++clearUpdates) {
            bool fireA = true, fireB = true;
            std::vector<unsigned> eventsA, eventsB;
            require(game.stepLevelClear(fireA, false, false, heartA,
                [&](downwell::LevelClearSelection::Effect e, unsigned v) { eventsA.push_back(unsigned(e)); eventsA.push_back(v); }) &&
                reference.stepLevelClear(fireB, false, false, heartB,
                [&](downwell::LevelClearSelection::Effect e, unsigned v) { eventsB.push_back(unsigned(e)); eventsB.push_back(v); }),
                "natural upgrade sequence advances");
            require(eventsA == eventsB && fireA == fireB && heartA == heartB &&
                game.nextLevelRequested() == reference.nextLevelRequested(), "natural upgrade effects agree");
            for (unsigned i = 0; i < 4; ++i)
                require(game.levelClear().offers.ids[i] == reference.levelClear().offers.ids[i], "natural offers agree");
            require(game.randomDrawCount() == reference.randomDrawCount() &&
                game.randomState().stateIndex() == reference.randomState().stateIndex(), "natural upgrade RNG cursor agrees");
            for (unsigned i = 0; i < 16; ++i)
                require(game.randomState().stateWord(i) == reference.randomState().stateWord(i), "natural upgrade RNG state agrees");
            for (unsigned i = 0; i < 23; ++i)
                require(game.upgrades().levels[i] == reference.upgrades().levels[i], "natural upgrade inventory agrees");
        }
        require(game.nextLevelRequested(), "natural upgrade sequence finishes within budget");
        downwell::Input held; held.fire = true;
        require(game.prepareNextAreaOneLevel(held, 319) && reference.loadNextAreaOneLevel(held),
            "natural route enters second cavern");
        require(game.runStage().level == 2 && reference.runStage().level == 2,
            "natural route advances exactly one level");
        live.bind(game, rooms, room, true);
        for (unsigned tick = 0; tick < 120; ++tick) {
            downwell::Input input;
            input.fire = tick < 4;
            game.step(downwell::packReplayInput(input)); reference.step(input, false);
            parityFrame = tick;
            compareInitialGameplay(game, reference);
        }
        std::cout << "Natural exit seed=" << seed << " gameplay=" << updates
                  << " upgrade=" << clearUpdates << " reentry=120 passed\n";
        if (seed == 4) {
            bool reached = false;
            unsigned shopUpdates = 0;
            for (; shopUpdates < 1500 && !game.dead() && !game.levelResultRequested(); ++shopUpdates) {
                const unsigned tick = shopUpdates;
                const int x = 240 + game.player().offsetX(), y = 176 + game.player().offsetY();
                if (tick % 16 == 0) {
                    int best = -1000000;
                    for (int candidate = 180; candidate <= 300; candidate += 8) {
                        int floor = amigaResolveTerrainLanding(&live.collision, candidate - 240, y - 176 + 8);
                        if (floor == downwell::gameplayNoLanding) floor = y + 400 - 176;
                        const int score = std::min(floor + 176 - y, 400) * 4 - std::abs(candidate - x);
                        if (score > best) { best = score; target = candidate; }
                    }
                }
                for (const auto& shop : game.shops().instances)
                    if (shop.present && y < shop.y + 12 && y > shop.y - 200) target = shop.x - 6;
                downwell::Input input;
                input.left = x > target + 2; input.right = x < target - 2;
                input.fire = tick % 40 == 0 || tick % 40 == 3;
                game.step(downwell::packReplayInput(input)); reference.step(input, false);
                parityFrame = tick;
                compareInitialGameplay(game, reference);
                if (game.shops().interactable) { reached = true; ++shopUpdates; break; }
            }
            require(reached && reference.shops().interactable, "ordinary input reaches the generated second-level shop");
            auto shopTick = [&](bool fire, bool left = false) {
                downwell::Input input; input.fire = fire; input.left = left;
                game.step(downwell::packReplayInput(input)); reference.step(input, false);
                compareInitialGameplay(game, reference);
                require(game.shops().noControl == reference.shops().noControl &&
                    game.shops().allocatedCount == reference.shops().allocatedCount, "natural shop ownership agrees");
                for (unsigned i = 0; i < downwell::ShopWorld::capacity; ++i) {
                    const auto& a = game.shops().instances[i]; const auto& b = reference.shops().instances[i];
                    require(a.present == b.present && a.menu.active == b.menu.active && a.menu.cursor == b.menu.cursor,
                        "natural shop menu agrees");
                    if (!a.allocated) continue;
                    for (unsigned j = 0; j < 3; ++j)
                        require(a.stock.items[j].upgrade == b.stock.items[j].upgrade &&
                            a.stock.items[j].price == b.stock.items[j].price &&
                            a.stock.items[j].remaining == b.stock.items[j].remaining, "unmodified natural stock agrees");
                }
            };
            shopTick(false); shopTick(true);
            require(game.shops().noControl, "natural counter opens shop instead of jumping");
            const downwell::ShopWorld::Instance* activeShop = nullptr;
            for (const auto& shop : game.shops().instances) if (shop.menu.active) activeShop = &shop;
            require(activeShop != nullptr, "generated shop owns active menu");
            const auto originalStock = activeShop->stock.items[0];
            const unsigned money = game.currency();
            shopTick(true); // Held opening input must remain filtered.
            require(activeShop->stock.items[0].remaining == originalStock.remaining && game.currency() == money,
                "held opening input neither purchases nor spends naturally collected gems");
            shopTick(false); shopTick(true);
            const bool affordable = money >= originalStock.price;
            require(activeShop->stock.items[0].remaining == originalStock.remaining - unsigned(affordable) &&
                game.currency() == money - (affordable ? originalStock.price : 0),
                "natural purchase response preserves or deducts actual price and stock");
            shopTick(false, true); // Cursor 1 -> Leave.
            shopTick(true);
            require(!game.shops().noControl, "natural shop Leave returns movement ownership");
            const int sideCamera = game.cameraOffsetX();
            unsigned returnUpdates = 0;
            for (; returnUpdates < 240; ++returnUpdates) {
                const int x = 240 + game.player().offsetX();
                shopTick(returnUpdates % 40 == 0, x > 240);
                if (240 + game.player().offsetX() < 280 && game.cameraOffsetX() == 0) break;
            }
            std::cout << "Natural shop seed=4 approach=" << shopUpdates << " affordable=" << affordable
                      << " gems=" << money << " price=" << originalStock.price << " return=" << returnUpdates
                      << " camera=" << sideCamera << "->" << game.cameraOffsetX() << std::endl;
            require(sideCamera != 0 && returnUpdates < 240 && !game.dead(),
                "natural shop return releases side camera and reaches main shaft alive");
        }
    }
}

int main(int argc, char** argv) {
    if (argc >= 4 && std::getenv("DOWNWELL_VERIFY_NATURAL_EXIT")) {
        testNaturalExit(argv[1], argv[2], argv[3]);
        return 0;
    }
    {
        AmigaSceneRuntime revisited;
        require(!revisited.enterResultsDestination(downwell::ResultsDestination::none),
                "no results destination leaves scene unchanged");
        for (int visit = 0; visit < 3; ++visit) {
            require(revisited.enterResultsDestination(downwell::ResultsDestination::styleSelect),
                    "results can reenter style selection");
            require(revisited.styleYOffset() == -128 && revisited.styleDitherY() == 80 &&
                    revisited.styleTileY() == 0 && revisited.styleCursor() == 0 &&
                    revisited.styleSmoothX() == 0 && !revisited.styleSelected() &&
                    revisited.stylePlayerDescent() == 0 && revisited.styleVerticalDitherY() == -320,
                    "style reentry resets all presentation and confirmation state");
            for (int frame = 0; frame < 150; ++frame) revisited.step({});
            downwell::Input confirm;
            confirm.fire = true;
            revisited.step(confirm);
            require(revisited.styleSelected(), "reentered default style accepts confirmation");
            for (int frame = 0; frame < 150; ++frame) revisited.step(confirm);
            require(revisited.scene() == AmigaScene::gameplay, "reentered style reaches gameplay");
            revisited.enterResultsDestination(downwell::ResultsDestination::styleSelect);
            for (int frame = 0; frame < 150; ++frame) revisited.step(confirm);
            require(!revisited.styleSelected(), "held confirmation is not a new press after room change");
            revisited.enterResultsDestination(downwell::ResultsDestination::surface);
            require(revisited.scene() == AmigaScene::menu && revisited.menuPlayerNapping() &&
                    revisited.menuPlayerVisible() && revisited.menuPlayerX() == 112 &&
                    revisited.menuPlayerY() == 512 && revisited.menuCameraX() == 112 &&
                    revisited.menuCameraY() == 512 && !revisited.menuEnterSequenceActive() &&
                    !revisited.menuTeleportActive() && revisited.menuTitleFrame() == 0,
                    "surface reentry resets garden state");
            downwell::Input walk;
            walk.right = true;
            for (int frame = 0; frame < 100; ++frame) revisited.step(walk);
            require(!revisited.menuPlayerNapping(), "reentered garden wakes and moves");
            require(revisited.enterResultsDestination(downwell::ResultsDestination::gameplay) &&
                    revisited.scene() == AmigaScene::gameplay && revisited.splashStage() == 0,
                    "retry bypasses publishers and style selection");
        }
    }
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
    if (argc >= 4) {
        testDecodedSceneParity(argv[1], argv[2], argv[3]);
        testDecodedSceneParity(argv[1], argv[2], argv[3], true);
    }
    std::cout << "Amiga scene runtime tests passed\n";
    return 0;
}
