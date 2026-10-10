#include "platform/amiga/AmigaSceneRuntime.h"
#include "platform/amiga/AmigaPortRuntime.h"
#include "platform/amiga/AmigaTerrain.h"
#include "platform/amiga/AmigaGameplayGeneration.h"
#include "platform/amiga/AmigaNaturalRouteVerification.h"
#include "platform/amiga/AmigaOverflowProfileEntry.h"
#include "core/BuilderPatternPack.h"
#include "core/ObjectPack.h"
#include "core/EnemyAlarmEvents.h"
#include "core/Replay.h"
#include "core/RoomPack.h"
#include "core/StartupRuntime.h"
#include "core/GameMakerRounding.h"
#include "core/GameplayContactProfileInput.h"
#include "GameplayEncounterCoverage.h"
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
#include <array>
#include <algorithm>
#include <memory>

namespace {
int parityFrame = 0;
downwell::StateDigest nativeTerrainDigest;
unsigned nativeTerrainPeak=0;
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
    AmigaTerrainTiles tiles;
    std::unique_ptr<AmigaGameplayGeneration::TerrainLifetimes> terrainLifetimes{
        new AmigaGameplayGeneration::TerrainLifetimes};
    AmigaGameplayGeneration generation;
    downwell::GameplayDitherAbove adoptedDither;

