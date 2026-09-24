#include "platform/amiga/AmigaTerrain.h"
#include "platform/amiga/AmigaPortRuntime.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
#include <array>

namespace {
void require(bool value, const char* message) {
    if (!value) { std::cerr << "FAILED: " << message << '\n'; std::exit(1); }
}
using Commands = std::vector<downwell::DeferredCreateInstance>;
struct ShopCreates {
    downwell::ShopWorld shops;
    downwell::GameMakerRandom& random;
    Commands commands;
    static bool create(void* opaque, const downwell::DeferredCreateInstance& command) {
        auto& self = *static_cast<ShopCreates*>(opaque);
        self.commands.push_back(command);
        if (command.objectId != 277) return true;
        return self.shops.create(command.x, command.y,
            [&](unsigned lo, unsigned hi) { return lo + self.random.irandomSmall(hi-lo); },
            4, 1, false, 4, false) != nullptr;
    }
};
bool collect(void* context, const downwell::DeferredCreateInstance& command) {
    static_cast<Commands*>(context)->push_back(command);
    return true;
}
struct LiveCollision {
    AmigaTerrainIndex index;
    AmigaTerrainTiles tiles;
    unsigned creates = 0;
    static bool create(void* opaque, const downwell::DeferredCreateInstance& command) {
        auto& self = *static_cast<LiveCollision*>(opaque);
        ++self.creates;
        return amigaAddTerrainObject(self.index, command.objectId, command.x, command.y) &&
               self.tiles.append(command);
    }
};

// Exercise the complete native simulation/generator/collision wiring without
// relying on the executable's still-forecast-based renderer.
struct LiveSimulation {
    AmigaTerrainFragment fragments[19];
    AmigaTerrainEnemy enemies[64];
    AmigaTerrainDecoration decorations[128];
    AmigaTerrainIndex index;
    AmigaAreaOneTerrainBuilder builder{fragments, 19, enemies, 64, decorations, 128};
    AmigaTerrainCollisionContext collision;
    AmigaPortRuntime* runtime = nullptr;
    const downwell::BuilderPatternPack& patterns;
    unsigned generationTicks = 0;

    explicit LiveSimulation(const downwell::BuilderPatternPack& pack)
        : collision{nullptr, nullptr, &pack, nullptr, 0, nullptr}, patterns(pack) {}

