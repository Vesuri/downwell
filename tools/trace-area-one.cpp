#include "core/BuilderPatternPack.h"
#include "core/ObjectPack.h"
#include "core/RoomPack.h"
#include "core/StartupRuntime.h"

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

namespace {

struct ExpectedFragment {
    int y;
    const char* family;
    int pattern;
    bool reversed;
    std::uint32_t rows;
};

constexpr ExpectedFragment expectedFragments[] = {
    {1168, "start", 3, true, 5},
    {1248, "main", 32, true, 14},
    {1472, "main", 30, true, 19},
    {1776, "main", 27, true, 12},
    {1968, "right", 6, false, 16},
    {2224, "main", 22, true, 12},
    {2416, "main", 14, true, 9},
    {2560, "main", 23, true, 12},
    {2752, "right", 7, true, 16},
    {3008, "main", 20, true, 14},
    {3232, "main", 21, true, 14},
    {3456, "main", 18, false, 16},
    {3712, "main", 32, true, 14},
    {3936, "main", 2, true, 10},
    {4096, "main", 10, false, 14},
    {4320, "main", 35, false, 20},
    {4640, "main", 16, true, 13},
    {4848, "main", 7, false, 14},
    {5072, "end", 1, false, 13},
};

std::vector<std::uint8_t> readBytes(const char* path) {
    std::ifstream input(path, std::ios::binary);
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(input),
                                     std::istreambuf_iterator<char>());
}

[[noreturn]] void fail(const char* message) {
    std::cerr << "area-one trace failed: " << message << '\n';
    std::exit(1);
}

void step(downwell::StartupRuntime& runtime,
          const downwell::Input& input, bool pressed = false) {
    runtime.step(input, pressed);
    if (!runtime.roomLoadSucceeded() || !runtime.deferredCommandsSucceeded())
        fail("reference runtime rejected a room or deferred command");
}

void enterMain(downwell::StartupRuntime& runtime) {
    const downwell::Input released;
    downwell::Input right;
    right.right = true;
    downwell::Input fire;
    fire.fire = true;

    step(runtime, released);
    for (int frame = 0; frame < 1200 && !runtime.inPlayMenu(); ++frame)
        step(runtime, right);
    if (!runtime.inPlayMenu())
        fail("autopilot did not enter style select");
    for (int frame = 0;
         frame < 180 && runtime.playMenuYOffset() != 0.0F; ++frame)
        step(runtime, released);
    if (runtime.playMenuYOffset() != 0.0F)
        fail("style-select entrance did not settle");
    step(runtime, fire, true);
    for (int frame = 0; frame < 180 && !runtime.inMainRoom(); ++frame)
        step(runtime, released);
    if (!runtime.inMainRoom())
        fail("autopilot did not enter rmMain");
}