    LiveSceneWorld(const downwell::BuilderPatternPack& pack,
                   const downwell::ObjectPack& definitions)
        : collision{nullptr, nullptr, &pack, nullptr, 0, nullptr},
          patterns(pack), objects(definitions), generation(builder, pack, definitions, index, tiles,terrainLifetimes.get()) {
        downwell::GameMakerRandom random(31);
        require(!generation.step(random, 8192) && random.drawCount() == 0 &&
            builder.fragmentCount() == 0, "unbound generation rejects without RNG or terrain effects");
    }
    static bool generate(void* opaque, AmigaPortRuntime&,
                         downwell::GameMakerRandom& random, int bottom) {
        auto& world = *static_cast<LiveSceneWorld*>(opaque);
        const bool ok = world.generation.step(random, bottom);
        if(ok)world.checkTerrainIdentities();
        require(world.generation.dirtyFirst >= 0 &&
            world.generation.dirtyFirst <= AmigaGameplayGeneration::worldRows &&
            world.generation.dirtyEnd >= 0 &&
            world.generation.dirtyEnd <= AmigaGameplayGeneration::worldRows,
            "generation damage stays within the backing raster");
        return ok;
    }
    void checkTerrainIdentities()const{
        require(terrainLifetimes->valid(),"native terrain identity owner stays valid");
        nativeTerrainPeak=std::max(nativeTerrainPeak,terrainLifetimes->size());
        nativeTerrainDigest.addU32(terrainLifetimes->size());
        require(terrainLifetimes->visit([&](const auto& id,bool active,bool destroyed){
            nativeTerrainDigest.addI32(id.object);nativeTerrainDigest.addI32(id.x);nativeTerrainDigest.addI32(id.y);
            nativeTerrainDigest.addU32(id.source);nativeTerrainDigest.addI32(id.creationCode);
            nativeTerrainDigest.addBool(active);nativeTerrainDigest.addBool(destroyed);return true;
        }),"capture native terrain identity replay");
        if(runtime->nativeTerrainInstances()==terrainLifetimes.get()){
            require(terrainLifetimes->visit([&](const auto& id,bool,bool){
                int sprite;unsigned index;bool created;
                if(id.object==60){
                    downwell::WallEndStepState state,expected;
                    if(!terrainLifetimes->initialWallState(id.handle,state)||
                        !downwell::wallRoomCreation(id.creationCode,expected)||state.pillar!=expected.pillar)return false;
                }
                return terrainLifetimes->appearance(id.handle,sprite,index,created)&&created&&
                    sprite==(id.object==99?-2:downwell::terrainTileForArea(runtime->runStage().area))&&
                    index==(id.object==93?23u:0u);
            }),"native room and generated terrain retain source Create appearance");
        }
        if(const auto* reference=runtime->terrainInstances()){
            require(reference->size()==terrainLifetimes->size(),"native identities match scalar birth count");
            unsigned ordinal=0;
            require(terrainLifetimes->visit([&](const auto& id,bool active,bool destroyed){
                const auto* r=reference->find(reference->handle(ordinal++));
                return r&&r->objectId==id.object&&r->x==id.x&&r->y==id.y&&r->sourceInstanceId==id.source&&
                    r->creationCodeId==id.creationCode&&r->active==active&&r->destroyRequested==destroyed;
            }),"native run identities preserve scalar source birth order and state");
        }
    }
    void bind(AmigaPortRuntime& port, const downwell::RoomPack& rooms,
              const downwell::PackedRoom& room, bool continuing = false,
              const downwell::StartupRuntime* entry = nullptr) {
        runtime = &port;
        tiles.clear();
        collision.index = nullptr; collision.runtime = nullptr; collision.fragmentCount = 0;
        require(amigaBuildTerrainIndex(collision, index), "live scene index initializes empty");
        downwell::PreparedRoomTerrain prepared;
        require(prepared.prepare(rooms,room),"live scene admits immutable room terrain");
        collision.index = &index;
        collision.runtime = &port;
        const int emptyBlock = 0;
        port.configureBreakables(&emptyBlock, &emptyBlock, 0);
        if (entry) {
            require(builder.resetFromRoomEntry(entry->mainBuilderState()),
                    "live room entry imports generator before any terrain emission");
            port.configureEnemyRandom(entry->randomState());
            downwell::GameplayDitherAbove::Snapshot dither;
            if(!entry->captureDitherAbove(dither))for(unsigned i=0;i<entry->activeInstanceCount();++i){
                const auto& r=entry->instanceAt(i);if(r.objectId!=61)continue;
                std::cerr<<"DITHER adoption rejected: "<<r.instanceId<<" room="<<r.roomId<<" sprite="<<r.spriteId
                    <<" mask="<<r.maskId<<" depth="<<r.depth<<" scale="<<r.scaleX<<','<<r.scaleY
                    <<" angle="<<r.rotation<<" pending="<<r.activationPending<<'\n';
            }
            require(entry->captureDitherAbove(dither),"capture stored controller builtins");
            if(!port.ditherAbove())require(port.bindDitherAbove(adoptedDither),"unindexed scene owns adopted controller");
            require(generation.adoptRoom(port,rooms,room,dither,&prepared),"adopted entry does not repeat field Create");
            const auto* imported=port.ditherAbove()->find(port.ditherAbove()->handle());
            require(dither.sourceId?(imported&&imported->sourceId==dither.sourceId&&
                imported->x.bits==dither.x.bits&&imported->y.bits==dither.y.bits):!imported,
                "room handoff retains controller presence and published coordinates");
        } else if (continuing) {
            require(generation.loadRoomWithStartField(port,rooms,room,&prepared,nullptr,
                [](AmigaPortRuntime& world,AmigaAreaOneTerrainBuilder& builder,
                   const downwell::PackedRoomInstance& instance,void*){
                    return world.resetAreaOneBuilder(builder,instance.x,instance.y);
                }),"continuation builder Create and field follow authored room order");
        } else {
            require(generation.loadRoomWithStartField(port,rooms,room,&prepared,nullptr,
                [](AmigaPortRuntime& world,AmigaAreaOneTerrainBuilder& builder,
                   const downwell::PackedRoomInstance& instance,void*){
                    require(instance.x==-16&&instance.y==704,"native checkpoint uses authored builder position");
                    downwell::GameMakerRandom random;
                    builder.resetInitialCheckpointBeforeEntrance(random);
                    world.configureEnemyRandom(random);return true;
                }),"native initial checkpoint and field follow authored room order");
        }
        generation.dirtyFirst = 0; generation.dirtyEnd = AmigaGameplayGeneration::worldRows;
        checkTerrainIdentities();
        generation.bind(port,std::getenv("DOWNWELL_SCALAR_TERRAIN")==nullptr);
        require(generation.dirtyFirst == AmigaGameplayGeneration::worldRows && generation.dirtyEnd == 0,
            "room binding retires previous generation damage");
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
    // objWell Create emits four object-98 children before object 317. A
    // guessed dynamic ID can therefore name a wall, not the title. Resource
    // identity is fixed by the VM; this comparison must not guess numeric IDs.
    const downwell::InstanceRecord* title = nullptr;
    for(std::size_t i=0;i<reference.activeInstanceCount();++i){
        const auto& candidate=reference.instanceAt(i);
        if(candidate.roomId==downwell::StartupRuntime::menuRoomId&&candidate.objectId==317){
            require(title==nullptr,"exactly one decoded garden title exists");
            title=&candidate;
        }
    }
    require(title != nullptr, "decoded menu title instance remains available");
    if(amiga.menuTitleFrame()!=static_cast<int>(title->imageIndex)||amiga.menuTitleY()!=title->y)
        std::cerr<<"TITLE native_frame="<<amiga.menuTitleFrame()<<" native_y="<<amiga.menuTitleY()
            <<" host_index="<<title->imageIndex<<" host_y="<<title->y<<" tick="<<amiga.tick()<<'\n';
    require(amiga.menuTitleFrame() ==
                static_cast<int>(title->imageIndex) &&
            amiga.menuTitleY() == title->y,
            "menu title animation follows decoded runtime");
    if (amiga.menuEnterSequenceActive() != reference.enterSequenceActive())
        std::cerr << "WELL ENTRY frame=" << parityFrame
                  << " active native=" << amiga.menuEnterSequenceActive()
                  << " host=" << reference.enterSequenceActive()
                  << " title=" << amiga.menuTitleFrame() << ',' << title->imageIndex << '\n';
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
            require(native.maskId == enemy.maskId &&
                    native.batActivity.initialized == enemy.batActivity.initialized &&
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
                      << " SDL=" << referenceX << ',' << referenceY << ',' << referenceVX << ',' << referenceVY
                      << " horizontal=" << native.frogHorizontal.position << ',' << enemy.frogHorizontal.position
                      << '/' << native.frogHorizontal.velocity << ',' << enemy.frogHorizontal.velocity
                      << " vertical=" << native.preciseY*5+native.gravityPositionRemainder << ',' << enemy.frogVertical.position
                      << '/' << native.speedY*5+native.gravityRemainder << ',' << enemy.frogVertical.velocity
                      << " ground=" << native.grounded << ',' << enemy.frogVertical.grounded
                      << " hit=" << native.frogHit.pending << ',' << enemy.frogHit.pending
                      << '/' << native.frogHit.ready << ',' << enemy.frogHit.ready
                      << '/' << native.frogHit.remaining << ',' << enemy.frogHit.remaining << '\n';
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
    const auto* nativeStart=amiga.startTimeField();const auto* hostStart=reference.startTimeField();
    require(bool(nativeStart)==bool(hostStart),"both worlds agree on start-field lifetime");
    if(nativeStart)require(nativeStart->x==hostStart->x&&nativeStart->y==hostStart->y&&
        nativeStart->radius.equal(hostStart->radius),"both worlds agree on start-field coordinates and Draw radius");
    require(amiga.timeFields().size()==reference.timeFields().size(),
        "both worlds retain every ordinary time-field birth");
    for(unsigned i=0;i<amiga.timeFields().size();++i){
        const auto& a=*amiga.timeFields().at(i);const auto& b=*reference.timeFields().at(i);
        require(a.x==b.x && a.y==b.y && a.radius.equal(b.radius),
            "ordinary field coordinates, creation order and radius agree");
    }
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
            "enemy populations match after births and offscreen destruction throughout this tape");
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
            if (native.active != enemy.active || native.health != enemy.health)
                std::cerr << "Actor " << enemy.objectId << " active=" << native.active << ',' << enemy.active
                    << " sprite=" << native.spriteId << ',' << enemy.spriteId
                    << " xy=" << native.x+240 << ',' << native.y+176 << '/' << enemy.x << ',' << enemy.y
                    << " scale=" << native.facing << ',' << native.imageScaleY() << '/'
                    << enemy.scaleX << ',' << enemy.scaleY << '\n';
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
            if(240 + native.x != enemy.x || 176 + native.y != enemy.y)
                std::cerr<<"Actor pixel mismatch object="<<enemy.objectId<<" slot="<<native.identitySlot
                    <<" native="<<240+native.x<<','<<176+native.y<<" host="<<enemy.x<<','<<enemy.y
                    <<" precise="<<std::hexfloat<<240+native.preciseX/256.0F<<','<<176+native.preciseY/256.0F
                    <<'/'<<enemy.preciseX<<','<<enemy.preciseY<<std::defaultfloat<<'\n';
            require(240 + native.x == enemy.x && 176 + native.y == enemy.y,
                    "paired actors publish identical pixel positions");
            if (enemy.objectId == 113 && enemy.active && !enemy.enemyStepEligible)
                require(enemy.spriteId == 166 && enemy.imageIndex == 0,
                        "new frog retains serialized Create sprite/frame before its first Step");
            else if (enemy.objectId == 113 && enemy.active && enemy.frogHit.remaining != 0)
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
                bullet.gemHigh == host.gemHigh && bullet.animation.index.bits==host.animation.index.bits,
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
    downwell::DeferredCreateInstance fieldCommand{82,240,900};
    fieldCommand.setTimeFieldRadiusWord(0xffffffffu);
    require(!world.createGeneratedTerrain(fieldCommand,terrain)&&terrainCalls==0&&
        world.timeFields().size()==0&&world.effectsSucceeded(),
        "terrain-only transaction rejects unrelated objects without invoking backend or changing world");
    const auto fieldDraws=world.randomDrawCount();
    require(world.createGeneratedInstance(fieldCommand,objects,terrain) && terrainCalls==0 &&
        world.timeFields().size()==1 && world.timeFields().at(0)->x==240 &&
        world.timeFields().at(0)->y==900 && world.timeFields().at(0)->radius.equal(
            downwell::OrdinaryTimeField::radiusFromWord(0xffffffffu)) && world.randomDrawCount()==fieldDraws,
        "time field registration adopts Create without terrain painting or a second RNG draw");
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
    if(std::getenv("DOWNWELL_VERIFY_SMOKE_TERRAIN")) {
        downwell::StartupRuntime host(downwell::PlatformProfile::desktop,&rooms,&objects,2,&patterns);
        host.skipToGameForTesting();
        AmigaPortRuntime native(0);
        static AmigaTerrainIndex contacts;
        AmigaTerrainCollisionContext context{nullptr,nullptr,nullptr,nullptr,0,&native,&contacts};
        require(amigaBuildTerrainIndex(context,contacts),"smoke fixture clears native terrain");
        std::vector<int> boxX,boxY;
        int x=0,y=0;bool found=false;
        for(std::size_t i=0;i<host.activeInstanceCount();++i){
            const auto& wall=host.instanceAt(i);
            if(!wall.active)continue;
            require(amigaAddTerrainObject(contacts,wall.objectId,wall.x,wall.y),"smoke fixture retains actual room instances");
            if(wall.objectId==92){boxX.push_back(wall.x-240);boxY.push_back(wall.y-176);}
            if(wall.objectId==60 && !found){x=wall.x;y=wall.y;found=true;}
        }
        require(found,"smoke fixture has a real wall");
        native.configureBreakables(boxX.data(),boxY.data(),boxX.size());
        native.configureSolidResolver(amigaResolveTerrainSolid,&context,amigaResolveTerrainSegment,
            amigaResolveTerrainPlayerMask,amigaResolveTerrainWallContacts);
        downwell::GameMakerRandom a(91),b(91);
        auto& hs=const_cast<downwell::SmokeEffects<32>&>(host.smokeEffects());
        auto& ns=const_cast<downwell::SmokeEffects<32>&>(native.smokeEffects());
        const auto hh=hs.create(downwell::MotionBinary64::fromInteger(x),downwell::MotionBinary64::fromInteger(y),a);
        const auto nh=ns.create(downwell::MotionBinary64::fromInteger(x),downwell::MotionBinary64::fromInteger(y),b);
        require(hh && nh,"both owners create collision fixtures");
        // Isolate dispatch, not natural smoke motion (covered independently).
        for(auto* effect:{hs.get(hh),ns.get(nh)}) {
            effect->allSet=true;effect->noCollision=false;effect->imageSpeed={};
        }
        const auto initial=hs.get(hh)->speed;
        for(unsigned tick=0;tick<4;++tick){
            host.step(false);native.step(0);
            require(host.effectsSucceeded() && native.effectsSucceeded(),"live contact queries succeed");
            const auto& h=*hs.get(hh);const auto& n=*ns.get(nh);
            require(h.speed.high==n.speed.high && h.speed.low==n.speed.low && h.x.bits==n.x.bits && h.y.bits==n.y.bits,
                "live decoded and native terrain collision responses agree bit-for-bit");
        }
        require(hs.get(hh)->speed.high!=initial.high || hs.get(hh)->speed.low!=initial.low,"live terrain contact changed speed");
        std::cout<<"Live smoke terrain contacts agree across both simulation adapters\n";
        return;
    }

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
            AmigaPortRuntime fresh(300,false,1201-176,true);
            fresh.configureEnemyRandom(roomRandom);
            require(fresh.initializeAreaOneBuilder(restartBuilder,-16,704)&&
                    fresh.randomDrawCount()==restarted.randomDrawCount(),
                    "native room entry includes builder and start-field Create RNG, matching decoded Retry");
            for(unsigned word=0;word<16;++word)require(fresh.randomState().stateWord(word)==restarted.randomState().stateWord(word),
                    "Retry room entry preserves complete RNG state");
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

    {
        downwell::StartupRuntime entry(downwell::PlatformProfile::nonDesktop,&rooms,&objects,42,&patterns);
        entry.skipToGameForTesting();
        entry.configureDitherAboveEnvironment(true,false);
        for(unsigned tick=0;tick<40;++tick)entry.step(0);
        downwell::GameplayDitherAbove::Snapshot state;
        require(entry.captureDitherAbove(state)&&state.sourceId==100854&&state.lowSpec&&!state.ending,
            "capture advanced dither and normalized environment");
        downwell::GameplayDitherAbove actor;
        AmigaPortRuntime imported(300,false,1201-176,true);
        require(imported.bindDitherAbove(actor)&&imported.adoptDitherAbove(state),"adopt advanced controller");
        const auto* r=actor.find(actor.handle());
        require(r&&r->x.bits==state.x.bits&&r->y.bits==state.y.bits&&state.x.bits!=0x44080000u,
            "advanced published position is not the authored 544,-80 position");
        entry.configureDitherAboveEnvironment(true,true);
        entry.step(0);
        require(entry.captureDitherAbove(state)&&!state.sourceId&&state.ending,
            "retired controller stays absent in room snapshot");
        downwell::GameplayDitherAbove absent;
        require(absent.adopt(state)&&!absent.handle().generation,"adoption does not recreate a retired controller");
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
            if(imported.randomDrawCount()!=entry.randomDrawCount()){
                std::cerr<<"ENTRY RNG seed="<<seed<<" tick="<<tick<<" native="<<imported.randomDrawCount()
                    <<" host="<<entry.randomDrawCount()<<'\n';
                for(unsigned i=0;i<imported.enemyCount();++i){const auto& e=imported.enemyAt(i);
                    std::cerr<<" N "<<e.objectId<<' '<<e.x<<','<<e.y<<" active="<<e.active<<" alarms="<<e.alarm0<<','<<e.alarm1<<'\n';}
                for(unsigned i=0;i<entry.activeInstanceCount();++i){const auto& e=entry.instanceAt(i);
                    if(e.objectId>=111&&e.objectId<=119)std::cerr<<" H "<<e.objectId<<' '<<e.x-240<<','<<e.y-176<<" active="<<e.active<<" alarms="<<e.alarms.get(0)<<','<<e.alarms.get(1)<<'\n';}
            }
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
    unsigned expectedBreakables = 0;
    for (const auto& fragment : terrain) {
        for (unsigned row = 0; row < fragment.pattern.rowCount; ++row) {
            std::uint32_t width = 0;
            require(patterns.rowWidth(fragment.pattern, row, width),
                    "forecast pattern row is readable");
            for (unsigned column = 0; column < width; ++column) {
                char symbol = 0;
                require(patterns.cell(fragment.pattern, column, row, symbol),
                        "forecast pattern cell is readable");
                if (symbol != 'B') continue;
                require(expectedBreakables < breakableCount,
                        "each forecast B cell fits native breakable storage");
                const int x = fragment.reversed ? 320 - (column + 1) * 16
                                                : 160 + (column + 1) * 16;
                require(breakableX[expectedBreakables] == x - 240 &&
                        breakableY[expectedBreakables] == fragment.y + int(row) * 16 - 176,
                        "forecast B cells retain serialized positions and order");
                ++expectedBreakables;
            }
        }
    }
    require(expectedBreakables > 0 && breakableCount == expectedBreakables,
            "forecast breakables contain exactly the selected pattern B cells");
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
                gameplay.levelResultRequested() == (tick >= 94) &&
                reference.levelResultRequested() == (tick >= 94),
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
            const auto oldPlayerHandle=gameplay.playerHandle();
            require(gameplay.enterLevelClear(0, false) && reference.enterLevelClear(0, false),
                "both adapters consume the exit request at an explicit scene boundary");
            require(!gameplay.findPlayer(oldPlayerHandle),"old player lifetime leaves before Result Create");
            require(reference.roomId()==downwell::StartupRuntime::resultRoomId&&
                !reference.instance(100855)&&reference.instance(100065)&&
                reference.instance(100065)->roomId==downwell::StartupRuntime::resultRoomId&&
                reference.instance(102522)&&reference.instance(102522)->objectId==325&&
                reference.instance(102522)->x==80&&reference.instance(102522)->y==142,
                "stored Result room retains controller and admits authored Result after player retirement");
            for(std::size_t i=0;i<reference.activeInstanceCount();++i)
                require(reference.instanceAt(i).persistent||reference.instanceAt(i).instanceId==102522,
                    "no nonpersistent gameplay record survives Result entry");
            const auto nativeTick = gameplay.tick();
            const auto hostTick = reference.tick();
            gameplay.step(0); reference.step({}, false);
            require(gameplay.tick() == nativeTick && reference.tick() == hostTick,
                "old gameplay dispatcher cannot advance while the clear room owns the runtime");
            bool nativeHeart = false, hostHeart = false;
            std::uint32_t resultWipeId=0;
            int roomSpeed=30;
            auto* persistent=const_cast<downwell::InstanceRecord*>(reference.instance(100065));
            persistent->alarms.set(0,1);persistent->controllerSlow=30;
            for (unsigned tick = 1; tick <= 210; ++tick) {
                bool nativeConfirm = true, hostConfirm = true;
                std::vector<unsigned> nativeEvents, hostEvents;
                const bool right = tick == 61 || tick == 62;
                const bool left = tick == 75;
                const bool nativeOkay=gameplay.stepLevelClear(nativeConfirm, left, right, nativeHeart,
                    [&](downwell::LevelClearSelection::Effect event, unsigned value) {
                        nativeEvents.push_back(unsigned(event)); nativeEvents.push_back(value);
                    });
                const auto missing=reference.unimplementedCodeCalls();
                const bool hostOkay=reference.withMomentDelayRoomSpeed(roomSpeed,[&]{return reference.stepLevelClear(hostConfirm, left, right, hostHeart,
                    [&](downwell::LevelClearSelection::Effect event, unsigned value) {
                        hostEvents.push_back(unsigned(event)); hostEvents.push_back(value);
                        if(event==downwell::LevelClearSelection::Effect::dither){
                            require(roomSpeed==60&&reference.requestMomentDelay()&&roomSpeed==30,
                                "Result Alarm 0 can arm an already-visited controller alarm");
                        }
                    });});
                if(!nativeOkay||!hostOkay)std::cerr<<"Result alarm failure tick="<<tick<<" native="<<nativeOkay
                    <<" host="<<hostOkay<<" missing="<<reference.unimplementedCodeCalls()-missing
                    <<" speed="<<roomSpeed<<" controller0="<<persistent->alarms.get(0)<<std::endl;
                require(nativeOkay&&hostOkay,"both result adapters update successfully");
                if(tick==121)require(roomSpeed==30&&persistent->alarms.get(0)==1,
                    "already-visited controller keeps its pending restoration");
                if(tick==1||tick==122)require(roomSpeed==60&&persistent->alarms.get(0)==0,
                    "active controller restores speed in the next Result alarm phase");
                const auto& a = gameplay.levelClear();
                const auto& b = reference.levelClear();
                std::int32_t resultY;
                require(b.motion.instanceY(resultY)&&reference.instance(102522)->y==resultY&&
                    reference.instance(102522)->preciseY==resultY,
                    "stored Result publishes the rounded yy builtin every Step");
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
                    gameplay.nextLevelRequested() == (tick == 210) &&
                    reference.nextLevelRequested() == (tick == 210),
                    "result grants, presentation and departure requests match across adapters");
                unsigned wipes=0;
                for(std::size_t i=0;i<reference.activeInstanceCount();++i){
                    const auto& r=reference.instanceAt(i);if(r.objectId!=68)continue;
                    ++wipes;
                    if(!resultWipeId)resultWipeId=r.instanceId;
                    require(r.instanceId==resultWipeId&&r.roomId==downwell::StartupRuntime::resultRoomId&&
                        r.x==reference.exitDither().x&&r.y==reference.exitDither().y&&
                        r.alarms.get(0)==reference.exitDither().alarm0,
                        "registered Result wipe retains identity and publishes shared movement/alarm state");
                }
                require(wipes==unsigned(tick>=121),"Result wipe is born once at the source Alarm 0 boundary");
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
            require(resultWipeId&&!reference.instance(resultWipeId),"Result wipe retires when leaving its room");
            require(!gameplay.findPlayer(oldPlayerHandle)&&gameplay.findPlayer(gameplay.playerHandle())&&
                    gameplay.playerHandle().generation>oldPlayerHandle.generation,
                    "next level retires the old player and publishes a fresh lifetime");
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
                    // Entrances can contain thin platforms, not just boxes.
                    // Walk off their edges instead of firing forever at centre.
                    const int x = 240 + gameplay.player().offsetX();
                    if (x < 192) { input.left = false; input.right = true; }
                    else if (x > 288) { input.left = true; input.right = false; }
                    // Sparse taps allow descent between jumps and shots.
                    input.fire = tick % 60 == 0 || tick % 60 == 3;
                    if (!sawReentryEnemy) {
                        int nearest = 1000;
                        for (unsigned i = 0; i < reference.activeInstanceCount(); ++i) {
                            const auto& block = reference.instanceAt(i);
                            if (!block.active || block.objectId != 92 || block.x < 176 || block.x > 288 ||
                                block.y < reference.mainPlayerY() + 8 ||
                                block.y > reference.mainPlayerY() + 80) continue;
                            const int distance = std::abs(x - block.x);
                            if (distance >= nearest) continue;
                            nearest = distance;
                            input.left = x > block.x + 2;
                            input.right = x < block.x - 2;
                        }
                    }
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
            if (!sawReentryEnemy) {
                for (unsigned i = 0; i < reference.activeInstanceCount(); ++i) {
                    const auto& item = reference.instanceAt(i);
                    if (item.active && (item.objectId == 60 || item.objectId == 92 || item.objectId == 93) &&
                        std::abs(item.x - reference.mainPlayerX()) < 80 &&
                        std::abs(item.y - reference.mainPlayerY()) < 64)
                        std::cerr << "REENTRY SOLID object=" << item.objectId
                                  << " x=" << item.x << " y=" << item.y << '\n';
                }
            }
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
    bool routeFrogHit = false;
    unsigned retiredCompared = 0;
    for (int tick = 0; tick < 1800; ++tick) {
        parityFrame += 1;
        downwell::Input traverse;
        traverse.right = (tick / 90) % 2 == 0;
        traverse.left = !traverse.right;
        // Jump, release, then fire in the air: generated breakable floors
        // cannot be traversed by a tape that only ever presses while grounded.
        traverse.fire = tick % 40 == 0 || tick % 40 == 3;
        // Keep this encounter tape in the shaft; side-room navigation has
        // separate coverage and a periodic sweep can become trapped there.
        if (reference.mainPlayerX() < 192) {
            traverse.left = false; traverse.right = true;
        } else if (reference.mainPlayerX() > 288) {
            traverse.left = true; traverse.right = false;
        }
        if (!routeFrogHit) {
            for (unsigned i = 0; i < reference.activeInstanceCount(); ++i) {
                const auto& enemy = reference.instanceAt(i);
                if (enemy.objectId != 113 || !enemy.active || enemy.health <= 0 ||
                    enemy.y < reference.mainPlayerY() + 20 ||
                    enemy.y > reference.mainPlayerY() + 260) continue;
                traverse.left = reference.mainPlayerX() > enemy.x + 2;
                traverse.right = reference.mainPlayerX() < enemy.x - 2;
                traverse.fire = tick % 10 < 5;
                break;
            }
        }
        struct PairedLifetime { std::uint32_t storedId, nativeKey; };
        std::vector<PairedLifetime> beforeStep;
        unsigned beforeOrdinal = 0;
        for (unsigned i=0;i<reference.activeInstanceCount();++i) {
            const auto& enemy=reference.instanceAt(i);
            if (!amigaEnemyHealth(enemy.objectId)) continue;
            require(beforeOrdinal<gameplay.enemyCount(), "pre-Step lifetime pairing is complete");
            const auto& native=gameplay.enemyAt(beforeOrdinal++);
            require(native.objectId==enemy.objectId, "pre-Step lifetime pairing preserves creation order");
            if (enemy.active && enemy.health>0)
                beforeStep.push_back({enemy.instanceId,native.lifetimeId});
        }
        gameplay.step(downwell::packReplayInput(traverse));
        reference.step(traverse, traverse.fire && !traverseFire);
        traverseFire = traverse.fire;
        compareInitialGameplay(gameplay, reference);
        for (const auto& previous:beforeStep) {
            if (reference.hasInstance(previous.storedId)) continue;
            require(!gameplay.findEnemy(previous.nativeKey),
                    "offscreen destruction invalidates both previously paired lifetimes on the same frame");
            ++retiredCompared;
        }
        for (unsigned i = 0; i < reference.activeInstanceCount(); ++i) {
            const auto& enemy = reference.instanceAt(i);
            if (enemy.objectId == 113 && enemy.active && enemy.health > 0 &&
                enemy.frogHit.remaining != 0) routeFrogHit = true;
        }
        if (std::getenv("DOWNWELL_TRACE_TRAVERSAL") && tick % 60 == 0)
            std::cerr << "TRAVERSE tick=" << tick
                      << " player=" << gameplay.player().offsetX() << ',' << gameplay.player().offsetY()
                      << " camera=" << gameplay.cameraOffsetY()
                      << " start=" << postLandingCameraY
                      << " grounded=" << gameplay.player().grounded()
                      << " health=" << reference.mainPlayerHealth()
                      << " ammo=" << reference.mainAmmo() << '\n';
        for (unsigned enemy = 0; enemy < gameplay.enemyCount(); ++enemy)
            sawAwakeEnemy = sawAwakeEnemy || gameplay.enemyAt(enemy).awake;
        if (routeFrogHit && sawAwakeEnemy && sawExtendedEnemyAnimation && sawPairedFrogAir && sawPairedFrogGround &&
            retiredCompared >= 2 &&
            gameplay.cameraOffsetY() >= postLandingCameraY + 256)
            break;
    }
    require(gameplay.cameraOffsetY() >= postLandingCameraY + 256,
            "post-landing tape scrolls at least one circular-buffer page");
    require(sawAwakeEnemy,
            "post-landing tape reaches generated native enemy simulation");
    require(routeFrogHit,
            "each traversal reaches a real surviving frog hit before retirement");
    unsigned enemyOrdinal = 0;
    for (unsigned i = 0; i < reference.activeInstanceCount(); ++i) {
        const auto& enemy = reference.instanceAt(i);
        if (enemy.objectId != 111 && enemy.objectId != 112 && enemy.objectId != 113 &&
            enemy.objectId != 116 && enemy.objectId != 117 && enemy.objectId != 119) continue;
        require(enemyOrdinal < gameplay.enemyCount() &&
                gameplay.enemyAt(enemyOrdinal).objectId == enemy.objectId,
                "generated enemy prefix preserves creation order across targets");
        ++enemyOrdinal;
    }
    if (retiredCompared < 2 && std::getenv("DOWNWELL_TRACE_TRAVERSAL")) {
        std::cerr << "RETIREMENTS count=" << retiredCompared
                  << " viewY=" << reference.mainCameraViewY() << '\n';
        for (unsigned i = 0; i < reference.activeInstanceCount(); ++i) {
            const auto& enemy = reference.instanceAt(i);
            if (enemy.objectId >= 111 && enemy.objectId <= 119)
                std::cerr << "ENEMY object=" << enemy.objectId << " y=" << enemy.y
                          << " active=" << enemy.active << " health=" << enemy.health
                          << " pending=" << enemy.activationPending << '\n';
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
        unsigned candleHits = 0, candleCountdowns = 0, candleExpiries = 0;
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
            ? static_cast<unsigned>(std::atoi(std::getenv("DOWNWELL_COMBAT_TICKS"))) : 1800;
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
            if (reference.mainPlayerX() < 192) {
                input.left = false; input.right = true;
            } else if (reference.mainPlayerX() > 288) {
                input.left = true; input.right = false;
            }
            int targetY = static_cast<int>(reference.mainPlayerY()) + 261;
            for (unsigned i = 0; i < reference.activeInstanceCount(); ++i) {
                const auto& enemy = reference.instanceAt(i);
                if (!amigaEnemyHealth(enemy.objectId) || !enemy.active || enemy.health <= 0 ||
                    enemy.x < 176 || enemy.x > 304 ||
                    enemy.y < reference.mainPlayerY() + 20 || enemy.y >= targetY) continue;
                targetY = enemy.y;
                input.left = reference.mainPlayerX() > enemy.x + 2;
                input.right = reference.mainPlayerX() < enemy.x - 2;
            }
            // Exercise alternate press phases of the same firing cycle.
            const int firePhase = std::getenv("DOWNWELL_COMBAT_FIRE_PHASE")
                ? std::atoi(std::getenv("DOWNWELL_COMBAT_FIRE_PHASE")) : 4;
            input.fire = (tick + firePhase) % 10 < 5;
            if ((std::getenv("DOWNWELL_VERIFY_DEATH")||candleFixture) && gameplay.enemiesDefeated() > 0) {
                // Once reward coverage exists, seek a nearby enemy and stop
                // shooting during airborne approach. Keep grounded firing to
                // break through platforms when the threat is below our feet.
                const downwell::InstanceRecord* threat = nullptr;
                float nearest = 96;
                for (unsigned i = 0; i < reference.activeInstanceCount(); ++i) {
                    const auto& enemy = reference.instanceAt(i);
                    if (!enemy.active || enemy.health <= 0 || !amigaEnemyHealth(enemy.objectId)) continue;
                    const float distance = std::fabs(enemy.x-reference.mainPlayerX()) +
                                           std::fabs(enemy.y-reference.mainPlayerY());
                    if (distance < nearest) { nearest=distance; threat=&enemy; }
                }
                if (threat) {
                    input.left = reference.mainPlayerX() > threat->x;
                    input.right = reference.mainPlayerX() < threat->x;
                    if (!reference.mainPlayerGrounded() ||
                        std::fabs(threat->y-reference.mainPlayerY()) < 16) input.fire=false;
                }
                // Do not let enemy pursuit oscillate forever over a platform.
                // Continue a real left/right firing sweep after the encounter
                // phase so damage fixtures explore newly generated rows.
                if(tick>=1800){
                    input.left=(tick/90)%2==0;
                    input.right=!input.left;
                    input.fire=tick%16==0||tick%16==3;
                }
            }
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
            const unsigned immunityBefore = gameplay.invulnerability();
            const int healthBefore = gameplay.health();
            gameplay.step(downwell::packReplayInput(input));
            reference.step(input, input.fire && (tick + firePhase) % 10 == 0);
            parityFrame = gameplay.tick();
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
                require(bullet.animation.index.bits==hostBullet.animation.index.bits,
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
            std::vector<std::array<long, 12>> nativeGems, referenceGems;
            for (unsigned i = 0; i < gameplay.gemCapacity; ++i) {
                const auto& gem = gameplay.gemAt(i);
                if (gem.active) nativeGems.push_back({static_cast<long>(gem.age), gem.amount,
                    static_cast<long>(gem.motion.animation.speedBits()), static_cast<long>(gem.motion.animation.phaseBits()),
                    gem.motion.x, gem.motion.y, gem.motion.vx, gem.motion.vy, gem.motion.attracted,
                    gem.motion.attractorAtBirth,gem.x,gem.y});
            }
            for (unsigned i = 0; i < reference.mainCurrencyCount(); ++i) {
                const auto& gem = reference.mainCurrencyAt(i);
                if(gem.age){
                    require(gem.x==240+downwell::GemMotion::rounded(gem.motion.x) &&
                            gem.y==176+downwell::GemMotion::rounded(gem.motion.y),
                            "currency Step publishes source-rounded pixels, not fractional motion");
                }
                referenceGems.push_back({static_cast<long>(gem.age), gem.amount,
                    static_cast<long>(gem.motion.animation.speedBits()), static_cast<long>(gem.motion.animation.phaseBits()),
                    gem.motion.x, gem.motion.y, gem.motion.vx, gem.motion.vy, gem.motion.attracted,
                    gem.motion.attractorAtBirth,static_cast<long>(gem.x-240),static_cast<long>(gem.y-176)});
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
            if (candleFixture) {
                const unsigned immunity = gameplay.invulnerability();
                if (immunity == 240) {
                    require(immunityBefore <= 1 && gameplay.health() < healthBefore,
                            "Candle interval starts only on real damage after prior immunity expires");
                    sawCandleDamage = true;
                    ++candleHits;
                } else if (immunityBefore > 0) {
                    require(immunity + 1 == immunityBefore,
                            "Candle immunity counts down once per live update");
                    ++candleCountdowns;
                    if (immunity == 0) ++candleExpiries;
                }
            }
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
        if (candleFixture) {
            require(candleHits >= 2 && candleExpiries >= 1 && candleCountdowns >= 240,
                    "Candle route covers full expiry and a later real damage encounter");
            std::cout << "CANDLE hits=" << candleHits << " countdowns=" << candleCountdowns
                      << " expiries=" << candleExpiries << '\n';
        }
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
        if (std::getenv("DOWNWELL_TRACE_TRAVERSAL"))
            std::cerr << "COMBAT LOCATION player=" << reference.mainPlayerX() << ','
                      << reference.mainPlayerY() << " camera=" << reference.mainCameraViewY()
                      << " noControl=" << reference.shops().noControl
                      << " grounded=" << reference.mainPlayerGrounded()
                      << " frogHit=" << routeFrogHit << '\n';
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

void testNumberedGeneration(const char* roomPath,const char* objectPath,const char* patternPath){
    const auto rb=readFile(roomPath),ob=readFile(objectPath),pb=readFile(patternPath);
    downwell::RoomPack rooms(rb.data(),rb.size());downwell::ObjectPack objects(ob.data(),ob.size());
    downwell::BuilderPatternPack patterns(pb.data(),pb.size());downwell::PackedRoom room;
    require(rooms.findRoom(11,room),"numbered generation room");
    auto entry=std::make_unique<downwell::StartupRuntime>(downwell::PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns);
    entry->skipToGameForTesting();
    std::vector<int> referenceBirths;std::uint64_t referenceDigest=0;
    for(unsigned mode=0;mode<3;++mode){
        auto world=std::make_unique<AmigaPortRuntime>(300,false,1201-176,true);
        auto scattered=std::make_unique<downwell::GameplayScattered>();
        require(world->bindScattered(*scattered),"unindexed generator fixture owns Scattered16 lifetimes");
        auto scene=std::make_unique<LiveSceneWorld>(patterns,objects);
        scene->bind(*world,rooms,room,false,entry.get());
        scene->generation.bind(*world,mode!=0);
        if(mode==0)scene->builder.setBreakablePublisher(nullptr);
        auto random=entry->randomState();
        // Stop before main-pattern generation creates the unregistered
        // spawner 78. The supplied cursor is a fixture checkpoint, not proof
        // of the complete original startup allocation stream.
        struct Cursor {unsigned next=900000;std::vector<int> births;} cursor;
        auto allocate=[&](int object,unsigned& id){
            cursor.births.push_back(object);id=cursor.next++;
            return mode!=2||cursor.births.size()!=3;
        };
        auto shop=[](const auto&,auto&){return false;};
        const bool ok=scene->generation.withSourceCreation(allocate,shop,[&]{
            require(!scene->generation.withSourceCreation(allocate,shop,[]{return true;}),"nested creation scope rejected");
            for(unsigned tick=0;!scene->builder.fragmentCount()&&tick<64;++tick)
                if(!scene->generation.step(random,100000))return false;
            return scene->builder.fragmentCount()==1;
        });
        if(mode==2){
            if(ok||cursor.births.size()!=3||world->effectsSucceeded())
                std::cerr<<"Numbered failure: ok="<<ok<<" allocations="<<cursor.births.size()
                    <<" effects="<<world->effectsSucceeded()<<"\n";
            require(!ok&&cursor.births.size()==3&&!world->effectsSucceeded(),"numbered generation retains failure prefix");
            require(!scene->generation.step(random,100000)&&cursor.births.size()==3,"failed generation cannot replay allocation");
            continue;
        }
        if(!ok){
            auto source=entry->mainBuilderState();auto rng=entry->randomState();
            std::vector<int> emitted;
            for(unsigned tick=0;source.generationCount()<0&&tick<64;++tick){
                downwell::AreaOneBuilderStep result;
                source.stepVisible(patterns,rng,100000,[](void* p,const auto& c){
                    static_cast<std::vector<int>*>(p)->push_back(c.objectId);return true;
                },&emitted,result);
            }
            std::cerr<<"Numbered pattern failed after "<<cursor.births.size()<<" allocations; source objects:";
            for(int object:emitted)std::cerr<<' '<<object;
            std::cerr<<'\n';
        }
        require(ok&&!cursor.births.empty(),"numbered generator routes live births");
        unsigned numberedTerrain=0;
        require(scene->terrainLifetimes->visit([&](const auto& id,bool,bool){
            if(id.source<900000)return true;
            ++numberedTerrain;
            return id.source<cursor.next&&cursor.births[id.source-900000]==id.object;
        })&&numberedTerrain,"native runs retain allocated source IDs");
        downwell::StateDigest digest;
        digest.addU64(random.drawCount());digest.addU32(random.stateIndex());
        for(unsigned i=0;i<16;++i)digest.addU32(random.stateWord(i));
        for(unsigned i=0;i<world->breakableSlots();++i){const auto& box=world->breakableSlot(i);
            digest.addU32(box.sourceId);digest.addI32(box.x);digest.addI32(box.y);
            digest.addU32(box.state.breakSound);digest.addU32(box.state.haveGem);
        }
        for(unsigned i=1;i<=scene->tiles.size();++i){const auto& tile=scene->tiles.at(i);
            digest.addI32(tile.x);digest.addI32(tile.objectId);digest.addU32(tile.frame);digest.addU32(tile.next);
        }
        if(!mode){referenceBirths=cursor.births;referenceDigest=digest.value();}
        else require(referenceBirths==cursor.births&&referenceDigest==digest.value(),
            "optimized publishers preserve scalar source-ID order, RNG, box state and tiles");
    }
}

void testBuilderCheckpoints(const char* roomPath, const char* objectPath, const char* patternPath) {
    auto rb = readFile(roomPath), ob = readFile(objectPath), pb = readFile(patternPath);
    downwell::RoomPack rooms(rb.data(), rb.size());
    downwell::ObjectPack objects(ob.data(), ob.size());
    downwell::BuilderPatternPack patterns(pb.data(), pb.size());
    downwell::PackedRoom room;
    require(rooms.findRoom(11, room), "natural route has decoded main room");
    downwell::PackedRoomInstance authoredBuilder;
    require(rooms.roomInstance(room, 0, authoredBuilder) &&
        authoredBuilder.objectId == 65 && authoredBuilder.instanceId == 100856 &&
        authoredBuilder.x == -16 && authoredBuilder.y == 704,
        "builder checkpoint is anchored to the serialized builder, not the player");
    for (unsigned seed : {2u, 4u, 8u}) {
        downwell::StartupRuntime entry(downwell::PlatformProfile::nonDesktop,
            &rooms, &objects, seed, &patterns);
        entry.skipToGameForTesting();
        auto reference = entry.mainBuilderState();
        auto referenceRandom = entry.randomState();
        downwell::GameMakerRandom nativeRandom;
        AmigaTerrainFragment fragments[19];
        AmigaAreaOneTerrainBuilder checkpoint(fragments, 19);
        if (seed == 2) checkpoint.resetInitialCheckpoint(nativeRandom);
        else if (seed == 4) AmigaNaturalRouteVerification::entry(checkpoint, nativeRandom);
        else {
            require(amigaOverflowProfileEntry(checkpoint, nativeRandom), "overflow checkpoint imports");
            nativeRandom.nextU32(); // This checkpoint precedes entrance-field Create.
        }
        using Commands = std::vector<downwell::DeferredCreateInstance>;
        Commands expected, actual;
        const auto collect = [](void* context, const downwell::DeferredCreateInstance& command) {
            static_cast<Commands*>(context)->push_back(command);
            return true;
        };
        checkpoint.setCreateObserver(collect, &actual);
        bool sawSpawner = false;
        for (unsigned tick = 0; tick < 128 && !reference.endReached(); ++tick) {
            expected.clear(); actual.clear();
            downwell::AreaOneBuilderStep a, b;
            require(reference.stepVisible(patterns, referenceRandom, 100000, collect, &expected, a) &&
                checkpoint.stepVisible(patterns, nativeRandom, 100000, b),
                "decoded and checkpoint builders generate the complete initial level");
            require(expected.size() == actual.size(), "checkpoint preserves ordered birth count");
            for (unsigned i = 0; i < expected.size(); ++i) {
                const auto& e = expected[i]; const auto& n = actual[i];
                require(e.objectId == n.objectId && e.x == n.x && e.y == n.y,
                    "checkpoint preserves every ordered birth identity and position");
                if (e.objectId == 78) {
                    require(!sawSpawner && n.x == authoredBuilder.x - 100,
                        "scrBuilder creates its single spawner at self.x minus 100");
                    sawSpawner = true;
                }
            }
            require(reference.y() == checkpoint.frontierY() &&
                referenceRandom.drawCount() == nativeRandom.drawCount() &&
                referenceRandom.stateIndex() == nativeRandom.stateIndex(),
                "checkpoint preserves generation frontier and RNG cursor");
            for (unsigned word = 0; word < 16; ++word)
                require(referenceRandom.stateWord(word) == nativeRandom.stateWord(word),
                    "checkpoint preserves complete generation RNG state");
        }
        require(sawSpawner && reference.endReached() && checkpoint.endReached(),
            "all initial checkpoints reach the end with the source-positioned spawner");
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
    {
        downwell::StartupRuntime entry(downwell::PlatformProfile::nonDesktop,
            &rooms, &objects, 2, &patterns);
        entry.skipToGameForTesting();
        std::cout << "Decoded seed-2 entry draws=" << entry.randomDrawCount()
            << " rooms=" << entry.mainBuilderState().rightRoom(0) << ','
            << entry.mainBuilderState().rightRoom(1) << std::endl;
        downwell::GameMakerRandom random;
        AmigaAreaOneTerrainBuilder checkpoint(nullptr, 0);
        checkpoint.resetInitialCheckpoint(random);
        require(random.drawCount() == entry.randomDrawCount() &&
            random.stateIndex() == entry.randomState().stateIndex() &&
            entry.mainBuilderState().rightRoom(0) == 8 &&
            entry.mainBuilderState().rightRoom(1) == 3 &&
            entry.mainBuilderState().generationCount() == -30 &&
            entry.mainBuilderState().rowsGenerated() == 0,
            "ordinary native shortcut is the decoded seed-2 entry checkpoint");
        for (unsigned word = 0; word < 16; ++word)
            require(random.stateWord(word) == entry.randomState().stateWord(word),
                    "ordinary entry checkpoint preserves complete decoded RNG state");
    }
    const bool sweep = std::getenv("DOWNWELL_VERIFY_ROUTE_SWEEP") != nullptr;
    const std::vector<unsigned> seeds = sweep ? std::vector<unsigned>{0, 1, 2, 3, 4, 5, 6, 7}
                                             : std::vector<unsigned>{99, 4};
    for (unsigned seed : seeds) {
        std::cout << "Natural route seed=" << seed << std::endl;
        downwell::StartupRuntime reference(downwell::PlatformProfile::nonDesktop,
            &rooms, &objects, seed, &patterns);
        reference.skipToGameForTesting();
        AmigaNaturalRouteVerification nativeRoute;
        std::vector<unsigned> profileInputs[2];
        if (seed == 4) {
            std::cout << "Decoded seed-4 entry draws=" << reference.randomDrawCount()
                << " rooms=" << reference.mainBuilderState().rightRoom(0) << ','
                << reference.mainBuilderState().rightRoom(1) << std::endl;
            downwell::GameMakerRandom random;
            AmigaAreaOneTerrainBuilder checkpoint(nullptr, 0);
            AmigaNaturalRouteVerification::entry(checkpoint, random);
            require(random.drawCount() == reference.randomDrawCount() &&
                random.stateIndex() == reference.randomState().stateIndex() &&
                reference.mainBuilderState().rightRoom(0) == 8 &&
                reference.mainBuilderState().rightRoom(1) == 4 &&
                reference.mainBuilderState().generationCount() == -30 &&
                reference.mainBuilderState().rowsGenerated() == 0,
                "native seed-4 shortcut is the decoded startup entry checkpoint");
            for (unsigned i = 0; i < 16; ++i)
                require(random.stateWord(i) == reference.randomState().stateWord(i), "native entry RNG matches decoded startup");
        }
        downwell::GameplayTerrainInstances terrainInstances;
        AmigaPortRuntime game(300, false, 1201 - 176, true,{},{},{},&terrainInstances);
        game.initializeEntryInput(false);
        LiveSceneWorld live(patterns, objects);
        live.bind(game, rooms, room, false, &reference);
        int target = 240;
        int deepestY = 176 + game.player().offsetY();
        unsigned noDescent = 0;
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
            // Escape local ledges using ordinary inputs, including when
            // corrected source RNG changes later generated geometry.
            {
                if (y > deepestY + 8) { deepestY = y; noDescent = 0; }
                else ++noDescent;
                if (noDescent >= 120) target = (updates / 120) % 2 ? 180 : 300;
            }
            downwell::Input input;
            input.left = x > target + 2; input.right = x < target - 2;
            input.fire = updates % 40 == 0 || updates % 40 == 3;
            if (noDescent >= 120) input.fire = updates % 12 == 0;
            if (seed == 4 && !sweep)
                require(downwell::packReplayInput(input) == downwell::packReplayInput(nativeRoute.input(game, live.collision)),
                    "native input controller reproduces host natural route");
            if(seed==4)profileInputs[0].push_back(downwell::packReplayInput(input));
            game.step(downwell::packReplayInput(input)); reference.step(input, false);
            parityFrame = updates;
            compareInitialGameplay(game, reference);
            if (sweep) {
                require(game.health() == reference.mainPlayerHealth() &&
                    game.currency() == reference.mainCurrencyTotal() &&
                    game.dead() == reference.mainDeathSequence().active &&
                    game.levelResultRequested() == reference.levelResultRequested(),
                    "route sweep health, currency and exit state agree");
            }
            {
                require(game.randomDrawCount() == reference.randomDrawCount() &&
                    game.randomState().stateIndex() == reference.randomState().stateIndex(),
                    "natural route RNG cursor agrees");
                for (unsigned i = 0; i < 16; ++i)
                    require(game.randomState().stateWord(i) == reference.randomState().stateWord(i),
                        "natural route complete RNG state agrees");
            }
        }
        if (sweep) {
            require(!game.dead() && game.levelResultRequested() && reference.levelResultRequested(),
                "every pinned sweep route reaches its generated exit alive within budget");
            std::cout << "Route sweep seed=" << seed << " updates=" << updates
                << " outcome=" << (game.dead() ? "death" : game.levelResultRequested() ? "exit" : "budget")
                << " health=" << game.health() << " gems=" << game.currency()
                << " depth=" << deepestY << " enemies=" << game.enemyCount() << std::endl;
            continue;
        }
        if(game.dead() || !game.levelResultRequested() || !reference.levelResultRequested())
            std::cerr << "Natural route incomplete seed=" << seed << " updates=" << updates
                << " position=" << 240+game.player().offsetX() << ',' << 176+game.player().offsetY()
                << " target=" << target << " health=" << game.health()
                << " fields=" << game.timeFields().size() << " rng=" << game.randomDrawCount()
                << " dead=" << game.dead() << std::endl;
        require(!game.dead() && game.levelResultRequested() && reference.levelResultRequested(),
            "input-only route reaches a naturally generated exit alive on both adapters");
        require(terrainInstances.valid()&&terrainInstances.size()>0,
            "natural generation publishes individual terrain lifetimes");
        const auto oldTerrain=terrainInstances.handle(0);
        require(game.enterLevelClear(0, false) && reference.enterLevelClear(0, false),
            "natural exit transfers to upgrade scene");
        require(terrainInstances.size()==0&&!terrainInstances.find(oldTerrain),
            "natural Result entry retires generated terrain before the upgrade screen");
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
        const auto carriedKills = game.enemiesDefeated();
        require(carriedKills == reference.mainEnemiesDefeated(), "natural run kill totals agree before reentry");
        std::cout << "Natural carried kills=" << carriedKills << std::endl;
        downwell::Input held; held.fire = true;
        require(game.prepareNextAreaOneLevel(held, 319) && reference.loadNextAreaOneLevel(held),
            "natural route enters second cavern");
        require(terrainInstances.size()==0&&!terrainInstances.find(oldTerrain),
            "natural level advance retires generated terrain before regeneration");
        require(game.runStage().level == 2 && reference.runStage().level == 2,
            "natural route advances exactly one level");
        require(game.enemiesDefeated() == carriedKills && reference.mainEnemiesDefeated() == carriedKills,
            "both room loaders preserve the run kill total used by results");
        live.bind(game, rooms, room, true);
        for (unsigned tick = 0; tick < 120; ++tick) {
            downwell::Input input;
            input.fire = tick < 4;
            if (seed == 4) require(downwell::packReplayInput(input) ==
                downwell::packReplayInput(nativeRoute.input(game, live.collision, tick)), "native reentry inputs match");
            if(seed==4)profileInputs[1].push_back(downwell::packReplayInput(input));
            game.step(downwell::packReplayInput(input)); reference.step(input, false);
            parityFrame = tick;
            compareInitialGameplay(game, reference);
        }
        require(terrainInstances.size()>0&&terrainInstances.handle(0).generation>oldTerrain.generation,
            "natural second-cavern generation cannot revive a first-cavern key");
        std::cout << "Natural exit seed=" << seed << " gameplay=" << updates
                  << " upgrade=" << clearUpdates << " reentry=120 passed\n";
        if (seed == 4) {
            bool reached = false;
            unsigned shopUpdates = 0;
            deepestY = 176 + game.player().offsetY();
            noDescent = 0;
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
                if (y > deepestY + 8) { deepestY = y; noDescent = 0; }
                else ++noDescent;
                if (noDescent >= 120) target = (tick / 120) % 2 ? 180 : 300;
                for (const auto& shop : game.shops().instances)
                    if (shop.present && y < shop.y + 12 && y > shop.y - 200) target = shop.x - 6;
                downwell::Input input;
                input.left = x > target + 2; input.right = x < target - 2;
                input.fire = tick % 40 == 0 || tick % 40 == 3;
                if (noDescent >= 120) input.fire = tick % 12 == 0;
                require(downwell::packReplayInput(input) == downwell::packReplayInput(nativeRoute.input(game, live.collision, tick + 120)),
                    "native natural shop approach inputs match");
                profileInputs[1].push_back(downwell::packReplayInput(input));
                game.step(downwell::packReplayInput(input)); reference.step(input, false);
                parityFrame = tick;
                compareInitialGameplay(game, reference);
                if (game.shops().interactable) { reached = true; ++shopUpdates; break; }
                if (std::getenv("DOWNWELL_TRACE_SHOP_ROUTE") && tick % 200 == 0) {
                    std::cout << "Shop approach tick=" << tick << " xy=" << x << ',' << y
                        << " target=" << target << " health=" << game.health() << std::endl;
                    for (const auto& shop : game.shops().instances) if (shop.present)
                        std::cout << "  shop=" << shop.x << ',' << shop.y << std::endl;
                }
            }
            require(reached && reference.shops().interactable, "ordinary input reaches the generated second-level shop");
            unsigned secondUpdates = 120 + shopUpdates;
            auto shopTick = [&](bool fire, bool left = false) {
                downwell::Input input; input.fire = fire; input.left = left;
                require(downwell::packReplayInput(input) == downwell::packReplayInput(nativeRoute.input(game, live.collision, secondUpdates++)),
                    "native natural shop and return inputs match");
                profileInputs[1].push_back(downwell::packReplayInput(input));
                game.step(downwell::packReplayInput(input)); reference.step(input, false);
                nativeRoute.check(game);
                require(nativeRoute.failures == 0, "native natural shop assertions pass on host");
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
                      << " gems=" << money << " price=" << originalStock.price << " return=" << nativeRoute.returnFrames
                      << " camera=" << sideCamera << "->" << game.cameraOffsetX() << std::endl;
            require(sideCamera != 0 && returnUpdates < 240 && !game.dead(),
                "natural shop return releases side camera and reaches main shaft alive");
            std::cout << "Natural route second-level updates=" << secondUpdates << '\n';
            require(nativeRoute.done && nativeRoute.returnFrames == returnUpdates+1,
                "native natural route completion matches host timing");
            if(std::getenv("DOWNWELL_EXPORT_SHOP_PROFILE")) {
                for(unsigned level=0;level<2;++level){
                    std::cout<<"SHOP_TAPE "<<level+1;
                    for(auto input:profileInputs[level])std::cout<<' '<<input;
                    std::cout<<'\n';
                }
                std::cout<<"SHOP_FINAL "<<game.health()<<' '<<game.currency()<<' '
                    <<game.player().offsetX()<<' '<<game.player().offsetY()<<' '
                    <<game.randomDrawCount()<<' '<<game.randomState().stateIndex();
                for(unsigned i=0;i<16;++i)std::cout<<' '<<game.randomState().stateWord(i);
                std::cout<<'\n';
            }
        }
    }
}

int main(int argc, char** argv) {
    {
        static GameplayWorld shaken(0,true,0,true),plain(0,true,0,true);
        auto oracle=shaken.randomState();const auto before=shaken.randomDrawCount();
        const int dx=int(oracle.irandom(2))-1,dy=int(oracle.irandom(4))-2;
        unsigned transitions=0;
        require(shaken.withPlayerTimeTransition([&](const auto& player,const auto& motion,int x,int y){
            ++transitions;
            require(player.x==x&&player.y==y&&
                x==240+downwell::roundGameMakerQ8(motion.offsetXFixed())&&
                y==176+downwell::roundGameMakerQ8(motion.offsetYFixed())&&shaken.randomDrawCount()==before,
                "native transition sees new published player coordinates before camera RNG");
            return shaken.requestTimeStopCameraShake();
        },[&]{shaken.step(0);return shaken.effectsSucceeded();}),"native transition requests shake inside player Begin Step");
        plain.step(0);
        require(shaken.effectsSucceeded()&&plain.effectsSucceeded()&&
            shaken.randomDrawCount()==before+4&&plain.randomDrawCount()==before&&
            shaken.cameraOffsetX()==plain.cameraOffsetX()+dx&&
            shaken.cameraOffsetY()==plain.cameraOffsetY()+dy,
            "native camera samples source-ordered shake after alarm countdown without altering easing");
        for(unsigned word=0;word<16;++word)
            require(shaken.randomState().stateWord(word)==oracle.stateWord(word),"native shake preserves exact WELL state");
        shaken.step(0);plain.step(0);
        require(transitions==1&&shaken.randomDrawCount()==before+4&&
            shaken.cameraOffsetX()==plain.cameraOffsetX()&&shaken.cameraOffsetY()==plain.cameraOffsetY(),
            "native second-tick alarm removes shake without additional RNG");
        shaken.step(0);plain.step(0);
        require(shaken.checkpoint()==plain.checkpoint(),"expired shake leaves no permanent scrolling offset");
    }
    for(bool nested:{false,true}){
        auto world=std::make_unique<GameplayWorld>(0,true,0,true);
        unsigned transitions=0,alarms=0;
        world->configureRoomAlarms([](void* p){++*static_cast<unsigned*>(p);return true;},&alarms);
        const auto draws=world->randomDrawCount();const auto clock=world->runTimeTicks();
        auto unused=[](const auto&,const auto&,int,int){return true;};
        require(!world->withPlayerTimeTransition([&](const auto&,const auto&,int,int){
            ++transitions;
            if(nested){
                require(!world->withPlayerTimeTransition(unused,[]{return true;}),"nested native transition scopes are rejected");
                return true; // A caller cannot hide the world's failed state.
            }
            return false;
        },[&]{world->step(0);return world->effectsSucceeded();}),"failed native transition stops the tick");
        require(transitions==1&&!alarms&&world->randomDrawCount()==draws&&world->runTimeTicks()==clock+1,
            "transition failure retains earlier clock work and prevents later alarms and camera RNG");
        world->step(0);
        require(transitions==1&&!alarms&&world->randomDrawCount()==draws,"failed native world does not retry transition");
    }
    {
        struct Capture {unsigned calls=0;std::uint32_t revision=1;bool okay=true;} capture;
        static GameplayWorld world(0);
        const auto read=[](void* p){return static_cast<Capture*>(p)->revision;};
        const auto collect=[](void* p,GameplayWorld::EnemyStepOrder&){
            auto& c=*static_cast<Capture*>(p);++c.calls;return c.okay;
        };
        world.configureEnemyStepOrder(collect,&capture,read);
        world.step(0);const auto first=capture.calls;
        require(first==1,"enemy order captured once across unchanged phase boundaries");
        world.step(0);require(capture.calls==first,"unchanged enemy order retained across frames");
        ++capture.revision;world.step(0);
        require(capture.calls==first+1,"revision mutation rebuilds enemy order");
        world.configureEnemyStepOrder(collect,&capture,read);world.step(0);
        require(capture.calls==first+2,"rebinding invalidates retained enemy order");
        capture.revision=0xffffffffu;world.step(0);const auto saturated=capture.calls;
        world.step(0);require(capture.calls>saturated,"saturated revision never reuses capture");
        world.configureEnemyStepOrder(collect,&capture);world.step(0);const auto uncached=capture.calls;
        world.step(0);require(capture.calls>uncached,"legacy callback always captures");
    }
    if(argc>=4&&std::getenv("DOWNWELL_VERIFY_CONTACT_PROFILE")){
        const auto roomBytes=readFile(argv[1]),objectBytes=readFile(argv[2]),patternBytes=readFile(argv[3]);
        downwell::RoomPack rooms(roomBytes.data(),roomBytes.size());
        downwell::ObjectPack objects(objectBytes.data(),objectBytes.size());
        downwell::BuilderPatternPack patterns(patternBytes.data(),patternBytes.size());
        downwell::PackedRoom room;
        require(rooms.valid()&&objects.valid()&&patterns.valid()&&rooms.findRoom(11,room),"contact profile resources");
        const bool encounterSearch=std::getenv("DOWNWELL_ENCOUNTER_PROFILE_SEARCH")!=nullptr;
        const bool search=encounterSearch||std::getenv("DOWNWELL_CONTACT_PROFILE_SEARCH")!=nullptr;
        unsigned seedCount=1,seedFirst=2,routeFirst=0,routeEnd=64;
        if(const char* count=std::getenv("DOWNWELL_ENCOUNTER_SEEDS")){
            char* end=nullptr;const auto parsed=std::strtoul(count,&end,10);
            require(encounterSearch&&end!=count&&!*end&&parsed>=1&&parsed<=64,
                "encounter seed count must be 1..64 in encounter-search mode");
            seedCount=static_cast<unsigned>(parsed);
        }
        if(const char* selected=std::getenv("DOWNWELL_ENCOUNTER_CASE")){
            char trailing=0;
            require(encounterSearch&&seedCount==1&&
                std::sscanf(selected,"%u:%u%c",&seedFirst,&routeFirst,&trailing)==2&&
                seedFirst>=1&&seedFirst<=65535&&routeFirst<64,
                "encounter case must be seed:route (seed 1..65535, route 0..63), without a seed sweep");
            routeEnd=routeFirst+1;
        }
        struct TerrainBirthAudit {
            unsigned walls=0,platforms=0,other=0;
            bool birth(downwell::InstanceHandle,const downwell::GameplayTerrainInstances::Record& r){
                if(r.objectId==60)++walls;else if(r.objectId==93)++platforms;else ++other;
                return true;
            }
            bool retire(downwell::InstanceHandle,const downwell::GameplayTerrainInstances::Record&){return true;}
            bool activation(downwell::InstanceHandle,const downwell::GameplayTerrainInstances::Record&,bool){return true;}
            bool membership(downwell::InstanceHandle,const downwell::GameplayTerrainInstances::Record&,bool){return true;}
            unsigned total()const{return walls+platforms+other;}
        };
        for(unsigned seed=seedFirst;seed<seedFirst+seedCount;++seed)
        for(unsigned phase=encounterSearch?routeFirst:search?0:downwell::gameplayContactProfilePhase;
            phase<(encounterSearch?routeEnd:search?10u:downwell::gameplayContactProfilePhase+1);++phase){
            if(encounterSearch)std::cout<<"Encounter case seed="<<seed<<" route="<<phase<<std::endl;
            downwell::StartupRuntime host(downwell::PlatformProfile::nonDesktop,&rooms,&objects,seed,&patterns);
            host.skipToGameForTesting();
            TerrainBirthAudit births,worstBirths;unsigned worstBirthTick=0;
            downwell::GameplayTerrainInstances terrain(downwell::GameplayTerrainInstances::Binding::bind(births));
            AmigaPortRuntime game(300,false,1201-176,true,{},{},{},&terrain);game.initializeEntryInput(false);
            LiveSceneWorld live(patterns,objects);live.bind(game,rooms,room,false,&host);
            unsigned firstDamage=0,damageFrames=0,currencyBirths=0,currencyAdvances=0;
            GameplayEncounterCoverage coverage;
            for(unsigned tick=0;tick<downwell::gameplayContactProfileFrames;++tick){
                births={};
                auto input=downwell::gameplayContactProfileInput(game.tick(),240+game.player().offsetX(),
                    176+game.player().offsetY(),phase);
                if(encounterSearch){
                    // Search input routes only. Neither owner nor gameplay RNG
                    // is altered. Keep the source entry checkpoint unchanged.
                    require(downwell::gameplayEncounterProfileInput(tick,240+game.player().offsetX(),
                        176+game.player().offsetY(),phase,input),"bounded encounter route");
                }
                game.step(downwell::packReplayInput(input));
                require(terrain.valid(),"contact tape terrain identity store remains valid");
                if(births.total()>worstBirths.total()){worstBirths=births;worstBirthTick=tick+1;}
                require(game.effectsSucceeded()&&game.generationSucceeded(),"contact tape runtime remains valid");
                require(coverage.sample(game,tick+1),"contact encounter census remains valid");
                if(game.damageNumbers().size()){if(!firstDamage)firstDamage=tick+1;++damageFrames;}
                if(!search||encounterSearch){
#ifdef DOWNWELL_COMPARE_CONTACT_ATAN
                    downwell::referenceContactAtan=true;
#endif
                    host.step(input,false);parityFrame=tick;
#ifdef DOWNWELL_COMPARE_CONTACT_ATAN
                    downwell::referenceContactAtan=false;
#endif
                    compareInitialGameplay(game,host);
                    // This tape creates currency through combat after the
                    // engine animation boundary, not as room-start instances.
                    unsigned currencyIndex=0;
                    for(unsigned i=0;i<game.gemCapacity;++i){
                        const auto& gem=game.gemAt(i);if(!gem.active)continue;
                        require(currencyIndex<host.mainCurrencyCount(),"paired currency count");
                        const auto& other=host.mainCurrencyAt(currencyIndex++);
                        require(gem.age==other.age &&
                            gem.motion.animation.phaseBits()==other.motion.animation.phaseBits() &&
                            gem.motion.animation.speedBits()==other.motion.animation.speedBits(),
                            "paired currency animation phase and speed agree");
                        // The runner's phase creation cutoff excludes rewards
                        // born in bullet Step / Collision. Create is age zero;
                        // first animation and first Step both occur next tick.
                        if(gem.age==0){
                            ++currencyBirths;
                            require(gem.motion.animation.phaseBits()==0,
                                "combat-born currency retains Create frame in its birth tick");
                        }else if(gem.age==1){
                            ++currencyAdvances;
                            downwell::RunnerImageValue expected{};bool crossed=false;
                            require(downwell::SequenceTimeline::advance(expected,
                                {gem.motion.animation.speedBits()},4,crossed),"first gem animation");
                            require(gem.motion.animation.phaseBits()==expected.bits,
                                "combat-born currency advances once on the next tick");
                        }
                    }
                    require(currencyIndex==host.mainCurrencyCount(),"paired live currency count");
                    require(game.health()==host.mainPlayerHealth()&&game.damageNumbers().size()==host.damageNumbers().size(),
                        "contact tape health and damage ownership agree");
                    require(game.damageNumbers().birthSequence().next==host.damageNumbers().birthSequence().next,
                        "contact tape damage lifetime sequence agrees");
                    for(unsigned i=0;i<game.damageNumbers().size();++i){
                        const auto& a=*game.damageNumbers().atNewest(i);
                        const auto& b=*host.damageNumbers().atNewest(i);
                        auto same=[](downwell::MotionBinary64 x,downwell::MotionBinary64 y){
                            return x.high==y.high&&x.low==y.low;
                        };
                        require(
#ifdef DOWNWELL_COMPARE_CONTACT_ATAN
                            [](auto x,auto y){float ax,bx;std::memcpy(&ax,&x.bits,4);std::memcpy(&bx,&y.bits,4);
                                return std::fabs(ax-bx)<1.0F/64;}(a.x,b.x)&&
#else
                            a.x.bits==b.x.bits&&same(a.direction,b.direction)&&same(a.xSpeed,b.xSpeed)&&
#endif
                            a.y.bits==b.y.bits&&a.alarm0==b.alarm0&&
                            a.drawBirth==b.drawBirth&&same(a.amount,b.amount)&&same(a.ySpeed,b.ySpeed),
                            "contact tape complete damage-number motion and painter order agree");
                    }
                    require(game.randomDrawCount()==host.randomDrawCount()&&
                        game.randomState().stateIndex()==host.randomState().stateIndex(),"contact tape RNG cursor agrees");
                    for(unsigned i=0;i<16;++i)require(game.randomState().stateWord(i)==host.randomState().stateWord(i),
                        "contact tape full RNG agrees");
                }
            }
            if(!search)require(currencyBirths&&currencyAdvances,"contact tape exercises currency birth and next animation");
            std::cout<<"Contact profile phase="<<phase<<" first_damage="<<firstDamage
                <<" damage_frames="<<damageFrames<<" health="<<game.health()<<" y="<<176+game.player().offsetY()
                <<" seed="<<seed<<std::endl;
            coverage.report();
            std::cout<<"Contact terrain burst frame="<<worstBirthTick<<" walls="<<worstBirths.walls
                <<" platforms="<<worstBirths.platforms<<" other="<<worstBirths.other<<std::endl;
            // Source-ordered enemy/currency/bullet updates change the combat
            // trajectory and subsequent pattern draws. This pins the corrected
            // workload; the historical frame-701 timings are not its baseline.
            if(!search)require(worstBirthTick==1689&&worstBirths.walls==127&&
                worstBirths.platforms==5&&worstBirths.other==0,
                "source-ordered contact benchmark retains its 132-terrain-birth burst");
            if(!search)require(firstDamage&&damageFrames,"profile input must reach real damage");
        }
#ifdef DOWNWELL_AUDIT_RANDOM
        require(downwell::GameMakerRandom::auditedDraws > 1000,
                "contact replay exercised the per-draw ring state oracle");
        std::cout << "RNG replay: " << downwell::GameMakerRandom::auditedDraws
                  << " consumed draws checked against all 16 oracle words\n";
#endif
        return 0;
    }
    if(argc>=4 && std::getenv("DOWNWELL_VERIFY_GENERATED_HANDOFF")){
        const auto bytes=readFile(argv[2]);
        const downwell::ObjectPack objects(bytes.data(),bytes.size());
        testGeneratedWorldHandoff(objects);
        testNumberedGeneration(argv[1],argv[2],argv[3]);
        std::cout << "Generated world handoff passed\n";
        return 0;
    }
    if (argc >= 4) testBuilderCheckpoints(argv[1], argv[2], argv[3]);
    if (argc >= 4 && (std::getenv("DOWNWELL_VERIFY_NATURAL_EXIT") || std::getenv("DOWNWELL_VERIFY_ROUTE_SWEEP"))) {
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
    std::cout << "Native terrain identity peak="<<nativeTerrainPeak<<" digest="<<nativeTerrainDigest.value()<<'\n';
    std::cout << "Amiga scene runtime tests passed\n";
    return 0;
}