    static bool create(void* opaque, const downwell::DeferredCreateInstance& command) {
        auto& world = *static_cast<LiveSimulation*>(opaque);
        if (!amigaAddTerrainObject(world.index, command.objectId, command.x, command.y))
            return false;
        if (command.objectId == 92) {
            const int x = command.x - 240, y = command.y - 176;
            if (!world.runtime->appendBreakables(&x, &y, 1)) return false;
        }
        if (amigaEnemyHealth(command.objectId))
            return world.runtime->configureEnemy(world.runtime->enemyCount(), command.objectId,
                0, command.x - 240, command.y - 176,
                command.integerVelocityX, command.integerVelocityY,
                amigaEnemyHealth(command.objectId), amigaEnemyReward(command.objectId),
                command.motionState, command.auxiliaryState,
                command.alarms.get(0), command.alarms.get(1),
                command.integerDirection, command.integerMoveSpeed,
                command.active, command.grounded, true);
        return true;
    }
    static bool generate(void* opaque, AmigaPortRuntime& runtime,
                         downwell::GameMakerRandom& random, int bottom) {
        auto& world = *static_cast<LiveSimulation*>(opaque);
        require(world.runtime == &runtime, "generation receives bound runtime");
        ++world.generationTicks;
        downwell::AreaOneBuilderStep result;
        return world.builder.stepVisible(world.patterns, random, bottom, result);
    }
    void bind(AmigaPortRuntime& port) {
        runtime = &port;
        require(amigaBuildTerrainIndex(collision, index), "live simulation starts with empty generated terrain");
        collision.index = &index;
        collision.runtime = &port;
        downwell::GameMakerRandom random;
        builder.resetInitialCheckpoint(random);
        builder.setCreateObserver(create, this);
        port.configureEnemyRandom(random);
        port.configureGenerationStep(generate, this);
        port.configureLandingResolver(amigaResolveTerrainLanding, &collision);
        port.configureSolidResolver(amigaResolveTerrainSolid, &collision);
    }
};
void compareRandom(const downwell::GameMakerRandom& a,
                   const downwell::GameMakerRandom& b) {
    require(a.drawCount() == b.drawCount() && a.stateIndex() == b.stateIndex(),
            "native generator consumes the same number of RNG draws");
    for (unsigned i = 0; i < 16; ++i)
        require(a.stateWord(i) == b.stateWord(i), "complete RNG state matches");
}

void checkSchedule(const downwell::BuilderPatternPack& patterns, bool interleave) {
    AmigaTerrainFragment fragments[19];
    AmigaTerrainEnemy enemies[64];
    AmigaTerrainDecoration decorations[128];
    AmigaAreaOneTerrainBuilder native(fragments, 19, enemies, 64, decorations, 128);
    LiveCollision collision;
    AmigaTerrainCollisionContext collisionContext{nullptr, nullptr, &patterns, nullptr, 0, nullptr};
    require(amigaBuildTerrainIndex(collisionContext, collision.index), "empty live collision map initializes");
    collisionContext.index = &collision.index;
    native.setCreateObserver(LiveCollision::create, &collision);
    std::array<bool, AmigaTerrainIndex::columns * AmigaTerrainIndex::rows> solid{}, landing{};
    downwell::GameMakerRandom nativeRandom, referenceRandom(2);
    native.resetInitialCheckpoint(nativeRandom);
    for (unsigned i = 0; i < 470; ++i) referenceRandom.nextU32();
    downwell::AreaOneBuilder reference;
    reference.restoreInitialCheckpoint(240, 704, -30, 3, 7);
    Commands commands;
    unsigned ticks = 0, enemyCount = 0, decorationCount = 0;
    while (!native.endReached() && ticks++ < 128) {
        downwell::AreaOneBuilderStep actual, expected;
        const unsigned oldCount = native.fragmentCount();
        const auto oldCreates = collision.creates;
        require(native.stepVisible(patterns, nativeRandom, native.frontierY() - 128, actual),
                "invisible tick succeeds");
        require(native.fragmentCount() == oldCount && !actual.patternGenerated &&
                    native.frontierY() == reference.y() && collision.creates == oldCreates,
                "invisible tick leaves terrain unchanged");
        compareRandom(nativeRandom, referenceRandom);
        const auto commandStart = commands.size();
        require(reference.step(patterns, referenceRandom, collect, &commands, expected) &&
                    native.stepVisible(patterns, nativeRandom, native.frontierY() - 127, actual),
                "visible tick advances both builders");
        require(native.frontierY() == reference.y() && actual.advancedRows == expected.advancedRows &&
                    actual.patternGenerated == expected.patternGenerated,
                "native frontier advances by exactly the recovered event");
        if (expected.patternGenerated) {
            require(native.fragmentCount() == oldCount + 1, "one new fragment emitted");
            const auto& fragment = fragments[oldCount];
            require(fragment.pattern.family == expected.family &&
                        fragment.pattern.sourceIndex == expected.patternIndex &&
                        fragment.y == expected.patternY && fragment.reversed == expected.reversed &&
                        fragment.sideRoom == expected.sideRoom &&
                        fragment.appendPillars == expected.appendPillars,
                    "native fragment preserves recovered metadata");
        }
        for (auto i = commandStart; i < commands.size(); ++i) {
            const auto& command = commands[i];
            if (command.objectId == 60 || command.objectId == 92 || command.objectId == 93) {
                const unsigned cell = (command.y / 16) * AmigaTerrainIndex::columns +
                    (command.x - AmigaTerrainIndex::minX) / 16;
                require(cell < solid.size(), "reference Create lies in collision storage");
                landing[cell] = true;
                if (command.objectId != 93) solid[cell] = true;
            }
            if (command.objectId == 111 || command.objectId == 112 || command.objectId == 113 ||
                command.objectId == 116 || command.objectId == 117 || command.objectId == 119) {
                require(enemyCount < native.enemyCount(), "enemy exists on its generation tick");
                const auto& enemy = enemies[enemyCount++];
                require(enemy.objectId == command.objectId && enemy.x == command.x - 240 &&
                            enemy.y == command.y - 176 && enemy.speedX == command.integerVelocityX &&
                            enemy.speedY == command.integerVelocityY &&
                            enemy.direction == command.integerDirection &&
                            enemy.moveSpeed == command.integerMoveSpeed &&
                            enemy.motionState == command.motionState &&
                            enemy.auxiliaryState == command.auxiliaryState &&
                            enemy.alarm0 == command.alarms.get(0) && enemy.alarm1 == command.alarms.get(1) &&
                            enemy.grounded == command.grounded && enemy.active == command.active,
                        "native enemy retains exact integer Create state and alarms");
            }
            if (command.objectId == 289 || command.objectId == 293 || command.objectId == 296 ||
                command.objectId == 109) {
                require(decorationCount < native.decorationCount(), "decoration exists on creation tick");
                const auto& decoration = decorations[decorationCount++];
                require(decoration.objectId == command.objectId && decoration.x == command.x &&
                            decoration.y == command.y && decoration.frame == command.integerImageIndex &&
                            decoration.flipX == (command.integerScaleX < 0),
                        "native decoration retains its recovered location and appearance");
            }
        }
        require(enemyCount == native.enemyCount() && decorationCount == native.decorationCount(),
                "no future actors or decorations created early");
        require(collision.creates == commands.size(), "every recovered Create is observed exactly once");
        // Independent full-grid oracle from actual Create events. In
        // particular, unchosen '2'/'3' cells must remain absent.
        for (unsigned column = 0; column < AmigaTerrainIndex::columns; ++column) {
            unsigned next = 0;
            for (int row = AmigaTerrainIndex::rows - 1; row >= 0; --row) {
                const unsigned cell = row * AmigaTerrainIndex::columns + column;
                if (landing[cell]) next = row + 1;
                require(collision.index.nextLanding[cell] == next,
                        "incremental links equal full recomputation after each generation event");
                require(amigaResolveTerrainSolid(&collisionContext,
                            column * 16 + AmigaTerrainIndex::minX - 240, row * 16 - 176) == solid[cell],
                        "only actually emitted terrain is solid at each generation tick");
            }
        }
        compareRandom(nativeRandom, referenceRandom);
        if (interleave && expected.patternGenerated) {
            nativeRandom.irandomSmall(180);
            referenceRandom.irandomSmall(180);
        }
    }
    require(native.endReached() && reference.endReached() && native.fragmentCount() == 19,
            "complete native generation ends with nineteen fragments");
    unsigned tileCount = 0;
    unsigned blocks = 0;
    for (const auto& command : commands) if (command.objectId == 92) ++blocks;
    std::cout << "Generated breakables (interleaved=" << interleave << "): " << blocks << '\n';
    for (int y = 0; y < AmigaTerrainTiles::rows * 16; y += 16) {
        unsigned id = collision.tiles.first(y);
        for (const auto& command : commands) {
            if (command.y != y || (command.objectId != 92 && command.objectId != 93)) continue;
            require(id != 0, "each emitted visible tile appears in its row list");
            const auto& tile = collision.tiles.at(id);
            require(tile.objectId == command.objectId && tile.x == command.x &&
                        tile.frame == (command.objectId == 93 ? 23 : command.integerImageIndex),
                    "tile rendering preserves object order and recovered Create frame");
            id = tile.next;
            ++tileCount;
        }
        require(id == 0, "row list has no uncreated probabilistic tiles");
    }
    require(tileCount == collision.tiles.size() && tileCount > 0,
            "row lists account for every stored tile exactly once");
    for (const auto& command : commands)
        if (command.objectId == 60)
            require(amigaIndexedWall(collision.index, command.x, command.y),
                    "generated walls are available for neighbor-based tile rendering");
    native.resetInitialCheckpoint(nativeRandom);
    require(native.frontierY() == 704 && native.fragmentCount() == 0 &&
                native.enemyCount() == 0 && native.decorationCount() == 0 &&
                nativeRandom.drawCount() == 470 && !native.failed(),
            "restart resets generator and counts without stale records becoming live");
}
}