const char* familyName(const downwell::StartupRuntime& runtime,
                       std::uint32_t oldMain,
                       std::uint32_t oldSide,
                       std::uint32_t oldShop,
                       bool oldEnd,
                       int& pattern) {
    if (runtime.mainBuilderStartPattern() >= 0 && oldMain == 0 &&
        runtime.mainBuilderGenerationCount() == 0) {
        pattern = runtime.mainBuilderStartPattern();
        return "start";
    }
    if (!oldEnd && runtime.mainBuilderEndReached()) {
        pattern = runtime.mainBuilderEndPattern();
        return "end";
    }
    if (runtime.mainBuilderShopsGenerated() != oldShop) {
        pattern = -1;
        return "shop";
    }
    if (runtime.mainBuilderSideRoomsGenerated() != oldSide) {
        pattern = runtime.mainBuilderLastRightPattern();
        return "right";
    }
    if (runtime.mainBuilderMainPatternsGenerated() != oldMain) {
        pattern = runtime.mainBuilderLastMainPattern();
        return "main";
    }
    pattern = -1;
    return "prelude";
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "usage: " << argv[0]
                  << " ROOM_PACK OBJECT_PACK BUILDER_PACK\n";
        return 2;
    }
    const std::vector<std::uint8_t> roomBytes = readBytes(argv[1]);
    const std::vector<std::uint8_t> objectBytes = readBytes(argv[2]);
    const std::vector<std::uint8_t> builderBytes = readBytes(argv[3]);
    const downwell::RoomPack rooms(roomBytes.data(), roomBytes.size());
    const downwell::ObjectPack objects(objectBytes.data(), objectBytes.size());
    const downwell::BuilderPatternPack builders(
        builderBytes.data(), builderBytes.size());
    if (!rooms.valid() || !objects.valid() || !builders.valid())
        fail("one or more generated recovery packs are invalid");

    downwell::StartupRuntime runtime(
        downwell::PlatformProfile::nonDesktop, &rooms, &objects,
        downwell::StartupRuntime::defaultRandomSeed, &builders);
    enterMain(runtime);
    std::cerr << "area-one entrance checkpoint: generation="
              << runtime.mainBuilderGenerationCount()
              << " y=" << runtime.mainBuilderY()
              << " right=" << runtime.mainBuilderRightRoom(0)
              << ',' << runtime.mainBuilderRightRoom(1)
              << " RNG draws=" << runtime.randomDrawCount() << '\n';

    unsigned fragment = 0;
    std::cout << "fragment\ty\tfamily\tpattern\treversed\trows\n";
    for (int frame = 0;
         frame < 256 && !runtime.mainBuilderEndReached(); ++frame) {
        const int oldY = runtime.mainBuilderY();
        const std::uint32_t oldRows = runtime.mainBuilderRowsGenerated();
        const std::uint32_t oldMain = runtime.mainBuilderMainPatternsGenerated();
        const std::uint32_t oldSide = runtime.mainBuilderSideRoomsGenerated();
        const std::uint32_t oldShop = runtime.mainBuilderShopsGenerated();
        const bool oldEnd = runtime.mainBuilderEndReached();

        /* Keep scrBuilder just inside its recovered 128-pixel frontier. */
        runtime.traceSetMainCameraViewY(
            static_cast<float>(oldY - downwell::StartupRuntime::surfaceHeight));
        step(runtime, downwell::Input{});

        if (runtime.mainBuilderRowsGenerated() == oldRows)
            continue;
        if (runtime.mainBuilderGenerationCount() < 0)
            continue;
        int pattern = -1;
        const char* family = familyName(runtime, oldMain, oldSide, oldShop,
                                        oldEnd, pattern);
        const std::uint32_t rows =
            runtime.mainBuilderRowsGenerated() - oldRows;
        const bool reversed = runtime.mainBuilderLastPatternReversed();
        if (fragment >= sizeof(expectedFragments) /
                            sizeof(expectedFragments[0]))
            fail("reference emitted more fragments than the pinned trace");
        const ExpectedFragment& expected = expectedFragments[fragment];
        if (oldY != expected.y || std::strcmp(family, expected.family) != 0 ||
            pattern != expected.pattern || reversed != expected.reversed ||
            rows != expected.rows)
            fail("reference fragment diverged from the pinned trace");
        std::cout << fragment++ << '\t' << oldY << '\t' << family << '\t'
                  << pattern << '\t'
                  << (reversed ? 1 : 0) << '\t' << rows
                  << '\n';
    }
    if (!runtime.mainBuilderEndReached()) {
        std::cerr << "frontier trace stop: generation="
                  << runtime.mainBuilderGenerationCount()
                  << " builderY=" << runtime.mainBuilderY()
                  << " player=" << runtime.mainPlayerX() << ','
                  << runtime.mainPlayerY()
                  << " grounded=" << runtime.mainPlayerGrounded()
                  << " health=" << runtime.mainPlayerHealth()
                  << " ammo=" << runtime.mainAmmo() << '\n';
        fail("camera-frontier trace did not reach the area-one ending");
    }
    if (fragment != sizeof(expectedFragments) / sizeof(expectedFragments[0]) ||
        runtime.mainBuilderRowsGenerated() != 286 ||
        runtime.randomDrawCount() != 4770)
        fail("complete trace summary diverged from its pinned reference");
    std::cerr << "area-one trace passed: " << fragment
              << " fragments, " << runtime.mainBuilderRowsGenerated()
              << " rows, " << runtime.randomDrawCount() << " RNG draws\n";
    return 0;
}
