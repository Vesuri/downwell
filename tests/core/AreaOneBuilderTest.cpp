#include "core/AreaOneBuilder.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

namespace {
using namespace downwell;
using Commands = std::vector<DeferredCreateInstance>;

void require(bool condition, const char* message) {
    if (!condition) { std::cerr << "FAILED: " << message << '\n'; std::exit(1); }
}

bool collect(void* context, const DeferredCreateInstance& command) {
    static_cast<Commands*>(context)->push_back(command);
    return true;
}

bool sameRandom(const GameMakerRandom& a, const GameMakerRandom& b) {
    if (a.drawCount() != b.drawCount() || a.stateIndex() != b.stateIndex())
        return false;
    for (unsigned i = 0; i < 16; ++i)
        if (a.stateWord(i) != b.stateWord(i)) return false;
    return true;
}

bool sameCommand(const DeferredCreateInstance& a, const DeferredCreateInstance& b) {
    if (a.objectId != b.objectId || a.x != b.x || a.y != b.y ||
        a.scaleX != b.scaleX || a.scaleY != b.scaleY ||
        a.integerScaleX != b.integerScaleX || a.imageSpeed != b.imageSpeed ||
        a.imageIndex != b.imageIndex || a.integerImageIndex != b.integerImageIndex ||
        a.velocityX != b.velocityX || a.velocityY != b.velocityY ||
        a.integerVelocityX != b.integerVelocityX || a.integerVelocityY != b.integerVelocityY ||
        a.direction != b.direction || a.integerDirection != b.integerDirection ||
        a.moveSpeed != b.moveSpeed || a.integerMoveSpeed != b.integerMoveSpeed ||
        a.motionState != b.motionState || a.auxiliaryState != b.auxiliaryState ||
        a.motionInitialized != b.motionInitialized || a.grounded != b.grounded ||
        a.active != b.active) return false;
    for (unsigned i = 0; i < AlarmBank::count; ++i)
        if (a.alarms.get(i) != b.alarms.get(i)) return false;
    return true;
}
}

int main(int argc, char** argv) {
    require(argc == 2, "builder pack path required");
    std::ifstream input(argv[1], std::ios::binary);
    const std::vector<std::uint8_t> bytes(
        (std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    BuilderPatternPack pack(bytes.data(), bytes.size());
    require(pack.valid(), "builder pack validates");
    GameMakerRandom random(2);
    AreaOneBuilder builder;
    builder.reset(240, 704, random);
    const GameMakerRandom before = random;
    Commands commands;
    AreaOneBuilderStep result;
    result.patternGenerated = true;
    result.advancedRows = 99;
    for (int bottom : {0, 575, 576}) {
        require(builder.stepVisible(pack, random, bottom, collect, &commands, result),
                "invisible builder step succeeds");
        require(builder.y() == 704 && builder.generationCount() == -30 &&
                    commands.empty() && sameRandom(random, before) &&
                    !result.patternGenerated && result.advancedRows == 0,
                "strict visibility boundary is a complete no-op and clears result");
    }
    // ceil(576.01) == 577: fractional camera motion across the boundary counts.
    require(builder.stepVisible(pack, random, 577, collect, &commands, result),
            "fractional boundary crossing generates a row");
    require(builder.y() == 720 && builder.generationCount() == -29 &&
                commands.size() == 2 && random.drawCount() == before.drawCount() + 21,
            "prelude advances one row with its two walls and exact RNG cost");
    require(builder.stepVisible(pack, random, 100000, collect, &commands, result) &&
                builder.y() == 736 && builder.generationCount() == -28 &&
                commands.size() == 4,
            "camera jump never catches up multiple builder events in one tick");

    AreaOneBuilder direct, incremental;
    GameMakerRandom directRandom(2), incrementalRandom(2);
    direct.reset(240, 704, directRandom);
    incremental.reset(240, 704, incrementalRandom);
    Commands directCommands, incrementalCommands;
    unsigned ticks = 0;
    while (!direct.endReached() && ticks++ < 128) {
        const auto pausedRandom = incrementalRandom;
        const auto pausedCount = incrementalCommands.size();
        require(incremental.stepVisible(pack, incrementalRandom, incremental.y() - 128,
                    collect, &incrementalCommands, result) &&
                    sameRandom(pausedRandom, incrementalRandom) &&
                    pausedCount == incrementalCommands.size(),
                "waiting between fragments neither emits nor consumes random draws");
        AreaOneBuilderStep expected;
        require(direct.step(pack, directRandom, collect, &directCommands, expected) &&
                    incremental.stepVisible(pack, incrementalRandom, incremental.y() - 127,
                        collect, &incrementalCommands, result),
                "direct and visible single events succeed");
        require(result.patternGenerated == expected.patternGenerated &&
                    result.family == expected.family && result.patternIndex == expected.patternIndex &&
                    result.patternY == expected.patternY && result.reversed == expected.reversed &&
                    result.sideRoom == expected.sideRoom && result.appendPillars == expected.appendPillars &&
                    result.advancedRows == expected.advancedRows &&
                    incremental.y() == direct.y() && sameRandom(directRandom, incrementalRandom),
                "visible event preserves pattern metadata and complete RNG state");
    }
    require(direct.endReached() && incremental.endReached() &&
                directCommands.size() == incrementalCommands.size(),
            "both schedules finish the complete level");
    bool emittedTorch = false;
    for (const auto& command : directCommands)
        emittedTorch = emittedTorch || command.objectId == 296;
    require(emittedTorch, "standalone builder tape retains I/J torch emission coverage");
    for (std::size_t i = 0; i < directCommands.size(); ++i)
        require(sameCommand(directCommands[i], incrementalCommands[i]),
                "ordered emissions preserve every Create-event field including alarms");
    const auto finishedRandom = incrementalRandom;
    const auto finishedCount = incrementalCommands.size();
    require(incremental.stepVisible(pack, incrementalRandom, 100000, collect,
                &incrementalCommands, result) && sameRandom(finishedRandom, incrementalRandom) &&
                finishedCount == incrementalCommands.size() && !result.patternGenerated &&
                result.advancedRows == 0,
            "finished generator stays inert even with a visible frontier");

    // Live events share this stream. Precomputing an entire level cannot stand
    // in for generation interleaved with enemy alarms that also draw randomness.
    AreaOneBuilder interleaved;
    GameMakerRandom liveRandom(2);
    interleaved.reset(240, 704, liveRandom);
    Commands liveCommands;
    ticks = 0;
    while (!interleaved.endReached() && ticks++ < 128) {
        require(interleaved.stepVisible(pack, liveRandom, interleaved.y() - 127,
                    collect, &liveCommands, result), "interleaved builder succeeds");
        if (result.patternGenerated) liveRandom.irandomSmall(60);
    }
    bool different = liveCommands.size() != directCommands.size();
    for (std::size_t i = 0; !different && i < liveCommands.size(); ++i)
        different = !sameCommand(liveCommands[i], directCommands[i]);
    require(interleaved.endReached() && different,
            "intervening gameplay RNG draws change later emitted level content");
    std::cout << "area-one visible builder tests passed\n";
}