int main(int argc, char** argv) {
    require(argc == 2, "builder pack path required");
    std::ifstream input(argv[1], std::ios::binary);
    const std::vector<std::uint8_t> bytes(
        (std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    downwell::BuilderPatternPack patterns(bytes.data(), bytes.size());
    require(patterns.valid(), "builder pack validates");
    for (unsigned seed = 1; seed <= 16; ++seed) {
        downwell::GameMakerRandom hostRandom(seed), nativeRandom(seed);
        downwell::AreaOneBuilder host;
        AmigaTerrainFragment fragments[32];
        AmigaAreaOneTerrainBuilder native(fragments, 32);
        const downwell::AreaOneBuilder::Setup setup{1,4,false,true};
        host.reset(240, 704, hostRandom, setup);
        native.resetRoom(240, 704, nativeRandom, setup);
        ShopCreates hostCreates{{},hostRandom,{}}, nativeCreates{{},nativeRandom,{}};
        native.setCreateObserver(ShopCreates::create, &nativeCreates);
        for (unsigned tick = 0; tick < 100 && !host.endReached(); ++tick) {
            downwell::AreaOneBuilderStep a, b;
            require(host.stepVisible(patterns, hostRandom, 100000, ShopCreates::create, &hostCreates, a) &&
                native.stepVisible(patterns, nativeRandom, 100000, b), "generated shop paired traversal");
            compareRandom(hostRandom, nativeRandom);
            require(host.y() == native.frontierY() && a.patternGenerated == b.patternGenerated &&
                a.family == b.family && a.patternIndex == b.patternIndex && a.reversed == b.reversed,
                "shop Create RNG preserves subsequent pattern selection and frontier");
        }
        require(host.endReached() && native.endReached() && hostCreates.shops.allocatedCount == 1 &&
            nativeCreates.shops.allocatedCount == 1, "actual generated shop registered once on both paths");
        require(hostCreates.commands.size() == nativeCreates.commands.size(), "paired create stream length");
        for (unsigned i = 0; i < hostCreates.commands.size(); ++i) {
            const auto& a = hostCreates.commands[i]; const auto& b = nativeCreates.commands[i];
            require(a.objectId == b.objectId && a.x == b.x && a.y == b.y,
                "terrain and enemy creates remain aligned after stock random draws");
        }
        const auto& a = hostCreates.shops.instances[0]; const auto& b = nativeCreates.shops.instances[0];
        require(a.x == b.x && a.y == b.y && a.farewellVariant == b.farewellVariant,
            "generated shop location and farewell match");
        for (unsigned i = 0; i < 3; ++i)
            require(a.stock.items[i].upgrade == b.stock.items[i].upgrade &&
                a.stock.items[i].price == b.stock.items[i].price &&
                a.stock.items[i].remaining == b.stock.items[i].remaining,
                "generated stock and style-discounted prices match");
    }
    checkSchedule(patterns, false);
    checkSchedule(patterns, true);
    AmigaTerrainTiles boundedTiles;
    downwell::DeferredCreateInstance tileCommand(92, 160, 704);
    tileCommand.integerImageIndex = 22;
    for (unsigned i = 0; i < AmigaTerrainTiles::capacity; ++i)
        require(boundedTiles.append(tileCommand), "tile storage accepts its full capacity");
    require(!boundedTiles.append(tileCommand) && boundedTiles.size() == AmigaTerrainTiles::capacity,
            "tile overflow is reported without changing the list");
    boundedTiles.clear();
    require(boundedTiles.size() == 0 && boundedTiles.first(704) == 0 &&
                boundedTiles.first(-16) == 0 && boundedTiles.first(8192) == 0,
            "tile reset discards old row links and bounds queries");
    LiveSimulation live(patterns);
    AmigaPortRuntime runtime(300, false, 4096 - 176, true);
    live.bind(runtime);
    for (unsigned tick = 0; tick < 300; ++tick) {
        runtime.step(0);
        require(runtime.generationSucceeded() && runtime.enemyCount() == live.builder.enemyCount(),
                "native runtime creates only the enemies emitted by its live generator");
    }
    require(live.generationTicks == 300 && live.builder.fragmentCount() > 0 &&
                live.builder.fragmentCount() < 19 && runtime.enemyCount() > 0 &&
                runtime.player().grounded(),
            "live simulation generates only the visible prefix and lands on generated terrain");
    LiveCollision boundary;
    AmigaTerrainCollisionContext empty{nullptr, nullptr, &patterns, nullptr, 0, nullptr};
    require(amigaBuildTerrainIndex(empty, boundary.index), "boundary map initializes");
    require(amigaResolveTerrainLanding(&empty, 0, 4096 - 176) == downwell::gameplayNoLanding,
            "empty fragment store does not manufacture a floor at world Y4096");
    auto indexedEmpty = empty;
    indexedEmpty.index = &boundary.index;
    require(amigaResolveTerrainLanding(&indexedEmpty, 0, 4096 - 176) == downwell::gameplayNoLanding &&
                amigaResolveTerrainLanding(&indexedEmpty, 0, 8192) == downwell::gameplayNoLanding &&
                amigaResolveTerrainLanding(&indexedEmpty, 0, 300000) == downwell::gameplayNoLanding,
            "empty indexed queries including beyond the last row return no landing safely");
    require(amigaAddTerrainObject(boundary.index, 60, 240, 5008) &&
                amigaResolveTerrainLanding(&indexedEmpty, 0, 4096 - 176) == 5008 - 192,
            "real terrain below world Y4096 remains eligible for landing");
    require(!amigaAddTerrainObject(boundary.index, 60, -528, 0) &&
                !amigaAddTerrainObject(boundary.index, 60, 1536, 0) &&
                !amigaAddTerrainObject(boundary.index, 60, 160, -16) &&
                !amigaAddTerrainObject(boundary.index, 60, 160, 8192) &&
                !amigaAddTerrainObject(boundary.index, 60, 161, 0),
            "invalid terrain coordinates are rejected before writing");
    require(amigaAddTerrainObject(boundary.index, 60, -512, 0, true) &&
                amigaIndexedRoomWall(boundary.index, -512, 0) &&
                amigaAddTerrainObject(boundary.index, 93, 1520, 8176),
            "edge cells and initial-room wall identity are supported");
    const auto snapshot = boundary.index;
    require(amigaAddTerrainObject(boundary.index, 60, -512, 0, true) &&
                amigaAddTerrainObject(boundary.index, 113, 5, -9),
            "duplicate terrain and non-terrain Create events are safe");
    for (unsigned cell = 0; cell < sizeof(snapshot.cells); ++cell)
        require(snapshot.cells[cell] == boundary.index.cells[cell] &&
                    snapshot.nextLanding[cell] == boundary.index.nextLanding[cell],
                "duplicates do not disturb the collision map");
    // Each capacity failure is terminal until reset, including a failure after
    // some objects in the current fragment have already been emitted.
    for (unsigned limit = 0; limit < 3; ++limit) {
        AmigaTerrainFragment fragments[19];
        AmigaTerrainEnemy enemies[64];
        AmigaTerrainDecoration decorations[128];
        AmigaAreaOneTerrainBuilder builder(fragments, limit == 0 ? 0 : 19,
            enemies, limit == 1 ? 0 : 64, decorations, limit == 2 ? 0 : 128);
        downwell::GameMakerRandom random;
        builder.resetInitialCheckpoint(random);
        downwell::AreaOneBuilderStep step;
        unsigned ticks = 0;
        while (!builder.failed() && ticks++ < 128)
            builder.stepVisible(patterns, random, builder.frontierY() - 127, step);
        require(builder.failed(), "capacity overflow is detected");
        const auto afterFailure = random;
        const unsigned count = builder.enemyCount();
        require(!builder.stepVisible(patterns, random, 100000, step) &&
                    builder.enemyCount() == count && !step.patternGenerated,
                "partial event cannot be silently retried");
        compareRandom(random, afterFailure);
        builder.resetInitialCheckpoint(random);
        require(!builder.failed() && builder.enemyCount() == 0, "explicit reset clears failure");
    }
    std::cout << "Amiga incremental terrain generation tests passed\n";
}
