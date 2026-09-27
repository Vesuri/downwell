#include "core/AreaOneBuilder.h"
#include "core/BuilderRewardChoice.h"
#include "core/CavernEnemyChoice.h"
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

namespace {
using namespace downwell;
using Commands = std::vector<DeferredCreateInstance>;

// Hash fields, not structure padding; pin ordered Create output plus RNG state.
std::uint64_t emissionDigest = 14695981039346656037ULL;
std::uint64_t legacyDigest = 14695981039346656037ULL;
std::uint64_t preBoxDigest = 14695981039346656037ULL;
void mix(std::uint32_t value,bool newFieldPayload=false,bool boxPayload=false) {
    auto legacy=(newFieldPayload||boxPayload)?0u:value;
    auto preBox=boxPayload?0u:value;
    for (unsigned byte = 0; byte < 4; ++byte) {
        emissionDigest = (emissionDigest ^ (value & 255u)) * 1099511628211ULL;
        legacyDigest = (legacyDigest ^ (legacy & 255u)) * 1099511628211ULL;
        preBoxDigest = (preBoxDigest ^ (preBox & 255u)) * 1099511628211ULL;
        value >>= 8;
        legacy >>= 8;
        preBox >>= 8;
    }
}
void capture(const Commands& commands, const GameMakerRandom& random) {
    mix(static_cast<std::uint32_t>(commands.size()));
    for (const auto& c : commands) {
        for (std::int32_t value : {c.objectId, c.x, c.y, c.integerScaleX,
             c.integerImageIndex, c.integerVelocityX, c.integerVelocityY,
             c.integerDirection, c.integerMoveSpeed, c.motionState}) mix(value);
        mix(c.auxiliaryState,c.objectId==82,c.objectId==92);
        for (float value : {c.scaleX, c.scaleY, c.imageSpeed, c.imageIndex,
             c.velocityX, c.velocityY, c.direction, c.moveSpeed}) {
            std::uint32_t bits;
            static_assert(sizeof bits == sizeof value, "32-bit float fixture");
            std::memcpy(&bits, &value, sizeof bits);
            mix(bits);
        }
        mix(c.motionInitialized); mix(c.grounded); mix(c.active);
        for (unsigned alarm = 0; alarm < AlarmBank::count; ++alarm) mix(c.alarms.get(alarm));
    }
    mix(random.stateIndex());
    mix(static_cast<std::uint32_t>(random.drawCount()));
    mix(static_cast<std::uint32_t>(random.drawCount() >> 32));
    for (unsigned word = 0; word < 16; ++word) mix(random.stateWord(word));
}

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
    // Independent floating-point transcription of the normal-mode VM branch.
    for (unsigned level = 1; level <= 3; ++level)
        for (unsigned seed = 0; seed < 128; ++seed) {
            GameMakerRandom actual(seed), expected(seed);
            for (unsigned repeat = 0; repeat < 32; ++repeat)
                for (char symbol : {'a', 'b', 'c', 'd', 'e', 't', '%', '.', '1'}) {
                    int enemy = -1;
                    if ((expected.nextU32() / 4294967296.0) * (level + 2) <= level) {
                        switch (symbol) {
                        case 'a': enemy = 111; break;
                        case 'b': enemy = 116; break;
                        case 'c': if (level >= 2 && expected.nextU32() % 2) enemy = 119; break;
                        case 'd': enemy = 113; break;
                        case 'e': enemy = 117; break;
                        case 't': enemy = 112; break;
                        case '%': {
                            const int choices[] = {113, 111, 119, 112};
                            enemy = choices[expected.nextU32() % 4]; break;
                        }
                        default: break;
                        }
                    }
                    require(cavernEnemyChoice(symbol, level, actual) == enemy &&
                            sameRandom(actual, expected), "cavern enemy rules preserve VM values and RNG order");
                }
        }
    struct BoundaryRandom {
        std::uint32_t word;
        unsigned draws = 0;
        std::uint32_t nextU32() { ++draws; return word; }
        unsigned chooseIndexSmall(unsigned n) { return nextU32() % n; }
    };
    for (unsigned level = 1; level <= 3; ++level) {
        const std::uint32_t cutoff = static_cast<std::uint32_t>(4294967296.0 * level / (level + 2));
        for (std::uint32_t word : {0U, cutoff - 1, cutoff, cutoff + 1, 0xffffffffU}) {
            BoundaryRandom random{word};
            const bool allowed = (word / 4294967296.0) * (level + 2) <= level;
            require(cavernEnemyChoice('a', level, random) == (allowed ? 111 : -1) && random.draws == 1,
                    "inclusive cavern probability boundaries match runner doubles");
        }
    }
    for (unsigned seed = 0; seed < 128; ++seed) {
        AreaOneBuilder builder;
        GameMakerRandom actual(seed), expected(seed);
        builder.reset(240, 704, actual, {2, 0, false, false});
        const unsigned count = expected.nextU32() % 3 == 2 ? 3 : 2;
        int rooms[3] = {};
        for (unsigned i = 0; i < count; ++i) {
            unsigned rejected = 0;
            for (;;) {
                rooms[i] = 3 + static_cast<int>(expected.irandom(13));
                bool conflict = false;
                for (unsigned j = 0; j < i; ++j)
                    if (std::abs(rooms[i] - rooms[j]) <= 2) conflict = true;
                if (!conflict) break;
                if (++rejected > 100) { rooms[i] = 9999; break; }
            }
        }
        require(builder.rightRoomCount() == count && sameRandom(actual, expected),
                "setup mixes one-word room-count choice with two-word position irandom");
        for (unsigned i = 0; i < count; ++i)
            require(builder.rightRoom(i) == rooms[i], "source-order room positions match");
    }
    for (unsigned seed = 0; seed < 128; ++seed)
        for (int health : {-1, 0, 1, 2, 3, 4})
            for (unsigned style = 0; style < 5; ++style) {
                GameMakerRandom actual(seed), expected(seed);
                int a = 0, b = 0;
                for (unsigned item = 0; item < 16; ++item) {
                    // Original Windows choose is a single unsigned word modulo argc.
                    if (!b) b = expected.nextU32() % 2 ? 109 : 293;
                    else b = b == 109 ? 293 : 109;
                    if (health <= 2 && b != 109) {
                        const unsigned choices[] = {0, 1, 1, 1, 1, 1};
                        if (choices[expected.nextU32() % 6]) b = 109;
                    }
                    if (style == 1) b = 109;
                    a = builderRewardChoice(a, health, style, actual);
                    require(a == b && actual.drawCount() == expected.drawCount() &&
                        actual.stateIndex() == expected.stateIndex(), "reward choice preserves VM branch and RNG ordering");
                    for (unsigned i = 0; i < 16; ++i)
                        require(actual.stateWord(i) == expected.stateWord(i), "reward choice preserves all RNG words");
                }
            }
    require(argc == 2, "builder pack path required");
    std::ifstream input(argv[1], std::ios::binary);
    const std::vector<std::uint8_t> bytes(
        (std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    BuilderPatternPack pack(bytes.data(), bytes.size());
    require(pack.valid(), "builder pack validates");
    unsigned fieldCount=0;
    for(unsigned seed=1;seed<=32;++seed){
        AreaOneBuilder builder;GameMakerRandom actual(seed),shadow(seed);
        builder.reset(240,704,actual,{2,4,true,false});
        struct Probe {GameMakerRandom& actual;GameMakerRandom& shadow;unsigned& fields;} probe{actual,shadow,fieldCount};
        AreaOneBuilderStep result;
        for(unsigned tick=0;tick<100 && !builder.endReached();++tick)
            require(builder.stepVisible(pack,actual,100000,[](void* opaque,const DeferredCreateInstance& command){
                auto& p=*static_cast<Probe*>(opaque);
                if(command.objectId!=82)return true;
                require(p.actual.drawCount()>p.shadow.drawCount(),"field Create has a new RNG draw");
                while(p.shadow.drawCount()+1<p.actual.drawCount())p.shadow.nextU32();
                require(command.timeFieldRadiusWord()==p.shadow.nextU32(),"field command preserves exactly its source Create word");
                require(sameRandom(p.actual,p.shadow),"field capture preserves complete source random stream");
                ++p.fields;return true;
            },&probe,result),"field capture builder traversal");
        require(builder.endReached(),"field capture traverses whole level");
    }
    require(fieldCount>0,"natural builder patterns exercise ordinary time fields");
    std::cout << "retained time-field Create words " << fieldCount << '\n';
    for (unsigned seed = 1; seed <= 32; ++seed) {
        AreaOneBuilder configured;
        GameMakerRandom configuredRandom(seed);
        configured.reset(240, 704, configuredRandom, {2,4,true,false});
        require(configured.generationCount() == -10 && !configured.shopMade() &&
            configured.rightRoomCount() >= 3 && configured.rightRoomCount() <= 4 &&
            configured.rightRoom(0) == 0, "later-stage member setup retains extra first side room");
        for (unsigned i = 1; i < configured.rightRoomCount(); ++i) {
            require(configured.rightRoom(i) >= 3 && configured.rightRoom(i) <= 16,
                "stage-two side room range uses levelLength minus one");
            for (unsigned j = 0; j < i; ++j)
                require(std::abs(configured.rightRoom(i) - configured.rightRoom(j)) > 2,
                    "all side-room indices obey source separation");
        }
        Commands emitted;
        AreaOneBuilderStep configuredStep;
        for (unsigned step = 0; step < 100 && !configured.endReached(); ++step)
            require(configured.stepVisible(pack, configuredRandom, 100000, collect, &emitted, configuredStep),
                "configured shop traversal succeeds");
        unsigned shops = 0;
        for (const auto& command : emitted) if (command.objectId == 277) ++shops;
        require(configured.endReached() && configured.shopsGenerated() == 1 && shops == 1,
            "member source branch emits exactly one actual Shop object through pattern traversal");
        capture(emitted, configuredRandom);
        configuredRandom.setSeed(seed);
        configured.reset(240, 704, configuredRandom, {1,4,false,true});
        emitted.clear();
        for (unsigned step = 0; step < 100 && !configured.endReached(); ++step) {
            require(configured.stepVisible(pack, configuredRandom, 100000, collect, &emitted, configuredStep),
                "style-four nonmember traversal succeeds");
            if (configured.shopsGenerated())
                require(configured.rightRoomsMade() == configured.rightRoomCount(),
                    "style four postpones the shop until the last side room");
        }
        require(configured.endReached() && configured.shopsGenerated() == 1,
            "style four permits exactly one shop even on first level");
        capture(emitted, configuredRandom);
    }
    std::cout << "ordered emission digest " << std::hex << emissionDigest << std::dec << '\n';
    // Re-recorded after VM-audited level-dependent enemy spawning; the previous
    // 1fd36f0408435e8b kept level-one odds and omitted crawler cells. This is a regression
    // fingerprint, not an independent Windows-runner capture.
    require(legacyDigest == 0x970b28efcbf63992ULL,
        "retaining field radius changes no other Create fields or RNG state");
    require(preBoxDigest == 0xddd99e417bca728fULL,
        "retaining box Create choices changes no prior emission fields or RNG state");
    require(emissionDigest == 0x5c6663997c7bbf14ULL,
        "64 seeded levels retain ordered box sound/gem choices and complete RNG state");
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
                commands.size() == 2 && random.drawCount() == before.drawCount() + 20,
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
    unsigned plainWalls = 0, plainPlatforms = 0, crates = 0;
    for (const auto& command : directCommands) {
        if (command.objectId == 92) {
            DeferredCreateInstance expected(92, command.x, command.y);
            require(command.integerImageIndex == 16 || command.integerImageIndex == 17 ||
                        command.integerImageIndex == 22,
                    "crate frame remains one of its source Create choices");
            expected.imageSpeed = 0.0F;
            const auto box=command.boxCreate();
            require(box.breakSound>=171&&box.breakSound<=174&&
                box.haveGem==(command.integerImageIndex==22?2:0)&&box.wallHp.high==0x3ff00000u&&
                !box.wallHp.low&&!box.hit&&!box.drilled&&!box.autoTile&&!box.checked,
                "crate command retains its source Create fields without repeating RNG");
            expected.setBoxCreate(box.breakSound,box.haveGem);
            expected.integerImageIndex = command.integerImageIndex;
#ifdef DOWNWELL_FREESTANDING
            expected.imageIndex = command.integerImageIndex == 22 ? 22.0F : 0.0F;
#else
            expected.imageIndex = static_cast<float>(command.integerImageIndex);
#endif
            require(sameCommand(command, expected),
                    "crate commands retain fresh non-presentation fields and alarms");
            ++crates;
            continue;
        }
        if (command.objectId != 60 && command.objectId != 93) continue;
        const DeferredCreateInstance expected(command.objectId, command.x, command.y);
        require(sameCommand(command, expected),
            "reused plain terrain commands retain fresh defaults including all alarms");
        if (command.objectId == 60) ++plainWalls; else ++plainPlatforms;
    }
    require(plainWalls > 100 && plainPlatforms > 0 && crates > 0,
        "default-field checks cover walls, platforms and crates interleaved with actors");
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
