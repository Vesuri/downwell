#include "platform/amiga/AmigaTerrain.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

namespace {
void require(bool value, const char* message) {
    if (!value) { std::cerr << "FAILED: " << message << '\n'; std::exit(1); }
}
using Commands = std::vector<downwell::DeferredCreateInstance>;
bool collect(void* context, const downwell::DeferredCreateInstance& command) {
    static_cast<Commands*>(context)->push_back(command);
    return true;
}
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
        require(native.stepVisible(patterns, nativeRandom, native.frontierY() - 128, actual),
                "invisible tick succeeds");
        require(native.fragmentCount() == oldCount && !actual.patternGenerated &&
                    native.frontierY() == reference.y(), "invisible tick leaves terrain unchanged");
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
                command.objectId == 109 || command.objectId == 306) {
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
        compareRandom(nativeRandom, referenceRandom);
        if (interleave && expected.patternGenerated) {
            nativeRandom.irandomSmall(180);
            referenceRandom.irandomSmall(180);
        }
    }
    require(native.endReached() && reference.endReached() && native.fragmentCount() == 19,
            "complete native generation ends with nineteen fragments");
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
    checkSchedule(patterns, false);
    checkSchedule(patterns, true);
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
